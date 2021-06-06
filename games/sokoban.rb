

class Sokoban

    include OS

    BOX = 9
    PLAYER = 13*4
    GOAL = 13*7+11
    FLOOR = 13*6+11
    WALL = 6*13+9
    GOAL_BOX = 9+4*13

    def setup_tiles
        img = Image.from_file('data/sokoban_tilesheet.png')
        w,h = img.width / 64, img.height / 64
        @tiles = img.split(w, h)
        @tiles.each_with_index do |tile, i|
            display.console.add_tile(256 + i, tile)
        end
    end

    def load_levels(fn)

        block_map = {
            '#' => [WALL,0],
            ' ' => [FLOOR,0],
            '$' => [FLOOR, BOX],
            '.' => [GOAL,0],
            '*' => [GOAL,BOX],
            '@' => [FLOOR,PLAYER],
            '+' => [GOAL,PLAYER]
        }

        @levels = []

        level = []
        lines = File.open(fn).readlines
        lines.each do |line|
            if line[0] == ';'
                @levels.append(level)
                level = []
                next
            end
            t = line.chomp.chars.map { |c| block_map[c] }
            level.append(t)
        end
    end

    def init_level(level)

        con = display.console
        display.clear

        @boxes = []
        @player = nil
        x,y = 0,0
        level.each do |l|
            l.each do |t,spr|
                p spr
                con.put_char(x,y,t+256) unless t == 0
                if spr != 0
                    s = add_sprite(@tiles[spr])
                    s.move(x*64+32, y*64+32)
                    if spr == BOX
                        @boxes.append(s)
                    elsif spr == PLAYER
                        @player  = s
                    end
                end
                x += 1
            end
            x = 0
            y += 1
        end
        check_boxes()
    end

    def tile_at(pos)
        px,py = pos[0]*64+32,pos[1]*64+32
        @boxes.each { |box|
            return BOX,box if box.pos == [px,py]
        }
        return display.console.get_tile(*pos) - 256, nil
    end

    def check_boxes()
        counter = 0
        @boxes.each do |box|
            x,y = (box.x-32)/64,(box.y-32)/64
            t = display.console.get_tile(x,y) - 256
            if t == GOAL
                box.img = @tiles[GOAL_BOX]
                counter += 1
            else
                box.img = @tiles[BOX]
            end
        end
        counter == @boxes.length
    end

    def initialize()

        scale(1)
        display.console.tile_size(64,64)
        setup_tiles()
        load_levels("data/Original.txt")
        @current_level = 0
        init_level(@levels[@current_level])
        @moves = 0

        on_key do |key|
            if key == Key::ENTER
                init_level(@levels[@current_level])
                next
            end
            if key == Key::F1
                @current_level += 1
                init_level(@levels[@current_level])
                next
            end
            sx,sy = @player.x,@player.y
            x,y = [(sx-32)/64, (sy-32)/64]
            dx,dy = 0,0
            case key
            when Key::LEFT
                dx = -1
            when Key::RIGHT
                dx = 1
            when Key::UP
                dy = -1
            when Key::DOWN
                dy = 1
            end
            target = [x + dx, y + dy]
            starget = [sx+dx*64,sy+dy*64]
            t,box = tile_at(target)
            next if t == WALL
            if t == BOX
                t2,box2 = tile_at([x + dx * 2, y + dy * 2])
                if t2 != WALL and t2 != BOX
                    box.move((x+dx*2)*64+32, (y+dy*2)*64+32)
                    if check_boxes()
                        display.canvas.text(5,5,"Success!", 50)
                    end
                else
                    next
                end
            end
            @moves += 1
            @player.move(*starget)
            display.canvas.clear
            display.canvas.text(display.width - 250, 10, "Moves #{@moves}", 50)
        end
    end

end

Sokoban.new
loop { Fiber.yield }

