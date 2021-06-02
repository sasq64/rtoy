#include OS

# img = Image.from_file('data/paddle.png')
# s = Display.default.sprites.add_sprite(img)
# s.x = 1000
# s.y = 500
# s.scale = 0.25
#Display.default.on_draw { con.rotation += 0.04 }


def snake() run("ruby/snake.rb") end
def pong() run("ruby/pong.rb") end

def tt()
    f = Font.from_file('data/Hack.ttf')
    i = f.render("Testing this!", 0)
    display.canvas.draw(10,800, i)
    i
end

def ball
    $bimg ||= Image.from_file('data/ball.png')
    w,h = display.size
    ball = add_sprite($bimg).move(rand(w - $bimg.width), rand(h - $bimg.height))
    ball.scale = 0.25
    bx,by = rand(16)-8,rand(16)-8
    tween(seconds: 10.0) do |delta|
        ball.x += bx
        ball.y += by
        bx = -bx if ball.x <= 0 or ball.x >= w
        by = -by if ball.y <= 0 or ball.y >= h
        display.sprites.remove_sprite(ball) if delta >= 1.0
    end
end


def block_test

    pressed = false
    on_key { pressed = true }
    puts "Press a key"
    Fiber.yield while !pressed
    puts "Thanks"

end

def maze
    500.times do
        print rand(2) == 0 ? "╲" : "╱"
        Fiber.yield
    end
end

def paint
    cx,cy = 0,0
    on_click { |x,y| cx,cy = x,y }
    on_drag { |x,y|
        line(cx, cy, x, y)
        cx,cy = x,y
    }
end

def paint2
    $img = Image.from_file('data/face.png')
    on_click { |x,y| $display.canvas.draw(x,y,$img) }
end

def rot()
    tween(seconds:2.0).target(display.console, :rotation=, from:0, to: 2 * Math::PI)
end

def circle_tween()
    tween(obj: display.canvas, seconds: 1.0)
        .target(method: :clear)
        .target(method: :circle,
               from: [500,500,10],
               to: [500,500,250])
end

