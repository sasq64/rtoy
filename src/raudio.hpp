#pragma once

#include "system.hpp"
#include "settings.hpp"
#include "mrb_tools.hpp"
#include "ring_buffer.hpp"

struct Sound
{
    std::vector<float> data;
    float const* channel(int n) const
    {
        size_t sz = data.size() / channels;
        return &data[n * sz];
    }
    size_t frames() const { return data.size() / channels; }
    float freq = 0;
    unsigned channels = 1;
    static inline RClass* rclass;
    static mrb_data_type dt;
};

class RAudio
{
    struct Channel
    {
        std::vector<float> data;
        bool loop = false;
        float pos = 0;
        float step = 0.0F;

        static constexpr float ds = 30000;
        static constexpr float dl = 20000;

        void set(float freq, float const* ptr, size_t size)
        {
            data.resize(size);
            std::copy(ptr, ptr + size, data.begin());
            // loop = loop;
            pos = 0;
            step = freq / 44100.F;
        }
        float read()
        {
            if (step == 0) { return 0; }

            auto sz = static_cast<float>(data.size());

            auto ip = static_cast<size_t>(pos);
            auto f = data[ip];
            pos += step;
            float damp = std::clamp(1.0F - (pos - ds) / dl, 0.0F, 1.0F);

            if (pos > sz) {
                pos -= sz;
                if (!loop) { step = 0; }
            }
            // float a = pos - (float)ip;
            // f = sound.data[ip] * (1-a) + sound.data[ip+1] * a;
            return f * damp;
        }
    };

    mrb::RubyPtr audio_handler;
    mrb_state* ruby;
    System& system;
    Ring<float, 16384> out_buffer;
    std::array<Channel, 32> channels;
    int next_channel = 0;
    void mix(size_t samples_len);

public:
    static inline RAudio* default_audio = nullptr;
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Audio", [](mrb_state*, void* data) {}};

    void set_sound(
        int channel, Sound const& sound, float freq = 0, bool loop = false);
    void set_frequency(int channel, int hz);

    void update();

    explicit RAudio(mrb_state* ruby, System& _system, Settings const& settings);
    static void reg_class(mrb_state* ruby, System& system, Settings const& settings);
};
