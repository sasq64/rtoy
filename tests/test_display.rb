

def color(rgb, a = 1.0)
    [(rgb >> 16) / 255.0, ((rgb>>8)&0xff) / 255.0, (rgb&0xff) / 255.0, a]
end

display = Display.default

assert(display.width > 100)
assert(display.height > 100)

# Does display clear to background color
#display.console.enabled = false
#display.bg = color(0x40ff80)
#display.clear
#assert(display.dump(0,0) == 0x40ff80)

#display.console.text(0,0,"Hello")

assert(display.console != nil)
assert(display.sprite_field != nil)
assert(display.canvas != nil)
display.canvas.clear()

img = Image.from_file('data/tile.png')

assert(img.width == 8)
assert(img.height == 16)

canvas = display.canvas
canvas.draw(0, 0, img)
#canvas.circle(100,100,2)

# TODO: Fix dump()
assert(display.dump(1,img.height-1) == 0x0082bc)
#assert(display.dump(0,0) == 0x7900ff)
#assert(display.dump(1,1) == 0x0000ff)
#assert(display.dump(img.width-1,img.height-1) == 0xff0080)



#canvas.text(100,100,"Hello", 50)

# This is called as main script
# When this script exists r-toy will enter the render loop
# and call handlers
# To test more then the initial state we must
# * create a fiber
# * resume the fiber in on_draw
#
test_fiber = nil
display.on_draw { test_fiber.resume }
test_fiber = Fiber.new do
    loop { Fiber.yield }
    exit
end

