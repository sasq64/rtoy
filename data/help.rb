## This is the help text for R-Toy
## It is also a Ruby source file.
## You can RUN this file at any time by pressing F5.
##
## Normally, you get BACK to the editor by pressing
## F5 again, but if things go wrong you can RESET
## by pressing F12

## This file contains a set of code snippets that you
## can try out, beginning with the classic "hello world",
## which is what you will see if you press F5 on an
## unchanged help file.

## 'puts' is the same as 'print' except it adds a newline.
## Note that parentheses in ruby are optional.

# puts "Hello world!"


## 'gets' is used to read a line of text

# print "What is your name ? "
# name = gets()
# puts "Hello #{name}"


## The classical maze effect. If we don't add a sleep
## the loop will never yield and nothing will be shown.

# loop { print rand(2) == 0 ? "╲" : "╱" ; sleep 1 }


## To paint some circles, uncomment this;

# on_drag { |x,y| circle(x,y,10) }


## To instead draw lines, you need a bit more

# px,py = 0,0
# on_click { |x,y| px,py = x,y }
# on_drag { |x,y|
#     line(px,py,x,y)
#     px,py = x,y
# }
 

## This code will add a bouncing ball that disappears after
## 10 seconds.

# display = Display.default
# bimg = Image.from_file('data/ball.png')
# w,h = display.size
# ball = add_sprite(bimg).move(rand(w), rand(h))
# ball.scale = 0.25
# bx,by = rand(16)-8,rand(16)-8
# tween(seconds: 10.0) do |delta|
#     ball.x += bx
#     ball.y += by
#     bx = -bx if ball.x <= 0 or ball.x >= w
#     by = -by if ball.y <= 0 or ball.y >= h
#     remove_sprite(ball) if delta >= 1.0
# end

## If you want to play with turtle graphics, type 'turtle' at
## the prompt.
