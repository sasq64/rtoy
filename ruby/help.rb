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

## 'print` and `println` are used to output text.
## Note that parentheses in ruby are optional.

println "Hello world!"

## We can also use text to speech
say "Hello world!"

## 'readln' is used to read a line of text

# print "What is your name ? "
# name = readln()
# println "Hello #{name}"


## The classical maze effect. If we don't add a sleep,
## we will only yield at newlines and the output will
## be very fast.

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
#

## This code will add a bouncing ball that disappears after
## 10 seconds.
#display = Display.default
#bimg = Image.from_file('data/ball.png')
#w,h = display.size
#ball = add_sprite(bimg).move(rand(w), rand(h))
#ball.scale = 0.25
#v = Vec2.rand(16,16) - [8,8]
#tween(seconds: 10.0) do |delta|
#    ball.pos += v
#    v.x = -v.x if ball.x <= 0 or ball.x >= w
#    v.y = -v.y if ball.y <= 0 or ball.y >= h
#    remove_sprite(ball) if delta >= 1.0
#end

## If you want to play with turtle graphics, type 'turtle' at
## the prompt.
