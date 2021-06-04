# Sprite split and tween example
#
img = Image.from_file("data/face.png")

sprites = img.split(32, 2).map { |img| add_sprite(img) }
saved = sprites.map { |s| s.pos }

sprites.each_with_index do | sprite, i |
    tween(sprite, seconds: 1.0)
    .target(method: :move, from: sprite.pos, to: [rand(1400), rand(900)])
    .target(method: :rotation=, from: 0, to: Math::PI * 2)
    .when_done {
        tween(seconds: 4.0).target(sprite, :move, from: sprite.pos, to: saved[i])
    }
end

