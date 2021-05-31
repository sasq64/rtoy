
require 'tween.rb'

p "OS"
module OS

    @@display = Display.default
    @@boot_fiber = nil

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
                    p[1].resume(*args) if p[1].alive?
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

    module_function :on_draw, :on_key, :on_drag, :on_click, :on_timer


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
        Input.default.on_key { |key,mod| 
            @@handlers.call_all(:key, key, mod)
        }
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
    
    def vsync()
        Fiber.yield
    end

    module_function :display, :text, :line, :scale, :offset, :add_sprite, :remove_sprite, :clear, :get_char

    @@cwd = "."

    def boot(src = nil, &block)
        if src
            @@boot_fiber = Fiber.new { eval(src) }
        else
            @@boot_fiber = Fiber.new &block
        end
        @@boot_fiber.resume
    end

    def exec(src = nil, &block)
        block = Proc.new { eval(src) } if src
        f = Fiber.new &block
         while f.alive? do
             f.resume
             Fiber.yield if f.alive?
         end
    end

    def run(name)
         f = Fiber.new do
             #load(name)
             src = File.read(name)
             OS.instance_eval(src)
         end
         while f.alive? do
             f.resume
             Fiber.yield if f.alive?
         end
    end

    def ls(d = nil)
        d ||= @@cwd
        list_files(d).each { |f| puts f }
    end

    def gets
        p "GETS"
        a = IOX.read_line
        puts
        a
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

    def sleep(n)
        (0..n).each { p  "x" ; Fiber.yield }
    end

    def help()
        ed = Editor.new
        ed.load('ruby/help.rb')
        ed.activate
    end

    module_function :gets, :help, :run, :edit, :show, :ls, :exec, :sleep, :exec, :boot

end

p "OS DONE"
