### SNAKE

#include OS

APPLE = '⬤'
WIDTH = 40
HEIGHT = 30
TOP = 2

$x,$y = 5,(HEIGHT/2)
$mx,$my = 1,0
$speed = 0.150
$length = 14
$score = 0
$keys = []
$snake = []
$game_over = 0

def draw_box(x,y,w,h)
    w.times { |i|
        text(x+i,y,'━')
        text(x+i,y+h-1,'━')
    }

    h.times { |i|
        text(x,y+i,'│')
        text(x+w-1,y+i,'│')
    }
    text x,y,'┍'
    text x+w-1,y,'┑'
    text x,y+h-1,'┕'
    text x+w-1,y+h-1,'┙'
end

def create_apple
    c = ''
    while c != ' '
        x,y = rand(WIDTH-2) + 1, rand(HEIGHT-TOP-1) + TOP + 1
        c = get_char(x, y)
    end
    text(x, y, APPLE, Color::LIGHT_RED, Color::BLACK)
end
 
on_key { |key| $keys.append(key) }

on_click do |x,y|
    if $mx != 0
        $keys.append(y/32 > $y ? Key::DOWN : Key::UP)
    else
        $keys.append(x/32 > $x ? Key::RIGHT : Key::LEFT)
    end
end


def step_worm

    $x += $mx
    $y += $my
    c = get_char($x,$y)


    text($x,$y,'▓')
    $snake.append($x,$y)

    if $snake.length > $length
        # Erase tail
        rx,ry = $snake.shift(2)
        text(rx, ry, ' ')
    end
    c
end

#tween(console).to(scale: [4,2])
scale(4.0, 2.0)
console.style.fg = Color::GREEN
console.style.bg = Color::BLACK
clear()
draw_box(0,TOP, WIDTH, HEIGHT-TOP)

1.times { create_apple }

loop do
    if $game_over > 0
        sleep(1000)
    end

    boost = false
    unless $keys.empty?
        case $keys.shift
        when Key::LEFT
            $mx,$my = -1,0 if $mx == 0
        when Key::RIGHT
            $mx,$my = 1,0 if $mx == 0
        when Key::UP
            $mx,$my = 0,-1 if $my == 0
        when Key::DOWN
            $mx,$my = 0,1 if $my == 0
        when ' '.ord
            boost = true
        end
    end

    while true
        c = step_worm
        break if !boost or c != ' '
    end 

    if c == APPLE
        $score += 1
        $length += 15
        create_apple
    elsif c != ' '
        $game_over = 1
        text(0, 1, "GAME OVER")
    end

    text(WIDTH - 10, 1,"SCORE: #{$score}")

    $speed -= 0.00002 if $speed > 0.02
    sleep($speed)
end
