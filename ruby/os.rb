
require 'tween.rb'

module Ret
    @@return_types = {}
    @@arg_types = {}
    @@doc_strings = {}
    @@returns = nil
    @@doc_string = nil
    @@file_name = nil
    @@takes_file = nil
    @@class_doc = nil

    def method_added(name)
        if @@doc_string
            p "#{name}:\n#{@@doc_string}\n"
            @@doc_strings[name] = @@doc_string
            @@doc_string = nil
        end
        if @@returns
            p "#{name} returns #{@@returns}"
            @@return_types[name] = @@returns
            @@returns = nil
        end
        if @@takes_file
            @@arg_types[name] = :file
            @@takes_file = nil
        end
    end

    def doc!(doc_string, meth = nil)
        if meth
            @@doc_strings[meth] = doc_string
        else
            @@doc_string = doc_string
        end
    end

    def class_doc!(doc_string)
        @@class_doc = doc_string
    end

    def returns!(type, meth = nil)
        if meth
            @@return_types[meth] = type
        else
            @@returns = type
        end
    end

    def takes_file!
        @@file_name = true
    end

    def takes_file?(method)
        return @@arg_types[method] == :file
    end

    def get_return_type(method)
        type = @@return_types[method]
        p ">> RET for #{method} is #{type}"
        type
    end

    def get_doc(meth = nil)
        meth.nil? ? @@class_doc : @@doc_strings[meth]
    end
end

class Vec2
    extend Ret
    class_doc! "class used to represent 2D points and vectors"

    def initialize(x, y = nil)
        @data = [x,y]
    end
    def x=(x) @data[0] = x ; end
    def y=(y) @data[1] = y ; end
    def x() @data[0] end
    def y() @data[1] end
    def to_s() @data.to_s ; end
    def to_a() @data end

    def self.rand(x,y)
        Vec2.new(Kernel::rand(x), Kernel::rand(y))
    end
    
    def ==(v)
        a = v.to_a
        a[0] == @data[0] && a[1] == @data[1]
    end

    def +(v)
        a = v.to_a
        Vec2.new(@data[0] + a[0], @data[1] + a[1])
    end
    def -(v)
        a = v.to_a
        Vec2.new(@data[0] - a[0], @data[1] - a[1])
    end
    def *(s)
        Vec2.new(@data[0] * s, @data[1] * s)
    end
    def /(s)
        Vec2.new(@data[0] / s, @data[1] / s)
    end
end

class Sprites
    extend Ret
    class_doc! "Sprite layer"
    doc! "Create and display new sprite from a given `Image`", :add_sprite
    returns! Sprite, :add_sprite
end

class Canvas
    extend Ret
    returns! Font, :font
end

class Font
    extend Ret
    returns! Image, :render
end

class Sprite
    extend Ret

    def pos=(v)
        a = v.to_a
        move(a[0], a[1])
    end
    def pos()
        Vec2.new(x, y)
    end

    returns! Image,:img
end

class Layer
    def scale=(v)
        a = v.to_a
        set_scale(a[0], a[1])
    end
    def scale()
        Vec2.new(*get_scale())
    end
end

class Display
    extend Ret
    def size
        Vec2.new(width, height)
    end

    returns! Canvas, :canvas
    returns! Sprites, :sprites
    returns! Console, :console
end

class Audio
    extend Ret
    returns! Sound, :load_wav
end

module OS

    extend Ret

    @@display = Display.default
    @@boot_fiber = nil

    def vec2(x,y)
        Vec2.new(x,y)
    end

    class Handlers
        def initialize()
            @callbacks = { key:[], draw:[], drag:[], click: [], timer: [] }
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
            @@get_key = key
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
            Fiber.yield if f.alive?
        end
        OS.set_handlers(handlers)
    end

    def reset_display
        scale(2,2)
        @con.offset(0,0)
        display.console.fg = Color::WHITE
        display.bg = Color::BLUE
    end

    def ls(d = nil)
        d ||= @@cwd
        list_files(d).each { |f| puts f }
    end

    def gets
        raise "Can't gets() in callback handlers" if @@handlers.in_callbacks
        #line = IOX.read_line
        line = LineReader.read_line
        puts
        line
    end

    def get_key()
        @@get_key = nil
        Fiber.yield until @@get_key
        @@get_key
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

    module_function :gets, :help, :run, :show, :ls, :exec,
        :sleep, :exec, :boot, :vsync, :flush, :get_key

end

p "OS DONE"
