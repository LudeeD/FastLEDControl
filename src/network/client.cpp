#include "client.hpp"

namespace teton {
namespace network {

const auto TIMEOUT = std::chrono::seconds(5);
const int QOS = 2;
const std::string NODE_END = "node_end";
const std::string CLIENT_LOG = "[teton::network::Client]   ";

Client::Client(std::string host, std::string clientId) :
    _client(host, clientId) {
    mClientId = clientId;
    mRunning = false;
}

Client::~Client() {
    disconnect();
}

bool Client::connect() {
    if (_client.is_connected()) {
        std::cout << CLIENT_LOG << "MQTT client is already connected." << std::endl;
        return true;
    }

    // Connect to the master
    if (_client.connect()->wait_for(20000)) {
        if (!_client.is_connected()) {
            std::cerr << CLIENT_LOG << "Could not connect to MQTT master." << std::endl;
            return false;
        }

        // If we connected successfully, subscribe to the special topic
        if (!subscribe(NODE_END)) {
            std::cerr << CLIENT_LOG << "Could not subscribe to special NODE_END topic." << std::endl;
            return false;
        }

        std::cout << CLIENT_LOG << "Successfully connected to MQTT master." << std::endl;
        return true;
    }

    std::cerr << CLIENT_LOG << "Connecting to MQTT master timed out." << std::endl;
    return false;
}

bool Client::disconnect() {
    std::cout << CLIENT_LOG << "Client is disconnecting..." << std::endl;

    // Stop the threads
    if (mRunning.load()) {
        if (_client.is_connected()) {
            publish(mClientId.c_str(), NODE_END);
        }
    }

    // Wait for the threads to exit
    while (mRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    for (auto &t : mThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Disconnect the client
    if (_client.is_connected()) {
        _client.disconnect()->wait_for(5000);

        if (_client.is_connected()) {
            std::cerr << CLIENT_LOG << "Failed to disconnect from MQTT master." << std::endl;
            return false;
        }

        std::cout << CLIENT_LOG << "Client disconnected successfully." << std::endl;
        return true;
    }

    return true;
}

bool Client::subscribe(std::vector<std::string> topic) {
    for (auto const &value : topic) {
        if (!subscribe(value))
            return false;
    }

    return true;
}

bool Client::subscribe(const std::string topic) {
    // std::cout << CLIENT_LOG << "Subscribing to topic: " << topic << std::endl;

    // Check if this is the first topic we're subscibing to. If so, start
    // the consuming thread.
    if (pendingSubscriptions.size() == 0) {
        mThreads.push_back(std::thread(&Client::start, this));
    }

    if (_client.is_connected()) {
        if (_client.subscribe(topic, 0)->wait_for(2000)) {
            pendingSubscriptions[topic] = new CircularBuffer<mqtt::const_message_ptr>(5);
            std::cout << CLIENT_LOG << "Subscribed to topic: " << topic << std::endl;
            return true;
        }
    }

    std::cerr << CLIENT_LOG << "Client is not connected. Failed to subscribe to topic: " << topic << std::endl;
    return false;
}

bool Client::isConnected() {
    return _client.is_connected();
}

bool Client::publish(bool signal, std::string clientid, std::string room, std::string bed, std::string topic) {
    // create json object
    rapidjson::Document d;
    d.SetObject();
    rapidjson::Document::AllocatorType &allocator = d.GetAllocator();
    size_t sz = allocator.Size();

    std::string timestamp = getTimestamp();

    // Adding content to json
    rapidjson::Value clientVal, timestampVal, dataVal, roomVal, bedVal;
    dataVal.SetBool(signal);
    roomVal.SetString(room.c_str(), room.length(), allocator);
    bedVal.SetString(bed.c_str(), bed.length(), allocator);
    timestampVal.SetString(timestamp.c_str(), timestamp.length(), allocator);
    clientVal.SetString(clientid.c_str(), clientid.length(), allocator);

    d.AddMember("data", dataVal, allocator);
    d.AddMember("room", roomVal, allocator);
    d.AddMember("bed", bedVal, allocator);
    d.AddMember("timestamp", timestampVal, allocator);
    d.AddMember("clientId", clientVal, allocator);

    // write json to string
    rapidjson::StringBuffer strBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuffer);
    d.Accept(writer);
    const char *output = strBuffer.GetString();
    if (!publish(output, topic)) {
        std::cerr << CLIENT_LOG << "Failed to log signal to " << topic << std::endl;
        return false;
    }

    return true;
}

bool Client::publish(std::string signal, std::string clientid, std::string room, std::string bed, std::string topic) {
    // create json object
    rapidjson::Document d;
    d.SetObject();
    rapidjson::Document::AllocatorType &allocator = d.GetAllocator();
    size_t sz = allocator.Size();

    std::string timestamp = getTimestamp();

    // Adding content to json
    rapidjson::Value clientVal, timestampVal, dataVal, roomVal, bedVal;
    dataVal.SetString(signal.c_str(), signal.length(), allocator);
    roomVal.SetString(room.c_str(), room.length(), allocator);
    bedVal.SetString(bed.c_str(), bed.length(), allocator);
    timestampVal.SetString(timestamp.c_str(), timestamp.length(), allocator);
    clientVal.SetString(clientid.c_str(), clientid.length(), allocator);

    d.AddMember("data", dataVal, allocator);
    d.AddMember("room", roomVal, allocator);
    d.AddMember("bed", bedVal, allocator);
    d.AddMember("timestamp", timestampVal, allocator);
    d.AddMember("clientId", clientVal, allocator);

    // write json to string
    rapidjson::StringBuffer strBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuffer);
    d.Accept(writer);
    const char *output = strBuffer.GetString();
    if (!publish(output, topic)) {
        std::cerr << CLIENT_LOG << "Failed to log signal to " << topic << std::endl;
        return false;
    }

    return true;
}

bool Client::publish(const char *signal, std::string clientid, std::string room, std::string bed, std::string topic) {
    return publish(std::string(signal), clientid, room, bed, topic);
}

std::tuple<bool, std::string, std::string, bool, std::string, std::string> Client::getBool(const std::string topic) {
    auto buffer = getBuffer(topic);
    if (!buffer->empty()) {
        mqtt::const_message_ptr msg_ptr = buffer->get();
        std::string msg = msg_ptr->to_string();
        rapidjson::Document d;
        d.Parse(msg.c_str());
        if (d.HasMember("data") &&
            d.HasMember("room") &&
            d.HasMember("bed") &&
            d.HasMember("timestamp") &&
            d.HasMember("clientId")) {
            std::string clientId(d["clientId"].GetString(), d["clientId"].GetStringLength());
            std::string timestamp(d["timestamp"].GetString(), d["timestamp"].GetStringLength());
            std::string room(d["room"].GetString(), d["room"].GetStringLength());
            std::string bed(d["bed"].GetString(), d["bed"].GetStringLength());

            if (d["data"].IsBool()) {
                bool ret(d["data"].GetBool());
                return make_tuple(true, room, bed, ret, clientId, timestamp);
            }
        }
    }

    return std::make_tuple(false, "", "", false, "", "");
}

std::tuple<bool, std::string, std::string, std::string, std::string, std::string> Client::getString(const std::string topic) {
    auto buffer = getBuffer(topic);
    if (!buffer->empty()) {
        mqtt::const_message_ptr msg_ptr = buffer->get();
        std::string msg = msg_ptr->to_string();
        rapidjson::Document d;
        d.Parse(msg.c_str());
        if (d.HasMember("data") &&
            d.HasMember("room") &&
            d.HasMember("bed") &&
            d.HasMember("timestamp") &&
            d.HasMember("clientId")) {
            std::string clientId(d["clientId"].GetString(), d["clientId"].GetStringLength());
            std::string timestamp(d["timestamp"].GetString(), d["timestamp"].GetStringLength());
            std::string room(d["room"].GetString(), d["room"].GetStringLength());
            std::string bed(d["bed"].GetString(), d["bed"].GetStringLength());

            if (d["data"].IsString()) {
                std::string ret(d["data"].GetString(), d["data"].GetStringLength());
                return make_tuple(true, room, bed, ret, clientId, timestamp);
            }
        }
    }

    return std::make_tuple(false, "", "", "", "", "");
}

/******************************************
 * PRIVATE MEMBERS
 * ***************************************/

void Client::start() {
    if (!_client.is_connected()) {
        std::cerr << CLIENT_LOG << "Called start() but the client is not connected." << std::endl;
        return;
    }

    _client.start_consuming();
    mRunning = true;

    while (_client.is_connected()) {
        mqtt::const_message_ptr msg;
        if (_client.try_consume_message_for(&msg, std::chrono::seconds(2))) {
            // Check if the message we received is the special stop signal
            if (msg->get_topic() == NODE_END && msg->get_payload().c_str() == mClientId) {
                std::cout << CLIENT_LOG << "Received NODE_END - stopping consuming thread...." << std::endl;
                break;
            }
            // Otherwise, process the message
            else {
                if (msg->get_payload() != "INIT") {
                    if (!putMessage(msg)) {
                        std::cerr << CLIENT_LOG << "Failed to process incoming message in topic: " << msg->get_topic() << std::endl;
                    }
                }
            }
        }
    }

    // Stop consuming messages
    _client.stop_consuming();
    mRunning = false;
}

bool Client::putMessage(mqtt::const_message_ptr input) {
    const std::lock_guard<std::mutex> lock(mMutexBuffer);
    const std::string topic = input->get_topic();
    if (pendingSubscriptions.size() > 0) {
        // Find the buffer for this topic
        auto it = pendingSubscriptions.find(input->get_topic());
        if (it != pendingSubscriptions.end()) {
            it->second->put(input);
            return true;
        }

        // Buffer doesn't exist for this topic
        // std::cerr << CLIENT_LOG << "No buffer is set up for topic: " << input->get_topic() << std::endl;
        return false;
    }

    // std::cerr << CLIENT_LOG << "No buffers are set up for the client. (topic: " << input->get_topic() << ")" << std::endl;
    return false;
}

CircularBuffer<mqtt::const_message_ptr> *Client::getBuffer(const std::string topic) {
    const std::lock_guard<std::mutex> lock(mMutexBuffer);
    if (pendingSubscriptions.size() > 0) {
        auto it = pendingSubscriptions.find(topic);
        if (it != pendingSubscriptions.end()) {
            return it->second;
        }
    }

    // std::cerr << "No buffer is available for topic: " << topic << std::endl;
    return new CircularBuffer<mqtt::const_message_ptr>(5);
}

bool Client::publish(const char *pubMsg, std::string topic) {
    // Publish message over MQTT
    mqtt::delivery_token_ptr pubtok;
    const std::lock_guard<std::mutex> lock(mMutexPublish);
    if (_client.is_connected()) {
        if (_client.publish(topic, pubMsg, strlen(pubMsg), QOS, false)->wait_for(TIMEOUT)) {
            // std::cout << CLIENT_LOG << "Successfully published message on topic: " << topic << std::endl;
            return true;
        }

        std::cerr << CLIENT_LOG << "Timed out publishing message in topic: " << topic << std::endl;
        return false;
    }

    std::cerr << CLIENT_LOG << "Client is not connected - cannot publish messages" << std::endl;
    return false;
}

}  // namespace network
}  // namespace teton
