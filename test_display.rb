

display = Display.default

assert(display.width > 100)
assert(display.height > 100)

# Does display clear to background color
display.console.enabled = false
display.bg = [0.25,1.0,0.5,1.0]
assert(display.bg == [0.25, 1.0, 0.5, 1.0])
display.clear
assert(display.dump(0,0) == 0x40ff80)

assert(display.console != nil)
assert(display.sprites != nil)
assert(display.canvas != nil)


draw_called = false
display.on_draw { draw_called = true }



exit
