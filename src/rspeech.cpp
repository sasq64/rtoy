#include "rspeech.hpp"
#include "raudio.hpp"

#include <flite.h>

extern "C" cst_voice* register_cmu_us_kal(const char* voxdir);

RSpeech::RSpeech(mrb_state* mrb) : ruby{mrb}
{
    flite_init();
    voice = register_cmu_us_kal(nullptr);
}

void RSpeech::reg_class(mrb_state* ruby)
{
    mrb::make_noinit_class<RSpeech>(ruby, "Speech");
    default_speech = new RSpeech(ruby);

    mrb::add_class_method<RSpeech>(ruby, "default", [] {
        return default_speech;
    });

    mrb::add_method<RSpeech>(ruby, "text_to_sound", [](RSpeech* self, std::string const& text) {
        auto* wav = flite_text_to_wave(
            text.c_str(), static_cast<cst_voice*>(self->voice));
        fmt::print("SPEECH {} {}\n", wav->num_samples, wav->sample_rate);
        auto* sound = new Sound();
        sound->freq = static_cast<float>(wav->sample_rate);
        sound->channels = wav->num_channels;
        sound->data.resize(wav->num_samples);
        for (size_t i = 0; i < sound->data.size(); i++) {
            sound->data[i] = static_cast<float>(wav->samples[i]) / 0x7fff;
        }
        delete_wave(wav);
        return sound;
    });

//    mrb_define_class_method(
//        ruby, rclass, "default",
//        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
//            if (default_speech == nullptr) {
//                default_speech = new RSpeech(mrb);
//            }
//            return mrb::new_data_obj(mrb, default_speech);
//        },
//        MRB_ARGS_NONE());
//
//    mrb_define_method(
//        ruby, RSpeech::rclass, "text_to_sound",
//        [](mrb_state* mrb, mrb_value self) -> mrb_value {
//            auto [text] = mrb::get_args<std::string>(mrb);
//
//            auto* speech = mrb::self_to<RSpeech>(self);
//            auto* wav = flite_text_to_wave(
//                text.c_str(), static_cast<cst_voice*>(speech->voice));
//            fmt::print("SPEECH {} {}\n", wav->num_samples, wav->sample_rate);
//
//            auto* sound = new Sound();
//            sound->freq = static_cast<float>(wav->sample_rate);
//            sound->channels = wav->num_channels;
//            sound->data.resize(wav->num_samples);
//            for (size_t i = 0; i < sound->data.size(); i++) {
//                sound->data[i] = static_cast<float>(wav->samples[i]) / 0x7fff;
//            }
//            delete_wave(wav);
//            return mrb::new_data_obj(mrb, sound);
//        },
//        MRB_ARGS_REQ(1));
}
