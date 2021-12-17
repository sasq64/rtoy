console.enabled = false
display.bg = Color::BLACK
sprite = add_sprite(Image.from_file('data/ship.png'),
                    scale: 5, rotation: Math::PI/2)

bullet = Image.solid(2,2,Color::YELLOW)

on_draw do
  r = sprite.rotation - Math::PI/2
  sprite.pos += vec2(4.0, 0.0).rotate(r)
  delta = vec2(-5,0).rotate(r) + Vec2.rand(10, 10) * 0.1
  s = add_sprite(bullet, scale: 10)
  s.pos = sprite.pos + sprite.size * (3.0 / 2)
  tween(s).seconds(4).delta(pos: delta).to(alpha: 0).when_done { remove_sprite(s) }
  sprite.rotation += 0.04 if is_pressed(Key::RIGHT)
  sprite.rotation -= 0.04 if is_pressed(Key::LEFT)
end


vsync {}
