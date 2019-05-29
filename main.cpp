#include "stdafx.h"

#define SETTINGSFILE "settings.txt"
#define ADJUSTLEVELS 3

struct Settings{
    char pngPreviewsDir[1000];
    char outputDir[1000];
    float displayResize;
    int displayTileSize;
    int nTiles;
    int tileSizeStep;
} sts;

struct TilesInfo{
    bool isProcessed = false;
    bool isSelected = false;
    Mat tile;
    Mat spmap;
    Point topleft;
};

Mat originalImage;
Mat smoothedImage;
Mat segmMap;
int segmIdx;
TilesInfo * ti;
Point cursorPos, lastClick, tileTopLeft, displaySize;
int tilesSize;
bool showSegm = true, inverted = false;
char winname[1000], seriesName[1000];
int frameIdx, adjustLevel = 0;
int approveCounter = 0;

/*----------------------------------------------------------------------------*/
bool segmentNuclei();
void repaintAll();
bool loadData();
void saveData();
void autosave();
void loadSettings();
/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{   
//    argc = 2;
//    argv[1] = "series_018_XY_point_19";
//    argv[2] = "1";

    char version[] = "0.3";

    printf("\n\n*==============================*\n");
    printf("*    Super Nuclei Segmentor    *\n");
    printf("*==============================*\n");
    printf("ver %s\n\n", version);

    char usage[] = "Usage :   SuperNucleiSegmentorCmd.exe <series_name> <frame_index>\n\n";
    if (argc < 2 || argc > 3){
        printf(usage);

        return -1;
    }
    else if (argc == 2){
        printf(usage);
        printf("Parameter <frame_index> not specified. Using frame_index = 1.\n\n");
        frameIdx = 1;
    }
    else {
        frameIdx = atoi(argv[2]);
    }

    if (frameIdx < 1){
        printf("Invalid frame index '%s'\n\n", argv[2]);
        return -1;
    }

    strcpy(seriesName, argv[1]);

    char buff[1000];
    LOG_Begin(version);
    sprintf(buff, "series_name = '%s', frame_index = %i", seriesName, frameIdx);
    LOG(buff);

    loadSettings();

    if (loadData())
        segmentNuclei();

    delete ti;
    LOG_End();
    return 0;
}

void readSettingStr(char * dest, char * settingName, char * defaultValue){
    char buf[2000];
    FILE * f = fopen(SETTINGSFILE, "rt");

    if (f == NULL){
        sprintf(buf, "Couldn't read setting '%s' from '%s'. Using default value '%s'",
                settingName, SETTINGSFILE, defaultValue);
        LOG(buf);
        strcpy(dest, defaultValue);
    }

    while (!feof(f)) {
        if (fgets(buf, 1000, f) != NULL){
            buf[strlen(buf) - 1] = 0;
            string str(buf);
            if (str.find(settingName) == 0){
                char * parStr = buf + str.find("=") + 1;
                strcpy(dest, parStr);
                fclose(f);
                return;
            }
        }
    }
    fclose(f);

    sprintf(buf, "Couldn't read setting '%s' from '%s'. Using default value '%s'",
            settingName, SETTINGSFILE, defaultValue);
    LOG(buf);
    strcpy(dest, defaultValue);
}

float readSettingFloat(char * settingName, float defaultValue){
    float val = defaultValue;

    char buf[1000];
    FILE * f = fopen(SETTINGSFILE, "rt");

    if (f == NULL){
        sprintf(buf, "Couldn't read setting '%s' from '%s'. Using default value %.2f",
                settingName, SETTINGSFILE, defaultValue);
        LOG(buf);
        return val;
    }

    while (!feof(f)) {
        if (fgets(buf, 1000, f) != NULL){
            string str(buf);
            if (str.find(settingName) == 0){
                char * parStr = buf + str.find("=") + 1;
                val = atof(parStr);
                fclose(f);
                return val;
            }
        }
    }
    fclose(f);

    sprintf(buf, "Couldn't read setting '%s' from '%s'. Using default value %.2f",
            settingName, SETTINGSFILE, defaultValue);
    LOG(buf);
    return val;
}

