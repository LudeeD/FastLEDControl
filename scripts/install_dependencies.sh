#!/bin/bash

ERROR='\033[0;31m[ERROR]'
WARN='\033[0;33m[WARNING]\033[0m'
INFO='\033[0;36m[INFO]\033[0m'
DEBUG='\033[0;37m[DEBUG]'
NC='\033[0m'

SCRIPT_NAME=$(basename "$0")
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

mkdir -p "$SCRIPT_DIR/tmp/"

# Clean up
function cleanup() {
    sudo rm -rf "$SCRIPT_DIR/tmp/"
}
trap cleanup EXIT

# Install packages
echo -e "${INFO} Installing required packages"
sudo apt update
sudo apt install -y \
    gcc \
    make \
    cmake \
    build-essential \
    libboost-dev \
    libssl-dev \
    doxygen \
    graphviz \
    libcppunit-dev \
    python3-pip \
    libopencv-dev

# Install MQTT
echo -e "${INFO} Installing MQTT packages"
cd "$SCRIPT_DIR/tmp/" || exit 1
# Install mosquitto
sudo apt-add-repository -y ppa:mosquitto-dev/mosquitto-ppa
sudo apt update
sudo apt install -y mosquitto mosquitto-clients
# Install PAHO C lib
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c || exit 1
git checkout v1.3.10
cmake -Bbuild -H. -DPAHO_WITH_SSL=ON -DPAHO_ENABLE_TESTING=OFF
sudo cmake --build build/ --target install
sudo ldconfig
# Install PAHO CPP lib
cd .. || exit 1
git clone https://github.com/eclipse/paho.mqtt.cpp
cd paho.mqtt.cpp || exit 1
cmake -Bbuild -H. -DPAHO_BUILD_DOCUMENTATION=TRUE -DPAHO_BUILD_SAMPLES=TRUE \
    -DCMAKE_PREFIX_PATH=../../paho.mqtt.c/build/install/usr/local
sudo cmake --build build/ --target install
sudo ldconfig
cd "$SCRIPT_DIR" || exit 1

echo -e "${INFO} Finished installing dependencies"
