require 'tween.rb'
require 'vec2.rb'

require 'meth_attrs.rb'
require 'color.rb'
require 'music.rb'

require 'extend.rb'
require 'shortcuts.rb'
require 'doc.rb'

module OS

    extend MethAttrs
    include Shortcuts
    include Doc

    @@display = Display.default
    @@boot_fiber = nil

    Color.constants.each do |c|
        const_set(c, Color.const_get(c))
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

    doc! "Add a handler that is called each time screen refreshes" 
    def on_draw(&block)
        @@handlers.add(:draw, block)
    end

    doc! "Add a handler that is called when a key is pressed"
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

    module_function :on_draw, :on_key, :on_drag, :on_click, :on_timer, :vec2, :remove_handler

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
            Display.default.bench_start(1);
            @@handlers.call_all(:draw, t.seconds)
            Display.default.bench_end(1)
            @@read_key = {}
        end
        Input.default.on_key do |key,mod,dev|
            @@read_key[dev] = key
            @@handlers.call_all(:key, key, mod, dev)
        end
        Input.default.on_drag { |*args| @@handlers.call_all(:drag, *args) }
        Input.default.on_click { |*args| @@handlers.call_all(:click, *args) }
        p "HANDLERS DONE"
    end

    doc! "Print text to screen without linefeed"
    def print(text, **kwargs)
        x,y = @@display.console.get_xy
        @@display.console.print(text, **kwargs)
        x2,y2 = @@display.console.get_xy
        # Yield if we line wrapped
        Fiber.yield if !@@handlers.in_callbacks && (y != y2 || x2 < x)
    end

    doc! "Print text to screen with linefeed"
    def println(text, **kwargs)
        @@display.console.print(text + "\n", **kwargs)
        Fiber.yield if !@@handlers.in_callbacks
    end

    doc! "Wait for vsync, or provide a block that will be called every vsync"
    def vsync()
        raise "Can't vsync() in callback handlers" if @@handlers.in_callbacks
        if block_given?
            loop { yield ; Fiber.yield }
        else
            Fiber.yield
        end
    end

    module_function :display, :console, :canvas, :sprite_field, :audio,
        :text, :line, :scale, :offset, :add_sprite,
        :remove_sprite, :clear, :get_char, :circle

    @@cwd = "."

    doc! "\"Boot\" the given code by running it in the 'root_fiber'. Should be called by the boot script."
    def boot(src = nil, &block)
        #raise "Already booted" if @@boot_fiber
        if src
            @@boot_fiber = Fiber.new { eval(src) }
        else
            @@boot_fiber = Fiber.new(&block)
        end
        @@boot_fiber.resume
    end

    doc! "Run a ruby file. Optionally clear handlers before starting"
    def run(name, clear: true)
        raise "Can't run() in callback handlers" if @@handlers.in_callbacks
        handlers = OS.get_handlers
        run_it = true
        while run_it
            run_it = false
            OS.clear_handlers if clear
            t = file_time(name)
            f = Fiber.new do
                src = File.read(name)
                p "LOAD #{name}"
                m = Module.new
                m.send(:extend, OS)
                m.instance_eval(src, name, 1)
            end
            while f.alive? do
                # TODO: This doesnt work anymore. block_given? is true
                # but yield breaks
                #break if block_given? and yield
                f.resume
                mods = Input.default.get_modifiers()
                if Input.default.is_pressed('c'.ord)
                    break if mods & 0xc0 != 0
                end
                # if t != file_time(name)
                #     run_it = true
                #     break
                # end
                Fiber.yield if f.alive?
            end
        end
        OS.set_handlers(handlers)
    end

    def reset_display
        scale(2,2)
        @con.offset = [0,0]
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

    doc! "Check if given key was pressed this frame"
    def was_pressed(key, dev = -1)
        key == @@read_key[dev] & 0xffffff
    end

    doc! "Wait for a key press and return it"
    def read_key(dev = 0)
        if !@@read_key[dev]
            Fiber.yield until @@read_key
        end
        r = @@read_key & 0xffffff
        @@read_key = nil
        r
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

    def plot(range = (-1.0..1.0), &block)
        ox, oy = nil, nil
        w = display.width.to_f
        (0..w).each do |x|
            y = yield (x.to_f/w) * (range.last - range.first) + range.first
            y = display.height - ((y + 1) * display.height/2)
            line(ox,oy, x, y) if ox
            ox,oy = x,y
        end
    end

    def sleep(n)
        raise "Can't sleep() in callback handlers" if @@handlers.in_callbacks
        t = Timer.default.seconds + n
        while Timer.default.seconds < t ; Fiber.yield ; end
    end

    def help(what = nil)
        if what == 'tutorial'
            ed = Editor.new
            ed.load('ruby/help.rb')
            ed.run_editor
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

    module_function :gets, :readln, :help, :run, :show, :ls,
        :sleep, :boot, :vsync, :read_key, :was_pressed

end

p "OS DONE"
