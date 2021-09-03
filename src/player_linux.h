#pragma once

#include <alsa/asoundlib.h>
#include <linux/soundcard.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

class sound_exception : public std::exception
{
public:
    explicit sound_exception(std::string m = "Sound") : msg(std::move(m)) {}
    const char* what() const noexcept override { return msg.c_str(); }

private:
    std::string msg;
};

class LinuxPlayer
{
    int hz;
    std::function<void(int16_t*, size_t)> callback;
    std::atomic<bool> quit{false};
    snd_pcm_t* playback_handle = nullptr;
    std::atomic<bool> paused{false};
    std::thread player_thread;

public:
    explicit LinuxPlayer(int hz = 44100) : hz(hz)
    {
        player_thread = std::thread{&LinuxPlayer::run, this};
    };

    void play(std::function<void(int16_t*, size_t)> const& cb)
    {
        callback = cb;
    }

    void pause(bool on) { paused = on; }

    ~LinuxPlayer()
    {
        quit = true;
        paused = false;
        if (player_thread.joinable()) { player_thread.join(); }
        if (playback_handle != nullptr) { snd_pcm_close(playback_handle); }
        snd_config_update_free_global();
    }

    void run()
    {
        if (int err = snd_pcm_open(
                &playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
            err < 0) {
            fprintf(
                stderr, "cannot open audio device (%s)\n", snd_strerror(err));
            throw sound_exception("");
        }
        if (int err = snd_pcm_set_params(playback_handle, SND_PCM_FORMAT_S16,
                SND_PCM_ACCESS_RW_INTERLEAVED, 2, hz, 1, 30000);
            err < 0) {
            fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
            throw sound_exception("");
        }

        std::vector<int16_t> buffer(8192*8);
        while (!quit) {
            if (paused) {
                std::this_thread::sleep_for(10ms);
            } else {
                if (callback) {
                    callback(&buffer[0], buffer.size());
                } else {
                    memset(&buffer[0], 0, buffer.size() * 2);
                }
                write_audio(&buffer[0], buffer.size());
            }
        }
        if (playback_handle != nullptr) { snd_pcm_close(playback_handle); }
        playback_handle = nullptr;
    }

    void write_audio(int16_t* samples, size_t sample_count)
    {
        auto frames = snd_pcm_writei(playback_handle,
            reinterpret_cast<char*>(samples), sample_count / 2);
        if (frames < 0) {
            snd_pcm_recover(playback_handle, static_cast<int>(frames), 0);
        }
    }

    static void set_volume(int volume)
    {
        long min = 0;
        long max = 0;
        snd_mixer_t* handle = nullptr;
        snd_mixer_selem_id_t* sid = nullptr;
        const char* card = "default";
        // const char *selem_name = "Master";

        if (snd_mixer_open(&handle, 0) < 0) {
            throw sound_exception("mixer_open");
        }
        if (snd_mixer_attach(handle, card) < 0) {
            throw sound_exception("mixer attach");
        }
        if (snd_mixer_selem_register(handle, nullptr, nullptr) < 0) {
            throw sound_exception("selem register");
        }
        if (snd_mixer_load(handle) < 0) { throw sound_exception("load"); }

        snd_mixer_selem_id_alloca(&sid); // NOLINT
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, "PCM");
        snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
        if (elem != nullptr) {
            snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
            double v = 1.0 - volume * 0.01;
            v = (1.0 - v * v * v);
            int dbvol = static_cast<int>(v * 7500) - 7500;
            snd_mixer_selem_set_playback_volume_all(elem, dbvol);
        }

        snd_mixer_close(handle);
    }
};
