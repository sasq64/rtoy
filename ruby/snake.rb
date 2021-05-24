### SNAKE

include OS

APPLE = '⬤'
WIDTH = 40
HEIGHT = 30
TOP = 2

$x,$y = 5,(HEIGHT/2)
$mx,$my = 1,0
$speed = 150
$length = 14
$score = 0
$keys = []
$snake = []
$game_over = false


def create_apple
    c = ''
    while c != ' '
        x,y = rand(WIDTH-2) + 1, rand(HEIGHT-TOP-1) + TOP + 1
        c = get_char(x, y)
    end
    text(x, y, APPLE, Color::LIGHT_RED, 0)
end
 
on_key { |key| $keys.append(key) }

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

Timer.default.on_timer($speed) {

    return if $game_over

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
        $game_over = true
        text(0, 1, "GAME OVER")
    end

    text(WIDTH - 10, 1,"SCORE: #{$score}")

    $speed -= 0.02 if $speed > 20
    Timer.default.on_timer($speed)
}


Display.default.on_draw {}
scale(4.0, 2.0)

# Draw playfield
clear()

WIDTH.times { |x|
    text(x,TOP,'━')
    text(x,HEIGHT-1,'━')
}

(HEIGHT-TOP).times { |y|
    text(0,y+TOP,'│')
    text(WIDTH-1,y+TOP,'│')
}
text 0,TOP,'┍'
text WIDTH-1,TOP,'┑'
text 0,HEIGHT-1,'┕'
text WIDTH-1,HEIGHT-1,'┙'

1.times { create_apple }

