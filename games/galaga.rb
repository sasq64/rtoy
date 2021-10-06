class Galaga
    include OS

    def initialize()
        @ship = add_sprite(Image.from_file('data/ship.png'),
            pos: [display.width/2, display.height - 180],
            collider: :ship,
            scale: 3)

        @bullet = Image.solid(2, 3, Color::LIGHT_RED);
        @eimg = Image.from_file('data/enemy1.png')
        @counter = 0

        sprite_field.on_collision(:bullet, :enemy) do |b,e|
            explode_sprite(e)
            remove_sprite(b)
        end

        sprite_field.on_collision(:enemy, :ship) do |e,ship|
            explode_sprite(ship)
        end

        display.bg = Color::BLACK
        console.enabled = false

    end

    def split_sprite(spr)
        z = spr.img.size / 2
        spr.img.split(4,4).map do |img|
            s = add_sprite(img,
                rotation: spr.rotation,
                scale: spr.scale)
            pos = (s.pos - z).rotate(spr.rotation) + z
            s.pos = pos * spr.scale + spr.pos
            s
        end
    end

    def explode_sprite(spr)
        split_sprite(spr).each do |s|
            tween(s).seconds(1.0).to(alpha: 0).
                delta(pos: (Vec2.rand(100,100) - [50,50]) / 10.0).
                when_done { remove_sprite(s) }
        end
        remove_sprite(spr)
    end

    def spawn_enemy()
        enemy = add_sprite(@eimg,
            scale: 3,
            collider: :enemy,
            pos: Vec2.rand(display.width, 0))

        tween(enemy).seconds(5.0).to(y: @ship.pos.y + 300).
            fn(:out_elastic).to(x: @ship.pos.x).to(rotation: Math::PI).
            when_done { remove_sprite(enemy) }
    end

    def update()
        @counter += 1
        spawn_enemy() if @counter % 90 == 0
        @ship.x += 8 if is_pressed(Key::RIGHT)
        @ship.x -= 8 if is_pressed(Key::LEFT)

        if was_pressed('z'.ord) || was_pressed(Key::FIRE)
            bs = add_sprite(@bullet, collider: :bullet, scale: 4,
                pos: @ship.pos)
            tween(bs).seconds(10).delta(pos: vec2(0,-4)).
                when_done { remove_sprite(bs) }
        end
    end
    
end

game = Galaga.new
vsync { game.update }
