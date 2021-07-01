
#include "mrb_tools.hpp"
#include "ring_buffer.hpp"

struct Sound
{
    std::vector<float> data;
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

        float read()
        {
            if (step == 0) { return 0; }

            auto sz = static_cast<float>(data.size());
            auto ip = static_cast<size_t>(pos);
            auto f = data[ip];
            pos += step;
            if (pos > sz) {
                pos -= sz;
                if (!loop) { step = 0; }
            }
            // float a = pos - (float)ip;
            // f = sound.data[ip] * (1-a) + sound.data[ip+1] * a;
            return f;
        }
    };

    mrb::RubyPtr audio_handler;
    static inline RAudio* default_audio = nullptr;
    uint32_t dev = 0;
    mrb_state* ruby;
    uint64_t sample_count = 0;
    Ring<float, 16384> out_buffer;
    std::array<Channel, 32> channels;
    int next_channel = 0;
    void* voice;
    void fill_audio(uint8_t* data, int len);
    void mix(size_t samples_len);

public:
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Audio", [](mrb_state*, void* data) {}};

    void set_sound(int channel, Sound const& sound, bool loop = false);
    void set_frequency(int channel, int hz);

    size_t available() const;

    explicit RAudio(mrb_state*);
    void reset();
    static void reg_class(mrb_state* ruby);
};
