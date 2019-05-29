#include "stdafx.h"

Vec3f rgbImageQuantiles(Mat im, float q){
    Mat channels[3];
    split(im, channels);

    Vec3f quantiles;
    for (int c = 0; c < 3; c++){
        Mat ch = channels[c];

        vector<uchar> v;
        v.reserve(ch.rows * ch.cols);
        for (int i = 0; i < ch.rows; i++){
            for (int j = 0; j < ch.cols; j++){
                uchar pix = ch.at<uchar>(i, j);
                v.push_back(pix);
            }
        }

        sort(v.begin(), v.end());

        int idx = int(floor(v.size() * q));
        idx = max(0, min(idx, (int)v.size()));
        quantiles[c] = v[idx - 1];
    }

    return quantiles;
}

Mat generateSuperpixels(Mat im, int spSize, float reg){
    Mat spmap(im.rows, im.cols, CV_32S, Scalar::all(0));

    IplImage ipl(im), spipl(spmap);
    Slic slic;
    slic.generate_superpixels(&ipl, spSize, reg);
    slic.create_connectivity(&ipl);

    slic.colour_with_cluster_indices(&spipl);

    Mat spmap1(&spipl, true);
    return spmap1;
}

Mat superpixelsBorders(Mat spmap){
    Mat spfloat(spmap.rows, spmap.cols, CV_32F);
    spmap.convertTo(spfloat, CV_32F);
    Mat spborders;
    Laplacian(spfloat, spborders, spfloat.depth());

    return spborders;
}

Mat matEqualsInt(Mat im, int val){
    Mat result(im.rows, im.cols, CV_8UC1, Scalar::all(0));
    for (int i = 0; i < im.rows; i++){
        for (int j = 0; j < im.cols; j++){
            if (im.at<int>(i, j) == val){
                result.at<uchar>(i, j) = 1;
            }
        }
    }

    return result;
}

Mat matEqualsUchar(Mat im, uchar val){
    Mat result(im.rows, im.cols, CV_8UC1, Scalar::all(0));
    for (int i = 0; i < im.rows; i++){
        for (int j = 0; j < im.cols; j++){
            if (im.at<uchar>(i, j) == val){
                result.at<uchar>(i, j) = 1;
            }
        }
    }

    return result;
}


Mat biggestComponent(Mat bw){
    Mat lbl = bwLabel(bw);
    double lblmax;
    minMaxLoc(lbl, NULL, &lblmax);
    int nlbl = (int)lblmax;
    int lBiggest = -1, sizeOfBiggest = 0;

    for (int l = 1; l <= nlbl; l++){
        Mat selComponent = matEqualsUchar(lbl, (uchar)l);
        Scalar s = sum(selComponent);

        int compSz = (int)s[0];
        if (compSz > sizeOfBiggest){
            lBiggest = l;
            sizeOfBiggest = compSz;
        }
    }

    Mat biggestComp = matEqualsUchar(lbl, (uchar)lBiggest);
    return biggestComp;
}

Mat bwLabel(Mat bw){
    Mat lbl;
    bw.convertTo(lbl, CV_8UC1, 255.);

    int nlbl = 0;
    for (int i = 0; i < bw.rows; i++){
        for (int j = 0; j < bw.cols; j++){
            Point p(j, i);
            if (lbl.at<uchar>(p) == 255){
                nlbl++;
                floodFill(lbl, p, Scalar::all(nlbl), NULL, Scalar(), Scalar(), 8);
            }
        }
    }

    return lbl;
}

Mat fillHoles(Mat bw){
    Mat flooded;
    bw.copyTo(flooded);
    floodFill(flooded, Point(0, 0), Scalar::all(1));
    Mat holes = - flooded + 1;
    Mat filled = bw + holes;

    return filled;
}
