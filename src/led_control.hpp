#ifndef __TETON_LED_CONTROL_HPP__
#define __TETON_LED_CONTROL_HPP__

#include <opencv2/core.hpp>

namespace teton {

// TODO: Implement this function. Make it as fast as possible.
static bool computeLEDSignalFromImageBrightness(const cv::Mat &image) {

    cv::Mat downScaled;

    cv::resize(image, downScaled, cv::Size(10, 10), cv::INTER_LINEAR);

    cv::Scalar tempVal = cv::mean( downScaled );

    float myMAtMean = (tempVal.val[0] + tempVal.val[1] + tempVal.val[2]) / 3 ;

    return myMAtMean < 40;
}

}  // namespace teton

#endif
