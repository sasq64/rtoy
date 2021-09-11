ship = add_sprite(Image.from_file('data/ship.png'),
    pos: [display.width/2, display.height - 180], scale: 3)

bullet = Image.solid(2,3,Color::LIGHT_RED);

counter = 0
vsync do
    if counter % 60 == 0
        enemy = add_sprite(Image.from_file('data/enemy0.png'), scale: 3,
            pos: Vec2.rand(display.width, 0))
        tween(enemy).seconds(5.0).to(y: ship.pos.y + 300).
            fn(:out_elastic).to(x: ship.pos.x).to(rotation: Math::PI).
            when_done { remove_sprite(enemy) }
    end
    ship.x += 8 if get_key(Key::RIGHT)
    ship.x -= 8 if get_key(Key::LEFT)

    if was_pressed('z'.ord)
        bs = add_sprite(bullet, scale: 4, pos: ship.pos + [ship.width/2*3,0])
        tween(bs).to(y: -10).when_done { remove_sprite(bs) }
    end
    counter += 1
end
