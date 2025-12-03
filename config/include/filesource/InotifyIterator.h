#pragma once

#include <sys/inotify.h>
#include <iterator>
#include <cstddef>

namespace config {

/**
 * @brief Iterator for inotify_event structures in a raw buffer.
 * 
 * Encapsulates the C-style pointer arithmetic required to iterate over
 * variable-length inotify_event structures.
 */
class InotifyIterator {
public:
    using value_type = struct inotify_event;
    using pointer = const struct inotify_event*;
    using reference = const struct inotify_event&;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    InotifyIterator(const char* buffer, size_t size, size_t offset = 0)
        : m_buffer(buffer), m_size(size), m_offset(offset) {}

    reference operator*() const {
        return *reinterpret_cast<pointer>(m_buffer + m_offset);
    }

    pointer operator->() const {
        return reinterpret_cast<pointer>(m_buffer + m_offset);
    }

    InotifyIterator& operator++() {
        if (m_offset < m_size) {
            const auto* event = operator->();
            m_offset += sizeof(struct inotify_event) + event->len;
        }
        return *this;
    }

    InotifyIterator operator++(int) {
        InotifyIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool operator==(const InotifyIterator& other) const {
        // Two iterators are equal if they point to the same buffer and offset,
        // OR if both are at the end (offset >= size).
        bool this_end = m_offset >= m_size;
        bool other_end = other.m_offset >= other.m_size;
        if (this_end && other_end) return true;
        return m_buffer == other.m_buffer && m_offset == other.m_offset;
    }

    bool operator!=(const InotifyIterator& other) const {
        return !(*this == other);
    }

private:
    const char* m_buffer;
    size_t m_size;
    size_t m_offset;
};

/**
 * @brief Helper class to enable range-based for loops.
 */
class InotifyEvents {
public:
    InotifyEvents(const char* buffer, size_t size)
        : m_buffer(buffer), m_size(size) {}

    InotifyIterator begin() const {
        return InotifyIterator(m_buffer, m_size, 0);
    }

    InotifyIterator end() const {
        return InotifyIterator(m_buffer, m_size, m_size);
    }

private:
    const char* m_buffer;
    size_t m_size;
};

} // namespace config
