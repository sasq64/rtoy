
class Sprites
    extend MethAttrs
    class_doc! "Sprite layer"
    doc! "Create and display new sprite from a given `Image`", :add_sprite
    returns! Sprite, :add_sprite
end

class Canvas
    extend MethAttrs

    alias line_with_style line
    alias circle_with_style circle

    def line(*args, **kwargs)
        if kwargs.size > 0
            style = Style.new
            kwargs.each { |a,b| style.send (a.to_s + '=').to_sym, b }
            line_with_style(*args, style)
        else
            line_with_style(*args)
        end
    end

    def circle(*args, **kwargs)
        if kwargs.size > 0
            style = Style.new
            kwargs.each { |a,b| style.send (a.to_s + '=').to_sym, b }
            circle_with_style(*args, style)
        else
            circle_with_style(*args)
        end
    end

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

    alias set_offset offset=
    alias get_offset offset

    def scale=(v)
        set_scale(v.to_a)
    end
    def scale()
        Vec2.new(*get_scale())
    end

    def offset=(v)
        set_offset(v.to_a)
    end
    def offset()
        Vec2.new(*get_offset())
    end

    def fg() style.fg end
    def fg=(fg) style.fg  = fg end


end

class Display
    extend MethAttrs
    def size
        Vec2.new(width, height)
    end

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

