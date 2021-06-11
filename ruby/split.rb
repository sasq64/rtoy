# Sprite split and tween example

img = Image.from_file("data/face.png")
sprites = img.split(16, 4).map { |img| add_sprite(img) }
sprites.each do | s |
    s.pos = s.pos * 2 + [400,250]
    tween(3.0).target(s, :smooth_step, from_pos: Vec2.rand(1400, 900)).
    target(s, from_rot: 4 * rand(Math::PI * 2)).
    target(s, to_scale: 2.0)
end

