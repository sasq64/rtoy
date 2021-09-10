#console.enabled = false
# Sprite split and tween example
img = Image.from_file("data/ball.png")
sprites = img.split(3, 4).map { |img| add_sprite(img) }
scale = 10.0
sprites.each do | s |
    s.pos = s.pos * scale
    p "#{s.pos.x} #{s.pos.y}"
    tween(s).seconds(5.0).fn(:out_back).
      from(pos: s.pos + Vec2.rand(5000, 5000)).
      from(rotation: 4 * rand(Math::PI * 2)).
      to(scale: scale)
end

loop { vsync }
