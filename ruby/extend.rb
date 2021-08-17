
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

class Layer
    alias set_scale scale=
    alias get_scale scale

    def scale=(v)
        set_scale(v.to_a)
    end
    def scale()
        Vec2.new(*get_scale())
    end
end

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

class Console
    def visible_rows
        ts = self.get_tile_size()
        s = self.scale.to_a
        (self.height / (ts[1] * s[1])).to_i
    end
end

class Audio
    extend MethAttrs
    returns! Sound, :load_wav

end

