display.bg = 0
display.canvas.fg = 0x00005010
display.canvas.blend_mode = :add
display.console.clear()

def sinx(i)
    (Math.sin(i * Math::PI / 500) + 1) * display.width / 2
end

def siny(i)
    (Math.sin(i * Math::PI / 500) + 1) * display.height / 2
end

scale 64
text 0,0,"SHADE BOBS"

xi = 0
yi = 0
loop do
    display.canvsas.clear()
    display.console.offset((sinx(xi) - display.width) * 2.6, 0)
    300.times { |j|
        circle (sinx(xi+j*7) + sinx(xi + 190 +j*5)) / 2, siny(yi+j*13), 100
    }
    xi += 2
    yi += 3
    vsync()
end
