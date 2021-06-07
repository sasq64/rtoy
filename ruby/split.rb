# Sprite split and tween example
img = Image.from_file("data/face.png")
sprites = img.split(16, 4).map { |img| add_sprite(img) }
sprites.each do | s |
    tween(1.0).target(s, from_pos: Vec2.rand(1400, 900)).
    target(s, from_rot: rand(Math::PI * 2))
end

