
#pragma once
#include "gl/buffer.hpp"
#include "rlayer.hpp"

#include <mruby.h>
#include <mruby/data.h>

#include <gl/texture.hpp>
#include <pix/pix.hpp>

#include <unordered_map>

struct RImage;
struct SpriteBatch;

struct RSprite
{
    gl_wrap::ArrayBuffer<GL_STATIC_DRAW> vbo;
    float width{};
    float height{};
    float alpha = 1.0F;
    bool dirty = false;
    SpriteBatch* parent = nullptr;

    std::array<float, 8> uvs{0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    std::array<float, 16> transform{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::array<float, 2> trans = {0.0F, 0.0F};
    std::array<float, 2> scale = {1.0F, 1.0F};
    float rot = 0;

    mrb_value obj{};
    static inline RClass* rclass;
    static mrb_data_type dt;

    void update_tx(float screen_width, float screen_height);
};

struct SpriteBatch
{
    pix::Image image;
    std::shared_ptr<gl_wrap::Texture> texture;
    std::vector<RSprite*> sprites;
};

class RSprites : public RLayer
{
    std::unordered_map<GLuint, SpriteBatch> batches;
    SpriteBatch fixed_batch;
public:
    RSprite* add_sprite(RImage* image, int flags);
    void remove_sprite(RSprite* spr);

    static inline RClass* rclass;
    static mrb_data_type dt;

    RSprites(int w, int h);
    void render() override;
    void reset() override;
    void clear();
    static void reg_class(mrb_state* ruby);
};

