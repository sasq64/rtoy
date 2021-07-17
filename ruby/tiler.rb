require 'ui.rb'
require 'vec2.rb'

class TileMap
    attr_reader :image, :size
    def initialize(image, size)
        @image = image
        @size = size
        @tiles = @image.split(image.width / size, image.height / size)
        p "Split into #{@tiles.size}"
        @con = Display.default.console
        @tiles.each_with_index do |tile, i|
            @con.add_tile(1024+i, tile)
        end
    end

    def get(i)
        @tiles[i]
    end
end

class TileView < UI::Element
    attr_reader :width,:height,:tile_map

    def initialize(tile_map, pos)
        @tile_map = tile_map
        @dirty = true
        @ex,@ey = 1,1
        image = Image.from_file("data/sel.png")
        @sel = Display.default.sprites.add_sprite(image)
        @scale = 3
        @sel.scale = @scale * tile_map.size / 8

        size = tile_map.size
        @width = tile_map.image.width / size
        @height = tile_map.image.height / size
        @selected = 0
        super(pos, [tile_map.image.width * @scale,
                    tile_map.image.height * @scale])
    end

    def tile=(i)
        @ex,@ey = 1,1
        @sel.scalex = @scale * @ex *tile_map.size / 8
        @sel.scaley = @scale * @ey *tile_map.size / 8

        p "#{i} #{@width} #{@height}"
        @sel.x = (i % @width) * (@tile_map.size * @scale)
        @sel.y = (i / @width) * (@tile_map.size * @scale)
        @selected = i
    end

    def tile()
        @selected
    end

    def extend(x, y)
        @ex += x
        @ey += y
        @sel.scalex = @scale * @ex *tile_map.size / 8
        @sel.scaley = @scale * @ey *tile_map.size / 8
    end

    def each_selected(&block)
        (0...@ey).each { |y|
            (0...@ex).each { |x|
                p self.tile
                p x
                p y
                block.call(self.tile + x + y*@width)
            }
            block.call(-1)
        }
    end

    def render(ui)
        ui.canvas.draw(@pos.x, @pos.y, @tile_map.image, @scale) if @dirty
        @dirty = false
    end

    def on_click(x,y)
        p "Clicked tileview"
        x = x / (@tile_map.size * @scale)
        y = y / (@tile_map.size * @scale)
        self.tile = x + y * @width
    end
end

Display.default.console.fg = Color::WHITE
Display.default.clear()

ui = UI::Frame.new

pos = Vec2.new(0,0)

tm = TileMap.new(Image.from_file("data/cave.png"), 8)
tile_view = TileView.new(tm, pos)
ui.add_element(tile_view)

ui.on_click_or_drag do |x,y|
    con = Display.default.console
    w,h = con.get_tile_size()
    sx,sy = con.get_scale()
    ox,oy = con.get_offset()
    x = (x-ox)/(w*sx)
    y = (y-oy)/(h*sy)
    xx = 0
    tile_view.each_selected do |tile|
        if tile == -1
            xx = 0
            y += 1
            next
        end
        con.put_char(x + xx, y, tile + 1024)
        xx += 1
    end
    #con.put_char(x, y, tile_view.tile + 1024)
end

ui.input.on_key do |key,mod|
    p "#{key} #{mod}"
    con = Display.default.console
    s = con.get_scale()
    o = con.get_offset()
    sz = 8*4
    case key
    #when 0x20..0xffffff
    when Key::LEFT
        o[0] += sz
    when Key::RIGHT
        o[0] -= sz
    when Key::UP
        o[1] += sz
    when Key::DOWN
        o[1] -= sz
    when 'A'.ord
        tile_view.extend(-1,0)
    when 'W'.ord
        tile_view.extend(0,-1)
    when 'D'.ord
        tile_view.extend(1,0)
    when 'S'.ord
        tile_view.extend(0,1)
    when 'd'.ord
        tile_view.tile += 1
    when 'a'.ord
        tile_view.tile -= 1
    when 'w'.ord
        tile_view.tile -= (tile_view.width)
    when 's'.ord
        tile_view.tile += (tile_view.width)
    when '+'.ord
        con.set_scale(s[0] * 2, s[1] * 2)
    when '-'.ord
        con.set_scale(s[0] * 0.5, s[1] * 0.5)
    end
    con.set_offset(*o)
end

sz = tm.size
Display.default.bg = Color::BLACK
Display.default.console.set_tile_size(sz, sz)

#button = UI::Button.new(pos, [200,100]) { p "Clicked" }
#ui.add_element(button)

#loop {
#    OS.vsync()
#}

