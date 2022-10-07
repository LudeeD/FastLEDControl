#ifndef __TETON_LED_CONTROL_HPP__
#define __TETON_LED_CONTROL_HPP__

#include <opencv2/core.hpp>

namespace teton {

// TODO: Implement this function. Make it as fast as possible.
bool computeLEDSignalFromImageBrightness(const cv::Mat &image) {
    CV_Assert(image.depth() == CV_8U);

    static int channels = image.channels();
    static int nRows = image.rows;
    static int nCols = image.cols * channels;
    static int nTotal = nRows * nCols;

    int i,j;
    const uchar* p;
    double sum = 0;
    for( i = 0; i < nRows; ++i)
    {
        p = image.ptr<uchar>(i);
        for ( j = 0; j < nCols; ++j)
        {
            sum += p[j];
        }
    }

    return (sum / nTotal) < 40;
}

}  // namespace teton

#endif
