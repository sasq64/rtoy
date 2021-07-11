
class Vec2
    extend MethAttrs
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

