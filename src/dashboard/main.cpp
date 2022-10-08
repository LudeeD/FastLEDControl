#include "../network/client.hpp"
#include <boost/thread/thread.hpp>
#include <iostream>
#include <signal.h>
#include <future>

// Generated header with information from CMake
#include "version_config.h"

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

  if (!nice)
    return -1;

  // MQTT client connection setup
  std::string clientId =
      "FastLEDControl_" + tetonRoomNoStr + "_" + tetonBedNoStr;

  teton::network::Client client("localhost:1883", clientId);

  if (!client.connect()) {
    std::string errorString = "Room " + tetonRoomNoStr + " Bed " +
                              tetonBedNoStr +
                              " Failed to connect to local MQTT master";
    std::cerr << errorString << std::endl;
    return -1;
  }

  client.subscribe("local/signal/led");

  std::thread readInput(input_handler);

  while (!sigInterrupt) {
    auto [something, room, bed, ret, clientId, timestamp] =
        client.getBool("local/signal/led");
    if (something) {
      std::cout << clientId << " " << ret << "\n";
    }
    if (sendUpdate) {
      std::cout << "Sending update request" << "\n";
      sendUpdate = false;
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));
  }

  std::cin.putback('\n');

  readInput.join();
}