void replaceAll(string& str, const string& from, const string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

void loadSettings(){
    char pngPreviewsDir[1000], outputDir[1000];

    LOG("Loading settings");
    sts.displayResize = readSettingFloat((char *)"displayResize", 0.6);
    sts.displayTileSize = (int)readSettingFloat((char *)"displayTileSize", 180);
    sts.nTiles = (int)readSettingFloat((char *)"nTiles", 3);
    sts.tileSizeStep = (int)readSettingFloat((char *)"tileSizeStep", 16);
    readSettingStr(pngPreviewsDir, (char *)"pngPreviewsDir", (char *)"");
    readSettingStr(outputDir, (char *)"outputDir", (char *)"");

    string strpng(pngPreviewsDir);
    replaceAll(strpng, string("\\"), string("/"));
    string strout(outputDir);
    replaceAll(strout, string("\\"), string("/"));

    strcpy(sts.pngPreviewsDir, strpng.c_str());
    strcpy(sts.outputDir, strout.c_str());

    char buff[2000];
    sprintf(buff, "displayResize = %.3f, displayTileSize = %i, nTiles = %i, tileSizeStep = %i",
            sts.displayResize, sts.displayTileSize, sts.nTiles, sts.tileSizeStep);
    LOG(buff);
    sprintf(buff, "pngPreviewsDir = %s", sts.pngPreviewsDir);
    LOG(buff);
    sprintf(buff, "outputDir = %s", sts.outputDir);
    LOG(buff);
}

void checkAndLoadPreviousSegmentation()
{
    char segmpath[2000];
    sprintf(segmpath, "%s/%s_%03i_nuclei.tif", sts.outputDir, seriesName, frameIdx);

    if (access(segmpath, 0) == 0){
        printf("Loading previous segmentation file from '%s'\n", segmpath);

        Mat sm = imread(segmpath, CV_LOAD_IMAGE_ANYDEPTH);

        if (sm.data != NULL){
            Mat channels[3];
            split(sm, channels);
            channels[0].convertTo(segmMap, CV_32SC1);
            double mx;
            minMaxIdx(segmMap, NULL, &mx);
            segmIdx = (int)mx;
            printf("Regions loaded: %i\n", segmIdx);
        }
        else
            printf("Couldn't load previous segmentation file from '%s'\n", segmpath);
    }
}

bool loadData(){
    char filepath[2000], buff[1000];
    sprintf(filepath, "%s/%s_%03i.png", sts.pngPreviewsDir, seriesName, frameIdx);

    sprintf(buff, "Reading RGB image from '%s'", filepath);
    LOG(buff);
    originalImage = imread(filepath);

    if (originalImage.data == NULL){
        sprintf(buff, "Could not read preview for '%s' and frameIdx %i", seriesName, frameIdx);
        LOG(buff);
        return false;
    }

    GaussianBlur(originalImage, smoothedImage, Size(11, 11), 1);
    segmMap = Mat(originalImage.rows, originalImage.cols, CV_32SC1, Scalar::all(0));
    segmIdx = 0;

    lastClick = Point(100, 100);

    ti = new TilesInfo[sts.nTiles];
    for (int i = 0; i < sts.nTiles; i++)
        ti[i].tile = Mat(sts.displayTileSize, sts.displayTileSize, CV_8UC3, Scalar::all(100));
    tilesSize = 100;
    sprintf(buff, "Tile size: %i", tilesSize);
    LOG(buff);
    cursorPos = Point(-1000, -1000);

    checkAndLoadPreviousSegmentation();

    return true;
}

void saveData(){
    char segmpath[2000], buff[1000];
    sprintf(segmpath, "%s/%s_%03i_nuclei.tif", sts.outputDir, seriesName, frameIdx);

    sprintf(buff, "Saving segmentation to '%s'", segmpath);
    LOG(buff);

    Mat sm;
    segmMap.convertTo(sm, CV_16UC1);
    imwrite(segmpath, sm);
}

void autosave(){
    char segmpath[2000], buff[1000];
    sprintf(segmpath, "%s/_autosave.tif", sts.outputDir);

    sprintf(buff, "Autosave to '%s'", segmpath);
    LOG(buff);

    Mat sm;
    segmMap.convertTo(sm, CV_16UC1);
    imwrite(segmpath, sm);
}

void copyIntoImageRGB(Mat src, Mat dst, Point topleft){
    for (int i = 0; i < src.rows; i++){
        for (int j = 0; j < src.cols; j++){
            dst.at<Vec3b>(topleft.y + i, topleft.x + j) = src.at<Vec3b>(i, j);
        }
    }
}

Mat cumulativeImage(Mat small)
{
    int tilesInCol = 3;
    int tileCols = 1 + (sts.nTiles - 1) / tilesInCol;
    int neededRows = max(small.rows, 3 * sts.displayTileSize);
    int neededCols = small.cols + tileCols * sts.displayTileSize;
    Mat all(neededRows, neededCols, CV_8UC3, Scalar::all(0));

    copyIntoImageRGB(small, all, Point(0, 0));
    for (int i = 0; i < sts.nTiles; i++){
        Mat dtile;
        cv::resize(ti[i].tile, dtile, Size(sts.displayTileSize, sts.displayTileSize));

        if (ti[i].isSelected){
            rectangle(dtile, Point(1, 1), Point(sts.displayTileSize - 1, sts.displayTileSize - 1), Scalar(100, 0, 0), 3);
        }

        int c = i / tilesInCol;
        int r = i - tilesInCol * c;
        Point tl(small.cols, 0);
        tl += Point(c, r) * sts.displayTileSize;

        copyIntoImageRGB(dtile, all, tl);
    }

    return all;
}

Mat putSegmMap()
{
    Mat toShow, channels[3];
    originalImage.copyTo(toShow);

    Mat sm;
    segmMap.convertTo(sm, CV_8UC1, 255.);

    split(toShow, channels);
    sm.copyTo(channels[0]);
    channels[1] -= sm;
    channels[2] -= sm;

    vector<Mat> mv({channels[0], channels[1], channels[2]});

    merge(mv, toShow);

    return toShow;
}

void repaintAll(){
    Mat small(originalImage);

    Mat toShow;
    if (showSegm)
        toShow = putSegmMap();
    else
        originalImage.copyTo(toShow);

    int w = int(originalImage.cols * sts.displayResize);
    int h = int(originalImage.rows * sts.displayResize);
    displaySize = Size(w, h);
    cv::resize(toShow, small, displaySize);

    circle(small, cursorPos, int(tilesSize * 2 * sts.displayResize / 6.), Scalar(255, 255, 255));

    float mult = 1. / (1. - float(adjustLevel) / ADJUSTLEVELS);
    small *= mult;

    if (inverted){
        Mat tmp;
        small.convertTo(tmp, CV_32FC3);
        tmp = 1 - tmp / 255;
        tmp.convertTo(small, CV_8UC3, 255.);
    }

    Mat all = cumulativeImage(small);

    imshow(winname, all);
}

TilesInfo extractTile(int origx, int origy, int tileSize)
{
    int top = origy - tileSize / 2;
    top = max(top, 0);
    top = min(top, originalImage.rows - tileSize);
    int left = origx - tileSize / 2;
    left = max(left, 0);
    left = min(left, originalImage.cols - tileSize);

    tileTopLeft = Point(left, top);
    Mat tile;
    smoothedImage(Range(top, top + tileSize), Range(left, left + tileSize)).copyTo(tile);
    Vec3f qs = rgbImageQuantiles(tile, 0.90);

    float maxq = max(qs[0], max(qs[1], qs[2]));
    tile.convertTo(tile, tile.type(), 240. / maxq);

    int spSize = tileSize / 4;
    float reg = 80;
    Mat spmap = generateSuperpixels(tile, spSize, reg);
    Mat spborders = superpixelsBorders(spmap);
    spborders.convertTo(spborders, CV_8U, 255.);
    cvtColor(spborders, spborders, CV_GRAY2BGR);
    tile = tile + spborders;

    Point cntr(tile.rows / 2, tile.cols / 2);
    tile.at<Vec3b>(cntr.y, cntr.x - 1) = Vec3b(255, 0, 0);
    tile.at<Vec3b>(cntr.y, cntr.x + 1) = Vec3b(255, 0, 0);
    tile.at<Vec3b>(cntr.y - 1, cntr.x) = Vec3b(255, 0, 0);
    tile.at<Vec3b>(cntr.y + 1, cntr.x) = Vec3b(255, 0, 0);

    TilesInfo t;
    t.isProcessed = true;
    t.isSelected = false;
    t.tile = tile;
    t.spmap = spmap;
    t.topleft = tileTopLeft;
    return t;
}

void tileAround(int clickx, int clicky){

    int origx = clickx / sts.displayResize;
    int origy = clicky / sts.displayResize;

    for (int i = 0; i < sts.nTiles; i++){
        int ofs = sts.nTiles / 2;
        int tileSize = tilesSize + (i - ofs) * sts.tileSizeStep / 2;
        ti[i] = extractTile(origx, origy, tileSize);
    }
}

void processImageClick(Point pos){
    tileAround(pos.x, pos.y);

    repaintAll();
}

void onMouse( int event, int x, int y, int , void * ){
    cursorPos = Point(x, y);

    if (event == CV_EVENT_LBUTTONDOWN){
        Point pos = cursorPos;

        if (pos.x > 0 && pos.y > 0 && pos.x < displaySize.x - 1
                && pos.y < displaySize.y - 1){
            lastClick = pos;
            processImageClick(pos);
        }
    }
}

void rollbackSegm(){
    char buff[1000];
    if (segmIdx > 0){
        Mat selSegm, floatSegm;
        segmMap.convertTo(floatSegm, CV_32FC1);
        threshold(floatSegm, selSegm, segmIdx - 0.5, segmIdx, CV_THRESH_BINARY);
        floatSegm -= selSegm;
        floatSegm.convertTo(segmMap, CV_32SC1);
        sprintf(buff, "Cancelling region #%i", segmIdx);
        LOG(buff);
        segmIdx--;
    }
}

void approveTile(int tileIdx){
    char buff[1000];
    if (tileIdx < sts.nTiles){
        TilesInfo t = ti[tileIdx];

        if (t.isProcessed){            
            for (int k = 0; k < sts.nTiles; k++){
                if (ti[k].isSelected)
                    rollbackSegm();

                ti[k].isSelected = k == tileIdx;
            }

            Mat sp = t.spmap;
            Point tl = t.topleft;
            Point cnt(sp.cols / 2, sp.rows / 2);
            int spIdx = sp.at<int>(cnt.y, cnt.x);

            Mat selSp = matEqualsInt(sp, spIdx);
            Mat holesFilled = fillHoles(selSp);
            Mat niceSp = biggestComponent(holesFilled);

            segmIdx++;
            sprintf(buff, "Creating region #%i", segmIdx);
            LOG(buff);
            sprintf(buff, "TileSize = %i, Top-left XY = (%i, %i), ", sp.cols, tl.x, tl.y);
            LOG(buff);
            for (int i = 0; i < sp.rows; i++){
                for (int j = 0; j < sp.cols; j++){
                    if (niceSp.at<uchar>(i, j) == 1){
                        segmMap.at<int>(tl.y + i, tl.x + j) = segmIdx;
                    }
                }
            }

            approveCounter++;
            if ((approveCounter % 10) == 0){
                autosave();
            }
        }
    }
}

bool segmentNuclei(){
    int key = -1;

    char tileKeys[] = "123456";
    char buff[1000];

    sprintf(winname, "%s frame %03i (%i x %i)", seriesName, frameIdx, originalImage.rows, originalImage.cols);
    repaintAll();
    setMouseCallback(winname, onMouse, NULL);

    while (key != 13){
        key = waitKey(5);

        repaintAll();
        if (key == 'i' || key == 'I')
            inverted = !inverted;
        if (key == 'j' || key == 'J')
            adjustLevel = (adjustLevel + 1) % ADJUSTLEVELS;
        if (key == 'z' || key == 'Z')
            rollbackSegm();
        if (key == 'p' || key == 'P')
            saveData();
        if (key == 'c' || key == 'C')
            showSegm = !showSegm;
        if (key == '+' || key == 'w' || key == 'W'){
            tilesSize = min(192, tilesSize - sts.tileSizeStep);
            sprintf(buff, "Tile size: %i", tilesSize);
            LOG(buff);
            processImageClick(lastClick);
        }
        if (key == '-' || key == 's' || key == 'S'){
            tilesSize = max(64, tilesSize + sts.tileSizeStep);
            sprintf(buff, "Tile size: %i", tilesSize);
            LOG(buff);
            processImageClick(lastClick);
        }
        if (key == 2424832){
            lastClick += Point(-1, 0);
            processImageClick(lastClick);
        }
        if (key == 2555904){
            lastClick += Point(1, 0);
            processImageClick(lastClick);
        }
        if (key == 2490368){
            lastClick += Point(0, -1);
            processImageClick(lastClick);
        }
        if (key == 2621440){
            lastClick += Point(0, 1);
            processImageClick(lastClick);
        }
        for (uchar i = 0; i < strlen(tileKeys); i++){
            if (key == tileKeys[i]){
                approveTile(i);
            }
        }

        if (key == 27){
            LOG("Exiting without saving");
            destroyWindow(winname);
            return false;
        }
    }
    LOG("Saving and exiting");
    saveData();
    destroyWindow(winname);

    return true;
}


