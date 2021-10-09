
#include "rlayer.hpp"

#include "gl/gl.hpp"
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>

#include "mrb_tools.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

mrb_data_type RStyle::dt{"RStyle",
    [](mrb_state*, void* data) { delete static_cast<RStyle*>(data); }};

void RLayer::update_tx(RLayer const* parent)
{
    glm::mat4x4 m(1.0F);
    // Matrix operations are read bottom to top

    // 6. Apply rotation
    m = glm::rotate(m, rot, glm::vec3(0.0, 0.0, 1.0));

    // 5. Change center back so we rotate around middle of layer
    m = glm::translate(m, glm::vec3(-1.0, 1.0, 0));

    // 4. Scale back to clip space (-1 -> 1)
    m = glm::scale(m, glm::vec3(2.0 / width, 2.0 / height, 1.0));
    float t0 = parent != nullptr ? parent->trans[0] : 0.0F;
    float t1 = parent != nullptr ? parent->trans[1] : 0.0F;
    // 3. Translate
    m = glm::translate(m, glm::vec3(trans[0] + t0, -(trans[1] + t1), 0));

    // 2. Scale to screen space and apply scale (origin is now to top left
    // corner).
    float s0 = parent != nullptr ? parent->scale[0] : 1.0F;
    float s1 = parent != nullptr ? parent->scale[1] : 1.0F;
    m = glm::scale(m, glm::vec3(static_cast<float>(width) * scale[0] * s0 / 2,
                          static_cast<float>(height) * scale[1] * s1 / 2, 1.0));

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

    auto lowerx = scissor[0];
    auto lowery = scissor[3];
    auto w = width - (scissor[0] + scissor[2]);
    auto h = height - (scissor[1] + scissor[3]);
    //fmt::print("{} {} {} {}\n", lowerx, lowery, w, h);
    glScissor(lowerx, lowery, w, h);

//    glScissor(scissor[0] + trans[0] + t0, scissor[1] + trans[1] + t1,
  //      width + t0 - scissor[2] * 2, height + t1 - scissor[3] * 2);
}

void RLayer::reg_class(mrb_state* ruby)
{
    RStyle::ruby = ruby;
    RStyle::rclass = mrb_define_class(ruby, "Style", ruby->object_class);
    MRB_SET_INSTANCE_TT(RStyle::rclass, MRB_TT_DATA);

    rclass = mrb_define_class(ruby, "Layer", ruby->object_class);
    MRB_SET_INSTANCE_TT(RLayer::rclass, MRB_TT_DATA);

    // RSTYLE
    //
    mrb_define_method(
        ruby, RStyle::rclass, "initialize",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            DATA_PTR(self) = new RStyle();                  // NOLINT
            DATA_TYPE(self) = mrb::get_data_type<RStyle>(); // NOLINT
            auto* rstyle = mrb::self_to<RStyle>(self);
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RStyle::rclass, "fg=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rstyle = mrb::self_to<RStyle>(self);
            rstyle->fg = mrb::to_array<float, 4>(av, mrb);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RStyle::rclass, "fg",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rstyle = mrb::self_to<RStyle>(self);
            return mrb::to_value(rstyle->fg, mrb);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, RStyle::rclass, "bg=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rstyle = mrb::self_to<RStyle>(self);
            rstyle->bg = mrb::to_array<float, 4>(av, mrb);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RStyle::rclass, "bg",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rstyle = mrb::self_to<RStyle>(self);
            return mrb::to_value(rstyle->bg, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RStyle::rclass, "blend_mode=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* style = mrb::self_to<RStyle>(self);
            mrb_sym sym{};
            mrb_get_args(mrb, "n", &sym);
            std::string s{mrb_sym_name(mrb, sym)};
            fmt::print("{}\n", s);
            if (s == "blend") {
                style->blend_mode = BlendMode::Blend;
            } else if (s == "add") {
                style->blend_mode = BlendMode::Add;
            } else {
                throw std::exception();
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RStyle::rclass, "blend_mode",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* style = mrb::self_to<RStyle>(self);
            mrb_sym sym = (style->blend_mode == BlendMode::Blend)
                              ? mrb_intern_lit(mrb, "blend")
                              : mrb_intern_lit(mrb, "add");
            return mrb_symbol_value(sym);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RStyle::rclass, "line_width=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [lw] = mrb::get_args<float>(mrb);
            auto* style = mrb::self_to<RStyle>(self);
            style->line_width = lw;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RStyle::rclass, "line_width",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* style = mrb::self_to<RStyle>(self);
            return mrb::to_value(style->line_width, mrb);
        },
        MRB_ARGS_NONE());

    // RLAYER
    //
    mrb_define_method(
        ruby, RLayer::rclass, "style",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return rlayer->stylep;
        },
        MRB_ARGS_NONE());

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
        ruby, RLayer::rclass, "enabled=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [e] = mrb::get_args<bool>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->enabled = e;
            rlayer->handle_enable();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RLayer::rclass, "enabled",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            return mrb::to_value(rlayer->enabled, mrb);
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
        ruby, RLayer::rclass, "border=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->scissor = mrb::to_array<int, 4>(av, mrb);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RLayer::rclass, "scale=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            fmt::print("SET_SCALE\n");
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->scale = mrb::to_array<float, 2>(av, mrb);
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
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));

    mrb_define_method(
        ruby, RLayer::rclass, "rotation=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x] = mrb::get_args<float>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->rot = static_cast<float>(x);
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
    scissor = {0, 0, 0, 0};
    current_style.fg = {1, 1, 1, 1};
    current_style.bg = {0, 0, 0, 0};
    rot = 0;
    update_tx(nullptr);
}
