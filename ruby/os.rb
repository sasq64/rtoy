
require 'meth_attrs.rb'
require 'tween.rb'
require 'vec2.rb'

class Color

    BLACK = [0.0, 0.0, 0.0, 1.0]
    WHITE = [1.0, 1.0, 1.0, 1.0]
    RED = [0.533, 0.0, 0.0, 1.0]
    CYAN = [0.667, 1.0, 0.933, 1.0]
    PURPLE = [0.8, 0.267, 0.8, 1.0]
    GREEN = [0.0, 0.8, 0.333, 1.0]
    BLUE = [0.0, 0.0, 0.667, 1.0]
    YELLOW = [0.933, 0.933, 0.467, 1.0]
    ORANGE = [0.867, 0.533, 0.333, 1.0]
    BROWN = [0.4, 0.267, 0.0, 1.0]
    LIGHT_RED = [1.0, 0.467, 0.467, 1.0]
    DARK_GREY = [0.2, 0.2, 0.2, 1.0]
    GREY = [0.467, 0.467, 0.467, 1.0]
    LIGHT_GREEN = [0.667, 1.0, 0.4, 1.0]
    LIGHT_BLUE = [0.0, 0.533, 1.0, 1.0]
    LIGHT_GREY = [0.733, 0.733, 0.733, 1.0]

    def data() @data ; end
    def initialize(r,g=nil,b=nil,a=nil)
        if g.nil?
            @data = [((r>>24) & 0xff)/255.0, ((r>>16) & 0xff)/255.0, 
                     ((r>>8) & 0xff)/255.0, (r & 0xff)/255.0]
        else
            a = 0xff if a.nil?
            @data = [r,g,b,a]
        end
    end
    def to_i
        ((@data[0].clamp(0,1) * 255).to_i << 24) |
            ((@data[1].clamp(0,1) * 255).to_i << 16) |
            ((@data[2].clamp(0,1) * 255).to_i << 8) |
            (@data[3].clamp(0,1) * 255).to_i
    end

    def+(col)
        d = col.data
        Color.new(@data[0] + d[0], @data[1] + d[1], @data[2] + d[2], @data[3] + d[3])
    end

    def-(col)
        d = col.data
        Color.new(@data[0] + d[0], @data[1] + d[1], @data[2] + d[2], @data[3] + d[3])
    end

    def/(n)
        Color.new(@data[0] / n, @data[1] / n, @data[2] / n, @data[3] / n)
    end

    def*(n)
        Color.new(@data[0] * n, @data[1] * n, @data[2] * n, @data[3] * n)
    end
end

class Sprites
    extend MethAttrs
    class_doc! "Sprite layer"
    doc! "Create and display new sprite from a given `Image`", :add_sprite
    returns! Sprite, :add_sprite
end

class Canvas
    extend MethAttrs
    returns! Font, :font
end

class Font
    extend MethAttrs
    returns! Image, :render
end

class Sprite
    extend MethAttrs

    def pos=(v)
        a = v.to_a
        move(a[0], a[1])
    end
    def pos()
        Vec2.new(x, y)
    end

    returns! Image,:img
end

# class Layer
#     def scale=(v)
#         a = v.to_a
#         p a
#         set_scale(a[0], a[1])
#     end
#     def scale()
#         Vec2.new(*get_scale())
#     end
# end

class Display
    extend MethAttrs
    def size
        Vec2.new(width, height)
    end

    alias set_bg bg=
    alias get_bg bg

    # def bg=(col)
    #     set_bg(col.to_i)
    # end
    # def bg
    #     Color.new(get_bg())
    # end

    returns! Canvas, :canvas
    returns! Sprites, :sprites
    returns! Console, :console
end

class Audio
    extend MethAttrs
    returns! Sound, :load_wav

    A4 = 440;
    A = (-4..4).map { |i| A4 * (2**i) }
    puts A

    keys = [ :c, :cs, :d, :ds, :e, :f, :fs, :g, :gs, :a, :as, :b ]

    ALL_KEYS = {}

    (0..8).each do |oct|
        notes = (-9..3).map { |i| A[oct] * (2**(i/12.0)) }
        (0..11).each do |note|
            ALL_KEYS[(keys[note].to_s + oct.to_s).to_sym] = notes[note]
        end
    end

