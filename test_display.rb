
def color(rgb, a = 1.0)
    [(rgb >> 16) / 255.0, ((rgb>>8)&0xff) / 255.0, (rgb&0xff) / 255.0, a]
end

display = Display.default

assert(display.width > 100)
assert(display.height > 100)

# Does display clear to background color
display.console.enabled = false
display.bg = color(0x40ff80)
display.clear
assert(display.dump(0,0) == 0x40ff80)

assert(display.console != nil)
assert(display.sprites != nil)
assert(display.canvas != nil)


draw_called = false
display.on_draw { draw_called = true }

exit
