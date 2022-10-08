#include "../network/client.hpp"
#include <boost/thread/thread.hpp>
#include <iostream>
#include <signal.h>
#include <future>

// Generated header with information from CMake
#include "version_config.h"

const std::string TOPIC_LED_STATUS = "local/signal/led";  // Topic for LED signal
const std::string TOPIC_LED_REQUEST_STATUS = "local/update/led";  // Topic for LED signal

volatile sig_atomic_t sigInterrupt = 0;
void sig_handler(int signo) {
  if (signo == SIGINT) {
    printf("Received SIGINT... Press ENTER to terminate\n");
    sigInterrupt = 1;
  }
  if (signo == SIGTERM) {
    printf("Received SIGTERM... Press ENTER to terminate\n");
    sigInterrupt = 1;
  }
}

std::atomic<bool> sendUpdate (false);

void input_handler() {
  std::string opt;

  while(sigInterrupt == 0) {
      getline(std::cin, opt);

      if (opt.compare("status") == 0) {
        sendUpdate = true;
      }
      if (opt.compare("exit") == 0) {
        sigInterrupt = 1;
      }
  }
}

int main(int argc, char **argv) {
  // Set up SIGINT/SIGTERM handler for graceful shutdown
  if (signal(SIGINT, sig_handler) == SIG_ERR)
    printf("Can't catch SIGINT...\n");
  if (signal(SIGTERM, sig_handler) == SIG_ERR)
    printf("Can't catch SIGTERM...\n");

  // Query static environment variables
  std::string tetonRoomNoStr;
  std::string tetonBedNoStr;
  bool nice = teton::utils::getEnvVar("TETON_BED_NO", tetonBedNoStr) &&
              teton::utils::getEnvVar("TETON_ROOM_NO", tetonRoomNoStr);

  if (!nice) {
    std::cerr
        << "\033[1;31mEnvVar TETON_BED_NO or TETON_ROOM_NO not present!\033[0m"
        << std::endl;
    return -1;
  }

  // MQTT client connection setup
  const std::string clientId_LOCAL =
      "FastLEDControl_" + tetonRoomNoStr + "_" + tetonBedNoStr;

  teton::network::Client client("localhost:1883", clientId_LOCAL);

  if (!client.connect()) {
    std::string errorString = "Room " + tetonRoomNoStr + " Bed " +
                              tetonBedNoStr +
                              " Failed to connect to local MQTT master";
    std::cerr << errorString << std::endl;
    return -1;
  }

  client.subscribe(TOPIC_LED_STATUS);

  std::thread readInput(input_handler);

  while (!sigInterrupt) {
    bool something, ret;
    std::string room, bed, clientId, timestamp;
    std::tie(something, room, bed, ret, clientId, timestamp) =
        client.getBool("local/signal/led");
    if (something) {
      std::cout << "[" << timestamp << "]"
                << "[" << clientId << "]"
                << "Room : " << room << " "
                << "Bed : " << bed   << " "
                << "Led : " << ret << std::endl;
    }
    if (sendUpdate) {
      client.publish(sendUpdate, clientId_LOCAL, tetonRoomNoStr, tetonBedNoStr,
                     TOPIC_LED_REQUEST_STATUS);
      sendUpdate = false;
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));
  }

  readInput.join();
}
