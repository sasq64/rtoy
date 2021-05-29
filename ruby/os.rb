
require 'tween.rb'

module OS

    @@display = Display.default

    class Handlers
        def initialize()
            @callbacks = { key:[], draw:[], drag:[], click: [], timer: [] }
        end

        def add(what, cb)
            index = @callbacks[what].index(:nil?)
            if index
                @callbacks[what][i] = [cb, nil]
            else
                index = @callbacks[what].size
                @callbacks[what].append([cb, nil])
            end
            index
        end

        def remove(what, index)
            @callbacks[what][index] = nil
        end


        def call_all(what, *args)
            @callbacks[what].each do |p|
                if p and p[0]
                    if p[1].nil?
                        p[1] = Fiber.new(&(p[0]))
                    end
                    p[1].resume(*args)
                    p[1] = nil unless p[1].alive?
                end
            end
        end

    end

    @@handlers = Handlers.new

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

    def self.reset_handlers
        p "HANDLERS"
        @@handlers = Handlers.new

        Tween.clear()

        t = Timer.default
        t.on_timer(1000) { |ms| @@handlers.call_all(:timer, ms) }
        Display.default.on_draw do
            if @runner
                if @runner.alive?
                    @runner.resume
                else
                    @ruller = nil
                end
            end

            Tween.update_all(t.seconds)
            @@handlers.call_all(:draw, t.seconds)
        end
        Input.default.on_key { |*args| @@handlers.call_all(:key, *args) }
        Input.default.on_drag { |*args| @@handlers.call_all(:drag, *args) }
        Input.default.on_click { |*args| @@handlers.call_all(:click, *args) }
    end

    def display() @@display end
    def load_image(*args) Image.from_file(*args) end
    def text(*args) @@display.console.text(*args) end
    def line(x, y, x2, y2) @@display.canvas.line(x, y, x2, y2) end
    def circle(x, y, r) @@display.canvas.circle(x, y, r) end
    def get_char(x, y) @@display.console.get_char(x, y) end
    def scale(x, y = x) @@display.console.scale(x,y) end
    def offset(x, y) @@display.console.offset(x,y) end
    def add_sprite(img) @@display.sprites.add_sprite(img) end
    def remove_sprite(spr) @@display.sprites.remove_sprite(spr) end
    def clear()
        Display.default.clear
    end

    @@cwd = "."

    def self.sync_run(&block)
        @runner = Fiber.new(&block)
        @runner.resume
    end

    def self.run(name)
        OS.clear_handlers()
        f = Fiber.new do
            load(name)
        end
        while f.alive? do
            f.resume
            Fiber.yield if f.alive?
        end
    end

    def run(name)
        OS.run(name)
    end

    def ls(d = nil)
        d ||= @@cwd
        list_files(d).each { |f| puts f }
    end

    def show(fn)
        img = Image.from_file(fn)
        if !img
            puts "Failed to load image"
            return
        end
        Display.default.clear
        Display.default.canvas.draw(0, 0, img)
        Display.default.console.goto_xy(0, img.height/32 + 1)
        img
    end

    def edit(f)
        ed = Editor.new
        ed.load(f)
        ed.activate
    end

    def help()
        ed = Editor.new
        ed.load('ruby/help.rb')
        ed.activate
    end


end

