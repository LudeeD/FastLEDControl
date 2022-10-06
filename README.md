
# FastLEDControl

This repo is used as part of a coding challenge. The goal is to create a fast and
efficient algorithm to control some IR LEDs.

**Please read this README carefully.** 👀

## Challenge 🦾

Your main task is to extend the provided codebase to generate a boolean signal to control LEDs based on the incoming frames.

Since our product is vision based, it's important that we see what's going on in the room. During the night, we need to turn on the IR LEDs to generate some light. The wavelength of the LEDs is outside the visible spectrum of humans, so it doesn't disturb the patients' sleep. In this challenge, you won't need to worry about the actual LEDs, but you will need to generate a boolean signal that would control them.

**Here's a breakdown of what you need to do:**

1. Implement the `computeLEDSignalFromImageBrightness(cv::Mat &)` function declared in `src/led_control.hpp`. You can use any technique that you wish, but you should make sure that the signal is calculated as fast as possible. The function is called for every frame, so it should be as fast as possible. You can add any other files that you need during your implementation.
1. Profile your implementation and print out stats about the processing speed to the console when the `TETON_BENCHMARK` compiler flag is set. Otherwise run the code without profiling.
1. The control signal is published at regular intervals, but if the signal changes, it should be published immediately.
1. Implement a separate client node that receives the LED control signal via the MQTT message. You should use C or C++ to implement the client. You can take a look at `src/network/client.hpp` for how the provided C++ client works. The client should print the incoming signals to the console.
1. Implement a new MQTT signal that will be used to request an update of the signal value. The client can send this signal at any time, and the `FastLEDControl` executable should respond with the current signal value immediately (even if it's the same value as the last time it published the message). Let's call this signal `"local/update/led"`.

The goal of the challenge is not to find the most robust algorithm to generate a control signal based on the image, although it should still make sense what you're doing. The focus is more on your ability to write efficient code and integrate your work into an existing codebase.

You should aim to integrate your solution with the provided codebase, but feel free to take ownership of the code and refactor it as you see fit. You can also add any other files that you need to implement your solution. You may also add any other dependencies that you need.

## Setup and prerequisites

We compiled a script for you in `scripts/install_dependencies.sh` which should install everything you should need to build and run the provided sample code. The main dependencies it installs are:

1. [OpenCV](https://github.com/opencv/opencv)
1. [Mosquitto](https://mosquitto.org/download/)
1. [Paho MQTT C](https://github.com/eclipse/paho.mqtt.c)
1. [Paho MQTT CPP](https://github.com/eclipse/paho.mqtt.cpp)

You should be **able to build and run the provided codebase** if you've correctly installed all prerequisites. If you have any problems, please let us know.

## Running the executable

You should provide the path to a video file as command line argument to the executable. Alternatively, feel free to modify the code to use a webcam as well.

In order to run this node, you'll have to have the following environment variables set:

* `TETON_ROOM_NO`: the number of the room in which the device is deployed,
* `TETON_BED_NO`: the number of the bed that the device is monitoring.

From the challenge's point of view, it doesn't matter what values you pick for these. For example:

* `TETON_ROOM_NO=10`,
* `TETON_BED_NO=1`.

To build the project, you should use `cmake` and `make`.

### Compiler flags

The `TETON_DEBUG` flag is configured in the `CMakeLists.txt` file, and can be either `ON` or `OFF` (default). When `ON`, you'll have visualization turned on.

The `TETON_BENCHMARK` flag configured in the `CMakeLists.txt` file can be used to benchmark the code. It can be either `ON` or `OFF` (default). When `ON`, you'll have benchmarking log outputs in the terminal.
