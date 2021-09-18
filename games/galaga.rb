ship = add_sprite(Image.from_file('data/ship.png'),
    pos: [display.width/2, display.height - 180], scale: 3)
ship.collider = 3


bullet = Image.solid(2,3,Color::LIGHT_RED);
eimg = Image.from_file('data/enemy0.png')

def split_sprite(spr)
    z = vec2(spr.img.width / 2, spr.img.height / 2)
    rc = spr.img.split(4,4).map do |i|
        s = add_sprite(i)
        pos = (s.pos - z).rotate(spr.rotation) + z
        s.rotation = spr.rotation
        s.scale = spr.scale
        s.pos = pos * spr.scale + spr.pos
        s
    end
    remove_sprite(spr)
    rc
end


sprites.on_collision(1,2) do |b,e|
    p "Collision!"
    split_sprite(e).each do |s|
        tween(s).seconds(1.0).to(alpha: 0).delta(pos: (Vec2.rand(100,100) - [50,50]) / 10.0).
            when_done { remove_sprite(s) }
    end
    remove_sprite(e)
end

sprites.on_collision(2,3) do |e,ship|
    p "Collision!"
    split_sprite(ship).each do |s|
        tween(s).seconds(1.0).delta(pos: (Vec2.rand(100,100) - [50,50]) / 10.0).
            when_done { remove_sprite(s) }
    end
    remove_sprite(ship)
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
    ship.x += 8 if is_pressed(Key::RIGHT)
    ship.x -= 8 if is_pressed(Key::LEFT)

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

