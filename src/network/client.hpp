#ifndef __TETON_NETWORK_MQTT_CLIENT_HPP__
#define __TETON_NETWORK_MQTT_CLIENT_HPP__

#include <mutex>
#include <thread>
#include <string>
#include <chrono>
#include <atomic>
#include <fstream>
#include <iostream>

#include "Base64.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#include "mqtt/async_client.h"
#include <boost/date_time/posix_time/posix_time.hpp>

#include "../utils/utils.hpp"
#include "circular_buffer.hpp"

namespace teton {
namespace network {

class Client {
   public:
    Client(std::string host, std::string clientId);
    ~Client();

    bool connect();
    bool disconnect();

    bool subscribe(const std::string topic);
    bool subscribe(std::vector<std::string> topic);

    inline bool empty(const std::string topic) {
        auto buffer = getBuffer(topic);
        return buffer->empty();
    }

    inline size_t size(const std::string topic) {
        auto buffer = getBuffer(topic);
        return buffer->size();
    }

    bool isConnected();

    bool publish(bool signal, std::string clientid, std::string room, std::string bed, std::string topic);
    bool publish(std::string signal, std::string clientid, std::string room, std::string bed, std::string topic);
    bool publish(const char *signal, std::string clientid, std::string room, std::string bed, std::string topic);

    std::tuple<bool, std::string, std::string, bool, std::string, std::string> getBool(const std::string topic);
    std::tuple<bool, std::string, std::string, std::string, std::string, std::string> getString(const std::string topic);

   private:
    std::string mClientId;
    std::atomic<bool> mRunning;
    std::vector<std::thread> mThreads;
    std::mutex mMutexPublish, mMutexBuffer;

    mqtt::async_client _client;
    std::map<std::string, CircularBuffer<mqtt::const_message_ptr> *> pendingSubscriptions;

    void start();
    bool putMessage(mqtt::const_message_ptr input);
    CircularBuffer<mqtt::const_message_ptr> *getBuffer(const std::string topic);
    bool publish(const char *output, std::string topic);

    inline std::string getTimestamp() const {
        boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
        std::string datetime = to_iso_extended_string(t) + "Z";
        std::cout << datetime << std::endl;
        return datetime;
    }
};

}  // namespace network
}  // namespace teton

#endif
