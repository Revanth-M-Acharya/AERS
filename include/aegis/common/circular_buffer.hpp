/**
 * @file circular_buffer.hpp
 * @brief Fixed-capacity, zero-heap-allocation circular ring buffer.
 */

#ifndef AEGIS_COMMON_CIRCULAR_BUFFER_HPP
#define AEGIS_COMMON_CIRCULAR_BUFFER_HPP

#include <array>
#include <cstddef>
#include <cassert>

namespace aegis {
namespace common {

template <typename T, size_t Capacity>
class CircularBuffer {
    static_assert(Capacity > 0, "CircularBuffer capacity must be greater than zero");
public:
    CircularBuffer() : m_head(0), m_tail(0), m_size(0) {}

    bool Push(const T& item) {
        if (IsFull()) {
            return false;
        }
        m_data[m_tail] = item;
        m_tail = (m_tail + 1) % Capacity;
        ++m_size;
        return true;
    }

    void PushOverwrite(const T& item) {
        m_data[m_tail] = item;
        m_tail = (m_tail + 1) % Capacity;
        if (IsFull()) {
            m_head = (m_head + 1) % Capacity;
        } else {
            ++m_size;
        }
    }

    bool Pop(T& item) {
        if (IsEmpty()) {
            return false;
        }
        item = m_data[m_head];
        m_head = (m_head + 1) % Capacity;
        --m_size;
        return true;
    }

    const T* Peek() const noexcept {
        if (IsEmpty()) {
            return nullptr;
        }
        return &m_data[m_head];
    }

    bool IsFull() const noexcept { return m_size == Capacity; }
    bool IsEmpty() const noexcept { return m_size == 0; }
    size_t Size() const noexcept { return m_size; }
    constexpr size_t MaxCapacity() const noexcept { return Capacity; }

    void Clear() noexcept {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

private:
    std::array<T, Capacity> m_data;
    size_t m_head{0};
    size_t m_tail{0};
    size_t m_size{0};
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_CIRCULAR_BUFFER_HPP
