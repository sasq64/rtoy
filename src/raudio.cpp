#include "raudio.hpp"

#include <SDL2/SDL_audio.h>

RAudio::RAudio(mrb_state* _ruby) : ruby{_ruby}
{
    SDL_AudioSpec want;
    SDL_AudioSpec have;

    SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 4096;
    want.userdata = this;
    want.callback = [](void* userdata, Uint8* stream, int len) {
        static_cast<RAudio*>(userdata)->fill_audio(stream, len);
    };

    dev = SDL_OpenAudioDevice(
        nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    fmt::print("Audio format {} {} vs {}\n", have.format, have.freq, dev);
    SDL_PauseAudioDevice(dev, 0);
}

// Pull 'count' samples from all channels into out buffer
void RAudio::mix(size_t samples_len)
{
    std::array<float, 8192 * 2> temp{};
    fmt::print("Mix {} samples\n", samples_len);
    int i = 0;
    for (auto& chan : channels) {
        chan.add(temp.data() + i, samples_len);
        i = i == 0 ? 8192 : 0;
    }
    out_buffer.interleave(temp.data(), temp.data() + 8192, samples_len);
}

void RAudio::fill_audio(uint8_t* data, int bytes_len)
{
    auto floats_len = bytes_len / 4;
    fmt::print("Fill {}\n", bytes_len);
    while (out_buffer.available() < floats_len) {
        mix(bytes_len / 8);
    }
    auto count = out_buffer.read(reinterpret_cast<float*>(data), floats_len);
    fmt::print("Read {} floats\n", count);
    // if (count < len) { memset(data + len - count, 0, len - count); }
}

void RAudio::write(int channel, float* data, size_t sz)
{
    // if (channel >= channels.size()) { channels.resize(channel + 1); }
    channels[channel].write(data, sz);
}

void RAudio::set_frequency(int channel, int hz)
{
    // if (channel >= channels.size()) { channels.resize(channel + 1); }
    channels[channel].set_hz(static_cast<float>(hz));
}

void RAudio::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Audio", nullptr);

    mrb_define_method(
        ruby, RAudio::rclass, "write",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [chan, a] = mrb::get_args<int, mrb_value>(mrb);
            auto sz = ARY_LEN(mrb_ary_ptr(a));
            std::vector<float> data(sz);
            for (int i = 0; i < sz; i++) {
                auto v = mrb_ary_entry(a, i);
                data[i] = mrb::to<float>(v);
            }
            auto* audio = mrb::self_to<RAudio>(self);
            audio->write(chan, data.data(), data.size());
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));

    mrb_define_class_method(
        ruby, rclass, "default",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            if (default_audio == nullptr) { default_audio = new RAudio(mrb); }
            return mrb::new_data_obj(mrb, default_audio);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, rclass, "on_audio",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* audio = mrb::self_to<RAudio>(self);
            mrb_int n = -1;
            mrb_value blk;
            mrb_get_args(mrb, "&", &blk);
            if (!mrb_nil_p(blk)) {
                audio->audio_handler = mrb::RubyPtr{mrb, blk};
            }
            return mrb_nil_value();
        },
        MRB_ARGS_BLOCK());
}
