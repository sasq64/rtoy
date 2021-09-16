ship = add_sprite(Image.from_file('data/ship.png'),
    pos: [display.width/2, display.height - 180], scale: 3)


bullet = Image.solid(2,3,Color::LIGHT_RED);
eimg = Image.from_file('data/enemy0.png')

sprites.on_collision(1,2) do |b,e|
    p "Collision!"
    eimg.split(4,4).each do |i|
        s = add_sprite(i)
        #s.rotation = e.rotation
        s.scale = e.scale
        s.pos = s.pos * e.scale + e.pos
        remove_sprite(b)
        tween(s).seconds(1.0).delta(pos: (Vec2.rand(10,10) - [5,5]) / 1.0).when_done { remove_sprite(s) }
    end
    remove_sprite(e)
end

counter = 0
vsync do
    if counter % 90 == 0
        enemy = add_sprite(eimg, scale: 3,
            pos: Vec2.rand(display.width, 0))
        enemy.collider = 2
        tween(enemy).seconds(5.0).to(y: ship.pos.y + 300).
            fn(:out_elastic).to(x: ship.pos.x).to(rotation: Math::PI).
            when_done { remove_sprite(enemy) }
    end
    ship.x += 8 if get_key(Key::RIGHT)
    ship.x -= 8 if get_key(Key::LEFT)

    if was_pressed('z'.ord)
        bs = add_sprite(bullet, scale: 4, pos: ship.pos)
        bs.collider = 1
        tween(bs).seconds(10).delta(pos: vec2(0,-4)).
            when_done { remove_sprite(bs) }
    end
    ship.alpha = 1.0
    counter += 1
    #canvas.clear
    #x = ship.pos.x + ship.width*3/2
    #y = ship.pos.y + ship.height*3/2
    #canvas.circle(x, y, ship.height*3/2)
end

