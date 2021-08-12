
# Sprite split and tween example
img = Image.from_file("data/face.png")
sprites = img.split(20, 2).map { |img| add_sprite(img) }
scale = 4
sprites.each do | s |
    s.pos = s.pos * scale + [100,0]
    tween(s).seconds(3.0).fn(:smooth_step).
      from(pos: Vec2.rand(1900, 900)).
      from(rotation: rand(Math::PI * 2)).
      to(scale: scale)
end

loop { vsync }
