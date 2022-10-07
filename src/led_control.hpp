#ifndef __TETON_LED_CONTROL_HPP__
#define __TETON_LED_CONTROL_HPP__

#include <opencv2/core.hpp>

namespace teton {

// TODO: Implement this function. Make it as fast as possible.
bool computeLEDSignalFromImageBrightness(const cv::Mat &image) {


    cv::Scalar tempVal = cv::mean( image );
    float myMAtMean = tempVal.val[0];

    return myMAtMean < 40;
}

}  // namespace teton

#endif
