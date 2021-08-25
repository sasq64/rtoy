
# Sprite split and tween example
img = Image.from_file("data/face.png")
sprites = img.split(64, 64).map { |img| add_sprite(img) }
scale = 4
sprites.each do | s |
    s.pos = s.pos * scale + [100,0]
    tween(s).seconds(5.0).fn(:ease_out_back).
      from(pos: s.pos + Vec2.rand(50, 50)).
      from(rotation: rand(Math::PI * 2)).
      to(scale: scale)
end

loop { vsync }
