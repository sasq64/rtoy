#include "mrb_tools.hpp"
#include "resampler.h"

class RAudio
{
    mrb::RubyPtr audio_handler;
    static inline RAudio* default_audio = nullptr;
    uint32_t dev = 0;
    mrb_state* ruby;
    Ring<float, 16384> out_buffer;

    std::array<Resampler<16384>, 32> channels;

    void fill_audio(uint8_t* data, int len);
    void mix(size_t count);

public:
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Audio", [](mrb_state*, void* data) {}};

    void write(int channel, float* data, size_t sz);
    void set_frequency(int channel, int hz);

    size_t available() const;

    explicit RAudio(mrb_state*);
    void reset();
    static void reg_class(mrb_state* ruby);
};
