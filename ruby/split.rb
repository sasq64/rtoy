#console.enabled = false
# Sprite split and tween example
console.enabled = false
img = Image.from_file("data/face.png")
sprites = img.split(64, 32).map { |img| add_sprite(img) }
scale = 4.0
sprites.each do | s |
    s.pos = s.pos * scale
    tween(s).seconds(5.0).fn(:noise).
      from(pos: s.pos + Vec2.rand(50, 50)).
      #from(rotation: 4 * rand(Math::PI * 2)).
      to(scale: scale)
end

loop { vsync }
