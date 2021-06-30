#pragma once

#include <array>
#include <atomic>
#include <fmt/format.h>
#include <vector>

template <typename T, size_t SIZE>
struct Ring
{
    std::array<T, SIZE> data;
    std::atomic<size_t> read_pos{0};
    std::atomic<size_t> write_pos{0};

    size_t write(T const* source, size_t n)
    {
        auto left = SIZE - write_pos + read_pos;
        if (n > left) { n = left; }
        for (size_t i = 0; i < n; i++) {
            data[(write_pos + i) % SIZE] = source[i];
        }
        write_pos += n;
        return n;
    }

    size_t interleave(T const* source0, T const* source1, size_t samples_len)
    {
        auto floats_len = samples_len * 2;
        auto left = SIZE - write_pos + read_pos;
        if (floats_len > left) { floats_len = left; }
        for (size_t i = 0; i < samples_len; i++) {
            data[(write_pos + i * 2) % SIZE] = source0[i];
            data[(write_pos + i * 2 + 1) % SIZE] = source1[i];
        }
        write_pos += floats_len;
        return samples_len;
    }

    size_t read(T* target, size_t n)
    {
        auto left = write_pos - read_pos;
        if (left < n) { n = left; }
        for (size_t i = 0; i < n; i++) {
            target[i] = data[(read_pos + i) % SIZE];
        }
        read_pos += n;
        return n;
    }

    size_t add(T* target, size_t n)
    {
        auto left = write_pos - read_pos;
        if (left < n) { n = left; }
        for (size_t i = 0; i < n; i++) {
            target[i] += data[(read_pos + i) % SIZE];
        }
        read_pos += n;
        return n;
    }

    // Available to read
    size_t available() const { return write_pos - read_pos; }
};
