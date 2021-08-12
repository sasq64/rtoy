#include "raudio.hpp"

#include <SDL2/SDL_audio.h>

#define DR_WAV_IMPLEMENTATION
#include <dr_libs/dr_wav.h>

mrb_data_type Sound::dt{
    "Sound", [](mrb_state*, void* ptr) { delete static_cast<Sound*>(ptr); }};

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

    dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have,
        SDL_AUDIO_ALLOW_ANY_CHANGE & (~SDL_AUDIO_ALLOW_FORMAT_CHANGE));

    fmt::print("Audio format {} {} vs {}\n", have.format, have.freq, dev);
    SDL_PauseAudioDevice(dev, 0);
}

// Pull 'count' samples from all channels into out buffer
void RAudio::mix(size_t samples_len)
{
    std::array<float, 8192> temp[2]{{}, {}}; // NOLINT
    int ear = 0;
    for (auto& chan : channels) {
        auto& t = temp[ear];
        ear ^= 1;
        for (size_t i = 0; i < samples_len; i++) {
            t[i] += chan.read();
        }
    }
    out_buffer.interleave(temp[0].data(), temp[1].data(), samples_len);
}

// Called from audio system to read samples
void RAudio::fill_audio(uint8_t* data, int bytes_len)
{
    auto floats_len = bytes_len / 4;
    while (out_buffer.available() < floats_len) {
        mix(bytes_len / 8);
    }
    out_buffer.read(reinterpret_cast<float*>(data), floats_len);
}

void RAudio::set_sound(int channel, Sound const& sound, float freq, bool loop)
{
    // Assume sample is C4 = 261.63 Hz
    if (freq == 0) { freq = 261.63F; }
    freq = sound.freq * (freq / 261.63F);
    for (int i = 0; i < sound.channels; i++) {
        auto& chan = channels[(channel + i) % 32];
        chan.set(freq, sound.channel(i), sound.frames());
        chan.loop = loop;
    }
}

void RAudio::set_frequency(int channel, int hz)
{
    auto& chan = channels[channel];
    chan.step = static_cast<float>(hz) / 44100.F;
}

void RAudio::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Audio", ruby->object_class);
    Sound::rclass = mrb_define_class(ruby, "Sound", ruby->object_class);
    MRB_SET_INSTANCE_TT(RAudio::rclass, MRB_TT_DATA);
    MRB_SET_INSTANCE_TT(Sound::rclass, MRB_TT_DATA);

    mrb_define_method(
        ruby, Sound::rclass, "channels",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            return mrb::to_value(mrb::self_to<Sound>(self)->channels, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Sound::rclass, "freq",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            return mrb::to_value(mrb::self_to<Sound>(self)->freq, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RAudio::rclass, "play",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* audio = mrb::self_to<RAudio>(self);
            Sound* sound{};
            mrb_float freq = 0;
            mrb_int chan = 0;
            auto n = mrb_get_argc(mrb);
            if (n == 3) {
                mrb_get_args(mrb, "idf", &chan, &sound, &Sound::dt, &freq);
            } else if (n == 2) {
                mrb_get_args(mrb, "id", &chan, &sound, &Sound::dt);
            } else if (n == 1) {
                mrb_get_args(mrb, "d", &sound, &Sound::dt);
                chan = audio->next_channel;
                audio->next_channel = (audio->next_channel + 2) % 32;
            }
            fmt::print("chan {}, freq {}\n", chan, freq);
            audio->set_sound(chan, *sound, static_cast<float>(freq));
            return self;
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

    mrb_define_method(
        ruby, rclass, "set_freq",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [chan, freq] = mrb::get_args<int, int>(mrb);
            auto* audio = mrb::self_to<RAudio>(self);
            audio->channels[chan].step = static_cast<float>(freq) / 44100.F;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));

    mrb_define_class_method(
        ruby, RAudio::rclass, "load_wav",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [fname] = mrb::get_args<std::string>(mrb);
            unsigned channel_count = 0;
            unsigned freq = 0;
            drwav_uint64 frames = 0;
            float* sample_data = drwav_open_file_and_read_pcm_frames_f32(
                fname.c_str(), &channel_count, &freq, &frames, nullptr);
            if (sample_data == nullptr) {
                // Error opening and reading WAV file.
                return mrb_nil_value();
            }
            fmt::print("Channels {}, freq {} frames {}\n", channel_count, freq,
                frames);

            auto* sound = new Sound();
            sound->freq = static_cast<float>(freq);
            sound->channels = channel_count;
            sound->data.resize(frames * channel_count);
            for (size_t i = 0; i < frames; i++) {
                for (size_t j = 0; j < channel_count; j++) {
                    sound->data[j * frames + i] = sample_data[i * 2 + j];
                }
            }
            drwav_free(sample_data, nullptr);
            return mrb::new_data_obj(mrb, sound);
        },
        MRB_ARGS_REQ(1));
}

#ifdef MUSIC
RAUdio::play_music(std::string const& name)
{
    using namespace musix;

    ChipPlugin::createPlugins("data");

    std::shared_ptr<ChipPlayer> player;

    for (const auto& plugin : ChipPlugin::getPlugins()) {
        if (plugin->canHandle(name)) {
            if (auto* ptr = plugin->fromFile(name)) {
                player = std::shared_ptr<ChipPlayer>(ptr);
                pluginName = plugin->name();
                break;
            }
        }
    }
}
#endif
