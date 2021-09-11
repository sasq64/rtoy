
module Shortcuts
    @@display = Display.default

    extend MethAttrs

    doc! "Create a Vec2"
    def vec2(x,y)
        Vec2.new(x,y)
    end

    doc! "Returns the default display"
    returns! Display
    def display() @@display end

    doc! "Returns the default console layer"
    returns! Console
    def console() @@display.console end

    doc! "Returns the default canvas layer"
    returns! Canvas
    def canvas() @@display.canvas end

    doc! "Returns the default sprites layer"
    returns! Sprites
    def sprites() @@display.sprites end

    doc! "Returns the default Audio instance"
    returns! Audio
    def audio() Audio.default end

    doc! "Returns the default Speech instance"
    returns! Speech
    def speech() Speech.default end

    doc! "Use text to speech to vocalize the given text"
    def say(text)
        sound = Speech.default.text_to_sound(text)
        audio.play(sound)
    end

    doc! "Play a sound at default or given frequency"
    def play(sound, freq = nil)
        audio.play(sound)
    end

    doc! "Load an image (png)"
    returns! Image
    takes_file! 
    def load_image(*args) Image.from_file(*args) end

    doc! "Draw text at specific location with specific colors"
    def text(*args, **kwargs) 
        @@display.console.text(*args)
    end
    doc! "Get the character at x,y in the console"
    def get_char(x, y) @@display.console.get_char(x, y) end
    def scale(x, y = x) @@display.console.scale = [x,y] end
    def offset(x, y) @@display.console.offset(x,y) end

    def get_key(key) Input.default.get_key(key) end
    
    doc! "Draw a line in the canvas from x,y to x2,y2"
    def line(*args) @@display.canvas.line(*args) end
    doc! "Draw a circle in the canvas at x,y with radius r"
    def circle(*args) @@display.canvas.circle(*args) end

    returns! Sprite
    doc! "Add a sprite to the screen"
    def add_sprite(img, **kwargs)
        spr = @@display.sprites.add_sprite(img)
        kwargs.each do |a,b|
            spr.send (a.to_s + '=').to_sym, b
        end
        spr
    end
    doc! "Remove a sprite"
    def remove_sprite(spr) @@display.sprites.remove_sprite(spr) end

    def clear()
        @@display.clear
    end
end
