
#include "rlayer.hpp"

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>

#include "mrb_tools.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

void RLayer::update_tx()
{
    glm::mat4x4 m(1.0F);
    // Matrix operations are read bottom to top

    // 6. Apply rotation
    m = glm::rotate(m, rot, glm::vec3(0.0, 0.0, 1.0));

    // 5. Change center back so we rotate around middle of layer
    m = glm::translate(m, glm::vec3(-1.0, 1.0, 0));

    // 4. Scale back to clip space (-1 -> 1)
    m = glm::scale(m, glm::vec3(2.0 / width, 2.0 / height, 1.0));

    // 3. Translate
    m = glm::translate(m, glm::vec3(trans[0], -trans[1], 0));

    // 2. Scale to screen space and apply scale (origin is now to top left corner).
    m = glm::scale(m, glm::vec3(static_cast<float>(width) * scale[0] / 2,
                          static_cast<float>(height) * scale[1] / 2, 1.0));

    // 1. Change center so 0,0 becomes the corner
    m = glm::translate(m, glm::vec3(1.0, -1.0, 0));

    //  1
    //  ^   Clip space
    //  |
    //  |    0
    //  |
    //  +-------->1 
    // -1
    //
    std::copy(glm::value_ptr(m), glm::value_ptr(m) + 16, transform.begin());
}

void RLayer::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Layer", ruby->object_class);
    MRB_SET_INSTANCE_TT(RLayer::rclass, MRB_TT_DATA);

    mrb_define_method(
        ruby, RLayer::rclass, "width",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->width, mrb);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, RLayer::rclass, "height",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->height, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "bg=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->style.bg = mrb::to_array<float, 4>(av, mrb);
            rlayer->update_tx();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RLayer::rclass, "bg",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->style.bg, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "fg=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->style.fg = mrb::to_array<float, 4>(av, mrb);
            rlayer->update_tx();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RLayer::rclass, "fg",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->style.fg, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "blend_mode=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            mrb_sym sym{};
            mrb_get_args(mrb, "n", &sym);
            std::string s{mrb_sym_name(mrb, sym)};
            fmt::print("{}\n", s);
            if (s == "blend") {
                rlayer->style.blend_mode = BlendMode::Blend;
            } else if (s == "add") {
                rlayer->style.blend_mode = BlendMode::Add;
            } else {
                throw std::exception();
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RLayer::rclass, "blend_mode",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            mrb_sym sym = (rlayer->style.blend_mode == BlendMode::Blend)
                              ? mrb_intern_lit(mrb, "blend")
                              : mrb_intern_lit(mrb, "add");
            return mrb_symbol_value(sym);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "line_width=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [lw] = mrb::get_args<float>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->style.line_width = lw;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RLayer::rclass, "line_width",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->style.line_width, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "scale",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->scale, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "scale=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            fmt::print("SET_SCALE\n");
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->scale = mrb::to_array<float, 2>(av, mrb);
            rlayer->update_tx();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RLayer::rclass, "offset",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->trans, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "offset=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->trans = mrb::to_array<float, 2>(av, mrb);
            rlayer->update_tx();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));

    mrb_define_method(
        ruby, RLayer::rclass, "rotation=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x] = mrb::get_args<float>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->rot = static_cast<float>(x);
            rlayer->update_tx();
            return mrb::to_value(rlayer->rot, mrb);
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RLayer::rclass, "rotation",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->rot, mrb);
        },
        MRB_ARGS_NONE());
}

void RLayer::reset()
{
    transform = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    trans = {0.0F, 0.0F};
    scale = {1.0F, 1.0F};
    rot = 0;
    update_tx();
}
