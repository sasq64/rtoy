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

# say "Hello world!"

## 'readln' is used to read a line of text

#print "What is your name ? "
#name = readln()
#println "Hello #{name}"

## The classical maze effect. If we don't add a sleep,
## we will only yield at newlines and the output will
## be very fast.

# loop { print rand(2) == 0 ? "╲" : "╱" }


## To paint some circles, uncomment this;

# on_drag { |x,y| circle(x,y,10) }


## To instead draw lines, you need a bit more

# px,py = 0,0
# on_click { |x,y| px,py = x,y }
# on_drag { |x,y|
#      line(px,py,x,y)
#      px,py = x,y
# }


## This code will add a bouncing ball that disappears after
## 10 seconds.
#
# def ball()
#     img = Image.from_file('data/ball.png')
#     ball = add_sprite(img).move(*display.size.rand())
#     velocity = Vec2.rand(16,16) - [8,8]
#     tween(ball).seconds(10) do |delta|
#         ball.pos += velocity
#         velocity.x *= -1 if ball.x <= 0 or ball.x >= display.size.x
#         velocity.y *= -1 if ball.y <= 0 or ball.y >= display.size.y
#         remove_sprite(ball) if delta >= 1.0
#     end
# end

# Try surrounding this with 100.times {}
# 100.times { ball }

## If you want to play with turtle graphics, type 'turtle' at
## the prompt.
