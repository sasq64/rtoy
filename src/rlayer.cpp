
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
    m = glm::rotate(m, rot, glm::vec3(0.0, 0.0, 1.0));
    m = glm::translate(m, glm::vec3(-1.0, 1.0, 0));
    m = glm::scale(m, glm::vec3(2.0 / width, 2.0 / height, 1.0));
    m = glm::translate(m, glm::vec3(trans[0], - trans[1], 0));
    m = glm::scale(
        m, glm::vec3(static_cast<float>(width) * scale[0] / 2,
               static_cast<float>(height) * scale[1] / 2, 1.0));
    m = glm::translate(m, glm::vec3(1.0, -1.0, 0));
    memcpy(transform.data(), glm::value_ptr(m), 16 * 4);
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
        ruby, RLayer::rclass, "size",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rlayer = mrb::self_to<RLayer>(self);
            mrb_value values[2] = {mrb::to_value(rlayer->width, mrb),
                mrb::to_value(rlayer->height, mrb)};
            return mrb_ary_new_from_values(mrb, 2, values);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RLayer::rclass, "fg=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [fg] = mrb::get_args<uint32_t>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->style.fg = fg;
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
            auto [x, y] = mrb::get_args<float, float>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->scale = {static_cast<float>(x), static_cast<float>(y)};
            rlayer->update_tx();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));
    mrb_define_method(
        ruby, RLayer::rclass, "offset",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y] = mrb::get_args<float, float>(mrb);
            auto* rlayer = mrb::self_to<RLayer>(self);
            rlayer->trans = {static_cast<float>(x), static_cast<float>(y)};
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
    update_tx();
}
