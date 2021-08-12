#pragma once

#include <libresample.h>

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

template <int SIZE>
struct Resampler
{
    float targetHz;
    float inputHz;
    bool active = false;
    void* r;
    Ring<float, SIZE> fifo;
    explicit Resampler(float hz = 22050) : targetHz(hz), inputHz(hz)
    {
        r = resample_open(1, 0.5, 40.0);
        set_hz(hz);
    }

    void set_hz(float hz)
    {
        active = hz != targetHz;
        inputHz = hz;
    }

    void write(float* samples, size_t size)
    {
        auto factor = targetHz / inputHz;
        /* if (factor == 1.0F) { */
        /*     fifo.write(samples, size); */
        /*     return; */
        /* } */

        int used = 0;
        fmt::print("Factor {}\n", factor);
        std::array<float, 16384> target; // NOLINT
        int count = resample_process(r, factor, samples, static_cast<int>(size),
            0, &used, target.data(), target.size());
        fmt::print("{} {}\n", count, used);
        fifo.write(target.data(), count);
    }

    size_t add(float* target, size_t n) { return fifo.add(target, n); }

    size_t read(float* target, size_t n) { return fifo.read(target, n); }
};
