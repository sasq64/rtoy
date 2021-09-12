
#pragma once
#include "gl/buffer.hpp"
#include "gl/program.hpp"
#include "rlayer.hpp"

#include <mruby.h>
#include <mruby/data.h>

#include <gl/texture.hpp>
#include <pix/pix.hpp>

#include <unordered_map>

class RImage;
struct SpriteBatch;

enum class Collide
{
    None,
    Point
};

class RSprite
{
public:
    gl_wrap::ArrayBuffer<GL_STATIC_DRAW> vbo;
    float alpha = 1.0F;
    bool dirty = false;
    // Held by either native or ruby. Whoever tries to destroy it first
    // sets it to false. If it is already false, free it.
    bool held = true;


    gl_wrap::TexRef texture{};
    std::array<float, 16> transform{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::array<float, 2> trans = {0.0F, 0.0F};
    std::array<float, 2> scale = {1.0F, 1.0F};
    float rot = 0;

    int width = 0;
    int height = 0;

    static inline RClass* rclass;
    static mrb_data_type dt;

    void update_tx(double screen_width, double screen_height);
};

struct Collider
{
    RSprite* sprite = nullptr;
    Collide collide = Collide::None;
    float r2 = 0;
};

struct SpriteBatch
{
    std::shared_ptr<gl_wrap::Texture> texture;
    std::vector<RSprite*> sprites;
};

class RSprites : public RLayer
{
    std::vector<Collider> colliders;
    std::unordered_map<GLuint, SpriteBatch> batches;
    SpriteBatch fixed_batch;
    gl_wrap::Program program;
    void purge();
    void collide();
public:
    RSprite* add_sprite(RImage* image, int flags);
    static void remove_sprite(RSprite* spr);

    static inline RClass* rclass;
    static mrb_data_type dt;

    RSprites(int w, int h);
    void render() override;
    void reset() override;
    void clear();
    static void reg_class(mrb_state* ruby);
};

