#ifndef __TETON_NETWORK_CIRCULAR_BUFFER_HPP__
#define __TETON_NETWORK_CIRCULAR_BUFFER_HPP__

#include <cstdio>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>

#include "mqtt/async_client.h"

namespace teton {
namespace network {

template <class T>
class CircularBuffer {
   private:
    std::mutex mutex_;
    std::unique_ptr<T[]> buf_;
    size_t head_ = 0;
    size_t tail_ = 0;
    const size_t max_size_;
    bool full_ = 0;

   public:
    explicit CircularBuffer(size_t size);

    void put(T item);
    T get();
    void reset();
    bool empty() const;
    bool full() const;
    size_t capacity() const;
    size_t size() const;
};

}  // namespace network
}  // namespace teton

#endif
