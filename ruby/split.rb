# Sprite split and tween example
img = Image.from_file("data/face.png")
sprites = img.split(32, 2).map { |img| add_sprite(img) }
scale = 4
sprites.each do | s |
    s.pos = s.pos * scale + [250,0]
    tween(seconds: 3.0, obj: s).
      target(func: :smooth_step, from_pos: Vec2.rand(1900, 900)).
      target(from_rot: rand(Math::PI * 2)).
      target(to_scale: scale)
end

