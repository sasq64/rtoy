
# Sprite split and tween example
img = Image.from_file("data/face.png")
sprites = img.split(20, 40).map { |img| add_sprite(img) }
scale = 4
sprites.each do | s |
    s.pos = s.pos * scale
    tween(s).seconds(5.0).fn(:ease_out_back).
      from(pos: s.pos + Vec2.rand(5000, 5000)).
      from(rotation: 4 * rand(Math::PI * 2)).
      to(scale: scale)
end

loop { vsync }
