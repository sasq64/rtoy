

module Turtle
    include OS

    def init_turtle()
        p "INIT"
        img = Image.from_file("data/turtle.png")
        p "WTF"
        @turtle = add_sprite(img)
        p "HERE"
        @angle = 0
        @xpos = 500
        @ypos = 500
        p "MOVE"
        @turtle.scale = 0.2
        @turtle.move(@xpos, @ypos)
        @drawing = true
    end

    def forward(len)
        x,y = @xpos,@ypos
        @xpos -= Math.cos((@angle % 360)* 2 * Math::PI / 360) * len
        @ypos -= Math.sin((@angle % 360)* 2 * Math::PI / 360) * len
        @turtle.move(@xpos, @ypos)
        line(x, y, @xpos, @ypos) if @drawing
    end

    def go(x, y)
        @xpos,@ypos = x,y
        @turtle.move(@xpos, @ypos)
    end

    def angle(a)
        @angle = a
        @turtle.rotation = (-@angle % 360) * 2 * Math::PI / 360
    end


    def backward(len)
        x,y = @xpos,@ypos
        @xpos += Math.cos((@angle % 360)* 2 * Math::PI / 360) * len
        @ypos += Math.sin((@angle % 360)* 2 * Math::PI / 360) * len
        @turtle.move(@xpos, @ypos)
        line(x, y, @xpos, @ypos) if @drawing
    end

    def pen_up() @drawing = false end
    def pen_down() @drawing = true end

    def right(degrees)
        @angle += degrees
        @turtle.rotation = (-@angle % 360) * 2 * Math::PI / 360
    end

    def left(degrees)
        @angle -= degrees
        @turtle.rotation = (-@angle % 360) * 2 * Math::PI / 360
    end

end

