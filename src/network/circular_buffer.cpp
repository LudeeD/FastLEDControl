#include "circular_buffer.hpp"

namespace teton {
namespace network {

using namespace cv;

template <class T>
CircularBuffer<T>::CircularBuffer(size_t size) :
    buf_(std::unique_ptr<T[]>(new T[size])),
    max_size_(size) {
    // empty constructor
}

template <class T>
void CircularBuffer<T>::reset() {
    std::lock_guard<std::mutex> lock(this->mutex_);
    head_ = tail_;
    full_ = false;
}

// State tracking
template <class T>
bool CircularBuffer<T>::empty() const {
    // if head and tail are equal, we are empty
    return (!full_ && (head_ == tail_));
}

template <class T>
bool CircularBuffer<T>::full() const {
    // if tail is ahead the head by 1, we are full
    return full_;
}

template <class T>
size_t CircularBuffer<T>::capacity() const {
    return max_size_;
}

template <class T>
size_t CircularBuffer<T>::size() const {
    size_t size = max_size_;
    if (!full_) {
        if (head_ >= tail_) {
            size = head_ - tail_;
        } else {
            size = max_size_ + head_ - tail_;
        }
    }
    return size;
}

// adding data
template <class T>
void CircularBuffer<T>::put(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    buf_[head_] = item;
    if (full_) {
        tail_ = (tail_ + 1) % max_size_;
    }
    head_ = (head_ + 1) % max_size_;
    full_ = head_ == tail_;
}

// retrieving data
template <class T>
T CircularBuffer<T>::get() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (empty()) {
        return T();
    }
    // Read data and advance the tail
    auto val = buf_[tail_];
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;
    return val;
}

template class CircularBuffer<Mat>;
template class CircularBuffer<mqtt::const_message_ptr>;

}  // namespace network
}  // namespace teton
