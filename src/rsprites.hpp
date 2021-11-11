
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

struct Collider
{
    uint32_t group = 0;
    float width = 0;
    float height = 0;
    float radius = 0;
};

class RSprite
{
public:
    gl_wrap::ArrayBuffer<GL_STATIC_DRAW> vbo;
    std::array<float, 4> color{1, 1, 1, 1};
    bool dirty = false;
    // Held by either native or ruby. Whoever tries to destroy it first
    // sets it to false. If it is already false, free it.
    bool held = true;

    Collider* collider = nullptr;

    gl_wrap::TexRef texture{};
    std::array<float, 16> transform{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::array<float, 2> trans = {0.0F, 0.0F};
    std::array<float, 2> scale = {1.0F, 1.0F};
    float rot = 0;
    mrb_value value{};

    static inline RClass* rclass;
    static mrb_data_type dt;

    ~RSprite() { delete collider; }

    void update_tx(double screen_width, double screen_height);
    void update_collision() const;
};

struct CollisionGroup
{
    uint32_t from = 0;
    uint32_t to = 0;
    mrb::RubyPtr handler;
};

struct SpriteBatch
{
    std::shared_ptr<gl_wrap::Texture> texture;
    std::vector<RSprite*> sprites;
};

class RSprites : public RLayer
{
    std::vector<CollisionGroup> groups;
    std::unordered_map<uint32_t, std::vector<RSprite*>> colliders;
    std::unordered_map<GLuint, SpriteBatch> batches;
    SpriteBatch fixed_batch;
    SpriteBatch particle_batch;
    gl_wrap::Program program;
    mrb_state* ruby;
    void purge();
    void collide();
    bool draw_batch(SpriteBatch& batch);

    int tx_location = -1;
    int col_location = -1;
    gl_wrap::Attribute pos;
    gl_wrap::Attribute uv;

public:
    RSprite* add_sprite(RImage* image, int flags);
    RSprite* add_particle(int size, uint32_t color);
    static void remove_sprite(RSprite* spr);

    static inline RClass* rclass;
    static mrb_data_type dt;

    RSprites(mrb_state* _ruby, int w, int h);
    void render(RLayer const* parent) override;
    void reset() override;
    void clear() override;
    static void reg_class(mrb_state* ruby);
};

