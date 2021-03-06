# Sokoban for R-Toy
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
            display.console.set_tile_image(256 + i, tile)
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

    def to_spr(p)
        p * 64
    end

    def from_spr(p)
        p / 64
    end

    def init_level(level)

        con = display.console
        display.clear

        @boxes = []
        @player = nil
        @moves = 0
        @game_won = false
        p = vec2(0,0)
        level.each do |l|
            l.each do |t,spr|
                p spr
                con.put_char(p.x,p.y,t+256) unless t == 0
                if spr != 0
                    s = add_sprite(@tiles[spr])
                    s.pos = to_spr(p)
                    if spr == BOX
                        @boxes.append(s)
                    elsif spr == PLAYER
                        @player  = s
                    end
                end
                p.x += 1
            end
            p = vec2(0, p.y+1)
        end
        check_boxes()
    end

    def tile_at(pos)
        sp = to_spr(pos)
        @boxes.each { |box|
            return BOX,box if box.pos == sp
        }
        return display.console.get_tile(*pos) - 256, nil
    end

    def check_boxes(b = nil)
        @counter = 0
        (0...@boxes.size).each do |i|
            box = @boxes[i]
            pos = box.pos
            p = from_spr(pos)
            t = display.console.get_tile(p.x, p.y) - 256
            if t == GOAL
                @counter += 1
                next if b && b != box
                remove_sprite(box)
                @boxes[i] = add_sprite(@tiles[GOAL_BOX])
                @boxes[i].move(*pos)
            else
                next if b && b != box
                remove_sprite(box)
                @boxes[i] = add_sprite(@tiles[BOX])
                @boxes[i].move(*pos)
            end
        end
        @counter == @boxes.length
    end

    def update_texts()
        cv = display.canvas
        cv.clear
        cv.text(display.width - 250, 10, "Moves #{@moves}", 50)
        cv.text(display.width - 250, display.height - 60,
                "Done #{@counter}/#{@boxes.length}", 50)
        cv.text(10, 10, "Level solved!", 50) if @game_won
        cv.text(10, display.height - 60, "Level #{@current_level+1}", 50)
    end

    def initialize()

        @counter = 0
        scale(1)
        display.console.set_tile_size(64,64)
        setup_tiles()
        load_levels("data/Original.txt")
        @current_level = 0
        init_level(@levels[@current_level])
        @moves = 0
        @tween = @box_tween = nil

        delta = { Key::LEFT => [-1,0],
                  Key::RIGHT => [1,0],
                  Key::UP => [0,-1],
                  Key::DOWN => [0,1] }

        on_key do |key|
            @tween.finish if @tween
            @box_tween.finish if @box_tween
            @tween = @box_tween = nil
            init_level(@levels[@current_level]) if key == 'r'.ord
            @current_level += 1 if key == 'n'.ord
            @current_level -= 1 if key == 'p'.ord
            d = delta[key]
            next unless d
            d = vec2(*d)
            pos = from_spr(@player.pos)
            target = pos + d
            t,box = tile_at(target)
            next if t == WALL
            if t == BOX
                box_target = pos + d * 2
                t2,box2 = tile_at(box_target)
                if t2 != WALL and t2 != BOX
                    @box_tween = tween(box).seconds(0.1).to(pos: to_spr(box_target)).
                        when_done {
                            gw = check_boxes(box)
                            if !@game_won && gw
                                @current_level += 1
                            end
                            @game_won = gw
                            update_texts()
                        }
                else
                    next
                end
                @moves += 1
            end
            @tween = tween(@player).seconds(0.1).to(pos: to_spr(target))
            update_texts()
        end
    end

end

Sokoban.new
loop { Fiber.yield }