end

module OS

    extend MethAttrs

    @@display = Display.default
    @@boot_fiber = nil

    def vec2(x,y)
        Vec2.new(x,y)
    end

    class Handlers
        def initialize()
            @callbacks = { key:[], draw:[], drag:[], click:[], timer:[] }
        end

        def empty?(what)
            @callbacks[what].empty?
        end

        def add(what, cb)
            index = @callbacks[what].index(:nil?)
            if index
                @callbacks[what][index] = cb
            else
                index = @callbacks[what].size
                @callbacks[what].append(cb)
            end
            index
        end

        def remove(what, index)
            @callbacks[what][index] = nil
        end


        def call_all(what, *args)
            @calling = true;
            @callbacks[what].each do |p|
                p.call(*args) if p
            end
            @calling = false;
        end

        def in_callbacks
            @calling
        end

    end

    @@handlers = Handlers.new

    doc! "Handler that is called each time screen refreshes" 
    def on_draw(&block)
        @@handlers.add(:draw, block)
    end

    def on_key(&block)
        @@handlers.add(:key, block)
    end

    def on_drag(&block)
        @@handlers.add(:drag, block)
    end

    def on_click(&block)
        @@handlers.add(:click, block)
    end
    
    def on_timer(t, &block)
        Timer.default.on_timer(t);
        @@handlers.add(:timer, block)
    end

    def remove_handler(what, i)
        @@handlers.remove(what, i)
    end

    def self.clear_handlers
        @@handlers = Handlers.new
    end

    def self.get_handlers
        @@handlers
    end

    def self.set_handlers(h)
        @@handlers = h
    end

    module_function :on_draw, :on_key, :on_drag, :on_click, :on_timer, :vec2

    @@key_queue = []
    def self.reset_handlers
        p "HANDLERS"
        @@handlers = Handlers.new
        Tween.clear()

        t = Timer.default
        t.on_timer(1000) { |ms| @@handlers.call_all(:timer, ms) }
        Display.default.on_draw do
            if @@boot_fiber
                if @@boot_fiber.alive?
                    @@boot_fiber.resume
                else
                    # TODO: QUIT
                end
            end
            Tween.update_all(t.seconds)
            @@handlers.call_all(:draw, t.seconds)
        end
        Input.default.on_key do |key,mod|
            @@read_key = key
            if @@handlers.empty?(:key) 
                @@key_queue.append(key)
            else
                @@handlers.call_all(:key, key, mod)
                @@key_queue = []
            end
        end
        Input.default.on_drag { |*args| @@handlers.call_all(:drag, *args) }
        Input.default.on_click { |*args| @@handlers.call_all(:click, *args) }
        p "HANDLERS DONE"
    end

    returns! Display
    def display() @@display end

    returns! Console
    def console() @@display.console end

    returns! Canvas
    def canvas() @@display.canvas end

    returns! Sprites
    def sprites() @@display.sprites end

    returns! Audio
    def audio() Audio.default end

    returns! Speech
    def speech() Speech.default end

    def say(text)
        sound = Speech.default.text_to_sound(text)
        audio.play(sound)
    end
    def play(sound, freq = nil)
        audio.play(sound)
    end

    returns! Image
    takes_file! 
    def load_image(*args) Image.from_file(*args) end

    def print(text)
        x,y = @@display.console.get_xy
        @@display.console.print(text)
        x2,y2 = @@display.console.get_xy
        Fiber.yield if !@@handlers.in_callbacks && (y != y2 || x2 < x)
    end

    def println(text)
        @@display.console.print(text + "\n")
        Fiber.yield if !@@handlers.in_callbacks
    end

    def text(*args) @@display.console.text(*args) end
    def line(x, y, x2, y2) @@display.canvas.line(x, y, x2, y2) end
    def circle(x, y, r) @@display.canvas.circle(x, y, r) end
    def get_char(x, y) @@display.console.get_char(x, y) end
    def scale(x, y = x) @@display.console.scale = [x,y] end
    def offset(x, y) @@display.console.offset(x,y) end
    
    returns! Sprite
    def add_sprite(img) @@display.sprites.add_sprite(img) end
    def remove_sprite(spr) @@display.sprites.remove_sprite(spr) end
    def clear()
        @@display.clear
    end

    def flush()
        raise "Can't flush() in callback handlers" if @@handlers.in_callbacks
        Fiber.yield
    end

    def vsync()
        raise "Can't vsync() in callback handlers" if @@handlers.in_callbacks
        Fiber.yield
    end

    module_function :display, :console, :canvas, :sprites, :audio,
        :text, :line, :scale, :offset, :add_sprite,
        :remove_sprite, :clear, :get_char, :circle

    @@cwd = "."

    def boot(src = nil, &block)
        #raise "Already booted" if @@boot_fiber
        if src
            @@boot_fiber = Fiber.new { eval(src) }
        else
            @@boot_fiber = Fiber.new(&block)
        end
        @@boot_fiber.resume
    end

    def exec(src = nil, &block)
        raise "Can't exec() in callback handlers" if @@handlers.in_callbacks
        block = Proc.new { eval(src) } if src
        f = Fiber.new(&block)
        while f.alive? do
            f.resume
            Fiber.yield if f.alive?
        end
    end

    def run(name, clear: true)
        raise "Can't run() in callback handlers" if @@handlers.in_callbacks
        handlers = OS.get_handlers
        OS.clear_handlers if clear
        f = Fiber.new do
            src = File.read(name)
            p "LOAD #{name}"
            m = Module.new
            m.send(:extend, OS)
            m.instance_eval(src, name, 1)
        end
        while f.alive? do
            break if block_given? and yield
            f.resume
            mods = Input.default.get_modifiers()
            if Input.default.get_key('c'.ord)
                break if mods & 0xc0 != 0
            end
            Fiber.yield if f.alive?
        end
        OS.set_handlers(handlers)
    end

    def reset_display
        scale(2,2)
        @con.set_offset(0,0)
        display.console.fg = Color::WHITE
        display.bg = Color::BLUE
    end

    def ls(d = nil)
        d ||= @@cwd
        list_files(d).each { |f| puts f }
    end

    def gets
        raise "Can't gets() in callback handlers" if @@handlers.in_callbacks
        line = LineReader.read_line
        puts
        line
    end

    alias readln gets

    def read_key()
        @@read_key = nil
        Fiber.yield until @@read_key
        @@read_key
    end

    def show(fn)
        img = Image.from_file(fn)
        if !img
            puts "Failed to load image '#{fn}'"
        end
        Display.default.clear
        Display.default.canvas.draw(0, 0, img)
        Display.default.console.goto_xy(0, img.height/32 + 1)
        img
    end

    def cat(fn)
        text = File.read(fn)
        puts text
    end

    def sleep(n)
        raise "Can't sleep() in callback handlers" if @@handlers.in_callbacks
        n.times { Fiber.yield }
    end

    def help(what = nil)
        if what == 'tutorial'
            ed = Editor.new
            ed.load('ruby/help.rb')
            ed.activate
        elsif what == 'intro'
            cat('data/intro.txt')
        elsif what == 'design'
            cat('data/design.txt')
        elsif what == 'games'
            puts "ls \"games\""
            ls('games')
            puts <<GAMES
You can run a game (or any ruby file for that matter) by
typing `run "path-to-game" `. You can also open up the
file in the editor first, using edit "path-to-game"  and
then pressing `F5` to run it.
GAMES
        else puts <<HELP
help 'intro'
help 'tutorial'
help 'games'
help 'design'
HELP
        end
    end

    module_function :gets, :readln, :help, :run, :show, :ls, :exec,
        :sleep, :exec, :boot, :vsync, :flush, :read_key

end

p "OS DONE"
