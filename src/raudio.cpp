#include "raudio.hpp"

#define DR_WAV_IMPLEMENTATION
#include <dr_libs/dr_wav.h>

RAudio::RAudio(mrb_state*, System& _system, Settings const& settings)
    : system{_system}
{
    system.init_audio(settings);
    system.set_audio_callback([this](float* data, size_t size) {
        while (out_buffer.available() < size) {
            mix(2048);
        }
        out_buffer.read(data, size);
    });
}

void RAudio::update()
{
    while (out_buffer.available() < 2048) {
        mix(2048 / 2);
    }
}

// Pull 'count' samples from all channels into out buffer
void RAudio::mix(size_t samples_len)
{
    // fmt::print("Mix {}\n", samples_len);
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

void RAudio::set_sound(int channel, Sound const& sound, float freq, bool loop)
{
    if (sound.data.empty()) { return; }
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

void RAudio::reg_class(
    mrb_state* ruby, System& system, Settings const& settings)
{
    mrb::make_noinit_class<RAudio>(ruby, "Audio");
    mrb::set_deleter<RAudio>(ruby, [](mrb_state*, void*) {});
    mrb::make_class<Sound>(ruby, "Sound");

    default_audio = new RAudio(ruby, system, settings);

    mrb::attr_reader<&Sound::channels>(ruby, "channels");
    mrb::attr_reader<&Sound::freq>(ruby, "freq");

    mrb::add_method<RAudio>(ruby, "play",
        [](RAudio* self, mrb::ArgN n, Sound* sound, int chan, float freq) {
            if (n == 1) { chan = self->next_channel; }
            self->next_channel = (chan + 2) % 32;
            fmt::print("chan {}, freq {}\n", chan, freq);
            self->set_sound(chan, *sound, static_cast<float>(freq));
            return self;
        });

    mrb::add_class_method<RAudio>(
        ruby, "default", [] { return RAudio::default_audio; });

    mrb::add_method<RAudio>(ruby, "on_audio",
        [](RAudio* self, mrb::Block block) { self->audio_handler = block; });

    mrb::add_method<RAudio>(
        ruby, "set_freq", [](RAudio* self, int chan, float freq) {
            self->channels[chan].step = static_cast<float>(freq) / 44100.F;
        });

    mrb::add_class_method<RAudio>(
        ruby, "load_wav", [](std::string const& fname) {
            unsigned channel_count = 0;
            unsigned freq = 0;
            drwav_uint64 frames = 0;
            Sound* sound = nullptr;
            float* sample_data = drwav_open_file_and_read_pcm_frames_f32(
                fname.c_str(), &channel_count, &freq, &frames, nullptr);
            if (sample_data == nullptr) {
                // Error opening and reading WAV file.
                return sound;
            }
            fmt::print("Channels {}, freq {} frames {}\n", channel_count, freq,
                frames);
            sound = new Sound();
            sound->freq = static_cast<float>(freq);
            sound->channels = static_cast<int>(channel_count);
            sound->data.resize(frames * channel_count);
            for (size_t i = 0; i < frames; i++) {
                for (size_t j = 0; j < channel_count; j++) {
                    sound->data[j * frames + i] =
                        sample_data[i * channel_count + j];
                }
            }
            drwav_free(sample_data, nullptr);
            return sound;
        });
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
