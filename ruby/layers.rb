
clist = display.canvases

colors = [LIGHT_RED, LIGHT_GREEN, LIGHT_BLUE, GREY]
colors2 = [RED, GREEN, BLUE, YELLOW]

i = 0
display.bg = Color::BLACK
console.enabled = false
backing = canvas.backing
clist.each { |c|
    c.enabled = true
    #c.backing = backing
    c.style.bg = colors[i]
    c.clear
    i += 1
}

50.times do
    r = rand(100) + 20
    p = (display.size - [r,r]).rand #Vec2.rand(display.width, display.height)
    4.times do |i|
        clist[i].circle(p.x + r , p.y + r, r, fg: colors2[i])
        clist[i].circle(p.x + r - display.width , p.y + r, r, fg: colors2[i])
        clist[i].circle(p.x + r, p.y + r - display.height, r, fg: colors2[i])
    end
end

offsets = [ vec2(0,0), vec2(0,1), vec2(1,0), vec2(1,1) ]
counter = 0
vsync do
    clist[0].offset += vec2(1,-1)
    clist[0].rotation += 0.002
    clist[1].offset += vec2(1,1)
    clist[1].rotation += 0.001
    clist[2].offset += vec2(-1,1)
    clist[2].rotation -= 0.002
    clist[3].offset += vec2(-1,-1)
    clist[3].rotation -= 0.001
    counter += 1
    if counter == 120
        v = display.size / 2
        p v.class
        i = 0
        clist.each do |c|
            o = offsets[i]
            o2 = vec2(1 - o.x, 1 - o.y)
            p v.to_a
            p o.to_a
            p o2.to_a
            tween(c).to(offset: v * o).seconds(3)
            b = (v * o).to_a + (v * o2).to_a
            p b
            tween(c).to(border: b).seconds(3)
            i += 1
        end
    end

end
