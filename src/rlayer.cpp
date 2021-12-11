
#include "rlayer.hpp"

#include "gl/gl.hpp"
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>

#include "mrb/mrb_tools.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

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

    auto low_x = scissor[0];
    auto low_y = scissor[3];
    auto w = width - (scissor[0] + scissor[2]);
    auto h = height - (scissor[1] + scissor[3]);
    glScissor(low_x, low_y, w, h);
}

void RLayer::reg_class(mrb_state* ruby)
{
    RStyle::ruby = ruby;
    mrb::make_noinit_class<RLayer>(ruby, "Layer");
    mrb::set_deleter<RLayer>(ruby, [](mrb_state*, void*) {});
    mrb::make_class<RStyle>(ruby, "Style");

    mrb::attr_accessor<&RStyle::fg>(ruby, "fg");
    mrb::attr_accessor<&RStyle::bg>(ruby, "bg");
    mrb::attr_accessor<&RStyle::line_width>(ruby, "line_width");

    blend_sym = mrb::Symbol{ruby, "blend"};
    add_sym = mrb::Symbol{ruby, "add"};

    mrb::add_method<RStyle>(
        ruby, "blend_mode=", [](RStyle* style, mrb::Symbol s) {
        return style->blend_mode;
            if (s == blend_sym) {
                style->blend_mode = BlendMode::Blend;
            } else if (s == add_sym) {
                style->blend_mode = BlendMode::Add;
            } else {
                throw std::exception();
            }
        });
    mrb::add_method<RStyle>(
        ruby, "blend_mode", [](RStyle* style, mrb_state* mrb) {
            return style->blend_mode == BlendMode::Blend ? blend_sym : add_sym;
        });

    mrb::add_method<RLayer>(
        ruby, "style", [](RLayer* l) -> mrb_value { return l->stylep; });

    mrb::attr_reader<&RLayer::width>(ruby, "width");
    mrb::attr_reader<&RLayer::height>(ruby, "height");
    mrb::attr_accessor<&RLayer::enabled>(ruby, "enabled");
    mrb::attr_accessor<&RLayer::scissor>(ruby, "border");
    mrb::attr_accessor<&RLayer::scale>(ruby, "scale");
    mrb::attr_accessor<&RLayer::trans>(ruby, "offset");
    mrb::attr_accessor<&RLayer::rot>(ruby, "rotation");
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
