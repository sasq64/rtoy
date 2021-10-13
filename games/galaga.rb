class Galaga
    include OS

    def initialize()

        @players = {}

        @ship = Image.from_file('data/ship.png')
        @bullet = Image.solid(2, 3, Color::LIGHT_RED)
        @enemy = Image.from_file('data/enemy2.png')
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

        on_key { |key, mod, dev|
            unless @players.include? dev
                p "NEW PLAYER"
                @players[dev] = add_sprite(@ship,
                    pos: [display.width/4, display.height - 180],
                    collider: :ship,
                    scale: 3)
            end
        }

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

    def spawn_enemy(pos)
        enemy = add_sprite(@enemy,
            scale: 3,
            collider: :enemy,
            pos: Vec2.rand(display.width/2, 0))

        tween(enemy).seconds(5.0).to(y: pos.y).
            fn(:out_elastic).to(x: pos.x).to(rotation: Math::PI).
            when_done { remove_sprite(enemy) }
    end

    def update()
        @counter += 1
        pos = vec2(rand(display.width), display.height + 300)
        spawn_enemy(pos) if @counter % 20 == 0
        @players.each do |dev,spr|
            spr.x += 8 if is_pressed(Key::RIGHT, dev)
            spr.x -= 8 if is_pressed(Key::LEFT, dev)
            if was_pressed('z'.ord, dev) || was_pressed(Key::FIRE, dev)
                bs = add_sprite(@bullet, collider: :bullet, scale: 4,
                    pos: spr.pos)
                tween(bs).seconds(10).delta(pos: vec2(0,-4)).
                    when_done { remove_sprite(bs) }
            end
        end

    end
    
end

game = Galaga.new
vsync { game.update }
