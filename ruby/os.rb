
module OS

    class App
        def initialize()
            @callbacks = { key:[], draw:[], drag:[], click: [], timer: [] }
        end

        def add(what, cb)
            i = @callbacks[what].index(:nil?)
            if i
                @callacks[what][i] = cb
            else
                i = @callbacks[what].size
                @callbacks[what].append(cb)
            end
            i
        end

        def remove(what, index)
            @callbacks[what][index] = nil
        end


        def call_all(what, *args)
            @callbacks[what].each { |cb| cb.call(*args) if cb }
        end
    end

    @@app_stack = [ App.new ]

    def on_draw(&block)
        @@app_stack.last.add(:draw, block)
    end

    def on_key(&block)
        @@app_stack.last.add(:key, block)
    end

    def on_drag(&block)
        @@app_stack.last.add(:drag, block)
    end

    def on_click(&block)
        @@app_stack.last.add(:click, block)
    end
    
    def on_timer(&block)
        @@app_stack.last.add(:timer, block)
    end

    def remove_handler(what, i)
        @@app_stack.last.remove(what, i)
    end

    def self.start_app(x)
        @@app_stack.append(App.new)
    end

    def self.end_app()
        @@app_stack.pop
    end

    def self.reset_handlers
        p "HANDLERS"

        t = Timer.default
        Display.default.on_draw { @@app_stack.last.call_all(:draw, t.seconds) }
        Input.default.on_key { |*args| @@app_stack.last.call_all(:key, *args) }
        Input.default.on_drag { |*args| @@app_stack.last.call_all(:drag, *args) }
        Input.default.on_click { |*args| @@app_stack.last.call_all(:click, *args) }
    end

    reset_handlers()

    @@display = Display.default
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

    def ls(*args)
        args = ["."] if args.empty?
        Dir.entries(args[0]).each { |f| puts f unless f.start_with?('.') }
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

