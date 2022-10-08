#include <thread>
#include <signal.h>
#include <iostream>

#include <boost/thread/thread.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "src/led_control.hpp"
#include "src/utils/utils.hpp"
#include "src/network/client.hpp"

// Generated header with information from CMake
#include "version_config.h"

const std::string TOPIC_LED_REQUEST_STATUS = "local/update/led";  // Topic for LED signal

volatile sig_atomic_t sigInterrupt = 0;
void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("Received SIGINT...\n");
        sigInterrupt = 1;
    }
    if (signo == SIGTERM) {
        printf("Received SIGTERM...\n");
        sigInterrupt = 1;
    }
}

int main(int argc, char **argv) {
    // Friendly log to ensure we are using the right version of the code
    printf("******* %s v%s %s *******\n", PROJECT_NAME, PROJECT_VERSION, CMAKE_BUILD_TYPE);

    if (argc < 2) {
        printf("ERROR: Path to video file not provided...\n");
        printf("  Usage with video file: %s <path_to_video_file>\n", argv[0]);
        printf("  Usage with webcam    : %s /dev/video0 \n", argv[0]);
        return -1;
    }

    // Set up SIGINT/SIGTERM handler for graceful shutdown
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("Can't catch SIGINT...\n");
    if (signal(SIGTERM, sig_handler) == SIG_ERR)
        printf("Can't catch SIGTERM...\n");

    std::string topicLED = "local/signal/led";  // Topic for LED signal
    int captureWaitTime = 20;  // Interval in seconds that we wait at max to receive a frame from the camera
    int LEDControlSignalPeriod = 10;  // Interval in seconds that we send the desired LED state

    // Query static environment variables
    std::string tetonRoomNoStr;
    std::string tetonBedNoStr;
    bool nice = 
      teton::utils::getEnvVar("TETON_BED_NO", tetonBedNoStr) && 
      teton::utils::getEnvVar("TETON_ROOM_NO", tetonRoomNoStr);

    if (!nice) {
        std::cerr
            << "\033[1;31mEnvVar TETON_BED_NO or TETON_ROOM_NO not present!\033[0m"
            << std::endl;
        return -1;
    }

    // MQTT client connection setup
    const std::string clientId = "FastLEDControl_" + tetonRoomNoStr + "_" + tetonBedNoStr;

    teton::network::Client client("localhost:1883", clientId);
    if (!client.connect()) {
        std::string errorString = "Room " + tetonRoomNoStr + " Bed " + tetonBedNoStr + " Failed to connect to local MQTT master";
        std::cerr << errorString << std::endl;
        return -1;
    }
    client.subscribe(TOPIC_LED_REQUEST_STATUS);

    // Create input stream
    cv::VideoCapture cap(argv[1]);

    if (!cap.isOpened()) {
        std::cerr << "Error opening input stream..." << std::endl;
        return -1;
    }

    auto timeOfLastCapture = std::chrono::high_resolution_clock::now();
    auto timeOfLastLEDControlSignalSent = std::chrono::high_resolution_clock::now();

    bool   lastSignalSent = false;

    // Do inference until node is stopped
    while (!sigInterrupt) {
        // Capture a new frame
        cv::Mat frame;
        cap >> frame;

        // If we have not captured a frame for 20 seconds, something is really wrong
        if (frame.empty()) {
            auto timeSinceLastCapture = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::high_resolution_clock::now() - timeOfLastCapture
            );
            if (timeSinceLastCapture.count() > captureWaitTime) {
                printf("Camera is not streaming...\n");
                sigInterrupt = 1;
            }

            // Sleep for 10 milliseconds
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            continue;
        }

        timeOfLastCapture = std::chrono::high_resolution_clock::now();

        // Determine whether we should turn the LEDs on or off
        bool turnLEDsOn = teton::computeLEDSignalFromImageBrightness(frame);

#ifdef TETON_BENCHMARK
        static auto maxProcessingTime = std::chrono::duration<double>(0);
        static double movingAvgTime = 0.0;
        static double movingAvgCount = 0;

        std::chrono::duration<double> timeToProcessFrame = std::chrono::high_resolution_clock::now() - timeOfLastCapture;
        if (timeToProcessFrame > maxProcessingTime) {
            maxProcessingTime = timeToProcessFrame;
        } 
        auto elapsedTime = timeToProcessFrame.count();
        std::cout << "Frame " << movingAvgCount << std::endl;
        std::cout << "elapsed time: " << elapsedTime  << std::endl;
        movingAvgTime += (elapsedTime - movingAvgTime) / movingAvgCount;
        std::cout << "moving  avg : " << timeToProcessFrame.count() << "s" << std::endl;
        std::cout << "max time    : " << maxProcessingTime.count() << "s" << std::endl;
        movingAvgCount += 1;
#endif //TETON_BENCHMARK

        // Send signal to turn LEDs on/off
        auto timeSinceLastLEDControlSignalSent = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now() - timeOfLastLEDControlSignalSent
        );

        // check if we received an update request
        auto request = client.getBool(TOPIC_LED_REQUEST_STATUS);

        if (timeSinceLastLEDControlSignalSent.count() > LEDControlSignalPeriod || lastSignalSent != turnLEDsOn || std::get<0>(request) ) {
            timeOfLastLEDControlSignalSent = std::chrono::high_resolution_clock::now();
            lastSignalSent = turnLEDsOn;
            client.publish(turnLEDsOn, clientId, tetonRoomNoStr, tetonBedNoStr, topicLED);
        }

#ifdef TETON_DEBUG
        std::string ledText = std::string("LED: ") + (turnLEDsOn ? "ON" : "OFF");
        cv::putText(frame, ledText, cv::Point(80, 100), cv::FONT_HERSHEY_COMPLEX, 2, cv::Scalar(255, 255, 255), 3);

        cv::Mat downScaled;
        cv::resize(frame, downScaled, cv::Size(960, 720), cv::INTER_LINEAR);

        cv::imshow("Debug Visualization", downScaled);
        int keycode = cv::waitKey(1) & 0xff;
        if (keycode == 27) {
            sigInterrupt = 1;
            break;
        }
#endif
    }

    // Clean up
    client.disconnect();

    return 0;
}
