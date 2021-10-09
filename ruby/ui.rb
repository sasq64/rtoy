
require 'vec2.rb'

module UI
    # Abstract rectangular UI element
    class Element

        def initialize(pos, size)
            @pos = Vec2.new(*pos)
            @size = Vec2.new(*size)
            p "SIZE #{@size.x} #{@size.y}"
        end

        def x() @pos.x end
        def y() @pos.y end
        def width() @size.x end
        def height() @size.y end

        def render(ui) end
        def on_click(x,y) end

        def contains?(pos)
            pos.between?(@pos, @pos + @size)
        end

    end

    # Parent for UI elements. Renders on canvas
    class Frame
        attr_reader :canvas, :input, :elements
        def initialize()
            @canvas = Display.default.canvas
            @input = Input.default
            @click_handler = nil
            @elements = []
            @input.on_drag do |x,y|
                if @drag_handler
                    @drag_handler.call(x,y)
                end
            end
            @input.on_click do |x,y|
                pos = Vec2.new(x,y)
                found = false
                @elements.each do |elem|
                    p "Check #{x},#{y} in #{elem}"
                    if elem.contains?(pos)
                        found = true
                        elem.on_click(x,y)
                        next
                    end
                end
                if !found && @click_handler
                    @click_handler.call(x,y)
                end
            end

            Display.default.on_draw do
                self.render()
            end
            
        end

        def on_click(&block)
            @click_handler = block
        end

        def on_click_or_drag(&block)
            @click_handler = block
            @drag_handler = block
        end

        def add_element(elem)
            @elements << elem
        end
        
        def render()
            @elements.each { |e| e.render(self) }
        end
    end

    class Text < Element
        def initialize(pos, size, text, size)
            super(pos, size)
            @text = text
            @size = size
        end

        def render(ui)
            ui.canvas.text(@x, @y, @text, @size)
        end
    end

    class Button < Element
        def initialize(pos, size, text, &block)
            super(pos, size)
            @text = text
            @click_handler = block
        end

        def render(ui)
            e = @pos + @size
            ui.canvas.line(@pos.x, @pos.y, e.x, e.y)
            ui.canvas.line(@pos.x, e.y, e.x, @pos.y)
            ui.canvas.text(@pos.x, @pos.y, @text, 24)
        end

        def on_click(x,y)
            @click_handler.call
        end
    end

    class ToolBar < Element
        def add_image(image)
            @images << image
        end

        def render(ui)
            @images.each do |img|
                ui.canvas.draw(pos, img)
                pos.x += img.width
            end
        end
    end
end
