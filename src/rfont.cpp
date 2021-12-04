#include "rfont.hpp"

#include "mrb/mrb_tools.hpp"
#include "rimage.hpp"

mrb_data_type RFont::dt{"Font", [](mrb_state*, void* data) {
                            fmt::print("Deleting image\n");
                            delete static_cast<RFont*>(data);
                        }};
RFont::RFont(std::string const& fname) : font(fname.c_str()) {}

RImage* RFont::render(std::string const& txt, uint32_t color, int n)
{
    font.set_pixel_size(n);
    auto [w, h] = font.text_size(txt);
    pix::Image img(w, h);
    font.render_text(txt, reinterpret_cast<uint32_t*>(img.ptr), color,
        img.width, img.width, img.height);
    auto* image = new RImage(img);
    return image;
}

void RFont::reg_class(mrb_state* ruby)
{
    rclass = mrb::make_noinit_class<RFont>(ruby, "Font");
    //rclass = mrb_define_class(ruby, "Font", ruby->object_class);
    //MRB_SET_INSTANCE_TT(RFont::rclass, MRB_TT_DATA);

    mrb_define_class_method(
        ruby, RFont::rclass, "from_file",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            const char* name{};
            mrb_get_args(mrb, "z", &name);
            return mrb::new_data_obj(mrb, new RFont(name));
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RFont::rclass, "get_size",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto argc = mrb_get_argc(mrb);
            auto [txt, n] = mrb::get_args<std::string, int>(mrb);
            auto* rfont = mrb::self_to<RFont>(self);
            rfont->font.set_pixel_size(n);
            auto [w, h] = rfont->font.text_size(txt);
            std::array<int, 2> a{w, h};
            return mrb::to_value(a, mrb);
        },
        MRB_ARGS_REQ(2));
    mrb_define_method(
        ruby, RFont::rclass, "render",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto argc = mrb_get_argc(mrb);
            if (argc == 2) {
                auto [txt, n] = mrb::get_args<std::string, int>(mrb);
                auto* rfont = mrb::self_to<RFont>(self);
                return mrb::new_data_obj(mrb, rfont->render(txt, 0xffffff00, n));
            }
            auto [txt, col, n] =
                mrb::get_args<std::string, mrb_value, int>(mrb);
            auto col_a = mrb::to_array<float, 4>(col, mrb);
            auto col_u = gl::Color(col_a).to_bgra();
            fmt::print("Text {} color {:x}\n", txt, col_u);
            auto* rfont = mrb::self_to<RFont>(self);
            return mrb::new_data_obj(mrb, rfont->render(txt, col_u, n));
        },
        MRB_ARGS_REQ(2));
}

