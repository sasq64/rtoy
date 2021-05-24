

$bimg = Image.from_file('data/ball.png')
$img = Image.from_file('data/paddle.png')

$paddle = add_sprite($img)

$paddle.y = 1000

on_drag { |x,y|
    $paddle.x = x
}

on_key { |key|
    ball()
}

on_draw { |t|
    Tween.update_all(t)
}


def ball
    ball = add_sprite($bimg).move(rand(1900), rand(1000))
    bx,by = rand(10),rand(10)
    tween() do |delta|
        ball.x += bx
        ball.y += by
        x = ball.x / 16
        y = ball.y / 32
        if $display.console.get_char(x, y) != ' '
            by = -by
            $display.console.text(x, y, ' ')
        end
        bx = -bx if ball.x <= 0 or ball.x >= 1900
        by = -by if ball.y <= 0 or
            (ball.y >= $paddle.y and ball.x.between?($paddle.x, $paddle.x + $img.width))
        remove_sprite(ball) if ball.y >= 1080
    end
end

clear()
scale(2,2)


$display.console.text(5, 20, '###########################################################################')
# 20.times { |y|
#     10.times { |x|
#         puts "#{x} #{y}"
#     }
# }



p $paddle
p $paddle.y
#puts $paddle
#puts $paddle.x
#ball()

