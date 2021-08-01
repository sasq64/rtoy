
class Color

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
    def to_a() @data end

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

    BLACK = Color.new(0.0, 0.0, 0.0, 1.0)
    WHITE = Color.new(1.0, 1.0, 1.0, 1.0)
    RED = Color.new(0.533, 0.0, 0.0, 1.0)
    CYAN = Color.new(0.667, 1.0, 0.933, 1.0)
    PURPLE = Color.new(0.8, 0.267, 0.8, 1.0)
    GREEN = Color.new(0.0, 0.8, 0.333, 1.0)
    BLUE = Color.new(0.0, 0.0, 0.667, 1.0)
    YELLOW = Color.new(0.933, 0.933, 0.467, 1.0)
    ORANGE = Color.new(0.867, 0.533, 0.333, 1.0)
    BROWN = Color.new(0.4, 0.267, 0.0, 1.0)
    LIGHT_RED = Color.new(1.0, 0.467, 0.467, 1.0)
    DARK_GREY = Color.new(0.2, 0.2, 0.2, 1.0)
    GREY = Color.new(0.467, 0.467, 0.467, 1.0)
    LIGHT_GREEN = Color.new(0.667, 1.0, 0.4, 1.0)
    LIGHT_BLUE = Color.new(0.0, 0.533, 1.0, 1.0)
    LIGHT_GREY = Color.new(0.733, 0.733, 0.733, 1.0)
    TRANSP = Color.new(0.0, 0.0, 0.0, 0.0)
end

