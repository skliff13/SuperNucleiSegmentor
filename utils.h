
cv::Mat matEqualsInt(cv::Mat im, int val);

cv::Mat fillHoles(cv::Mat bw);

cv::Vec3f rgbImageQuantiles(cv::Mat im, float q);

cv::Mat generateSuperpixels(cv::Mat im, int spSize, float reg);

cv::Mat superpixelsBorders(cv::Mat spmap);

cv::Mat bwLabel(cv::Mat bw);

double matMax2D(cv::Mat m);

cv::Mat biggestComponent(cv::Mat bw);
