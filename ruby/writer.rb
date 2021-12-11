
display.bg = BLACK
console.enabled = false
canvas.fg = Color::WHITE
clear()
font = Font.from_file('data/Ubuntu-B.ttf')
text = %w(
  This is the text that should be written to the screen.
  It should also wrap when we run out of horizontal space.
  When the sprites reach their final destination they
  are removed, and we draw the text into the canvas instead.
)

margin = 20
pos = vec2(margin, 0)
edge = display.width - margin
size = 100
space = size/3

text.each do |word|
    img = font.render(word, size, Color::WHITE)
    if pos.x + img.width > edge
        pos = vec2(margin, pos.y + size)
    end
    s = add_sprite(img).move(*pos)
    s.alpha = 0
    pos += [img.width + space, 0]
    tween(s).seconds(2.0).
        fn(:in_bounce).from(pos: vec2(display.width,800)).
        fn(:smooth_step).to(alpha: 1).
        to(rotation: 2 * Math::PI).
        when_done {
          display.canvas.draw(s.x, s.y, s.img) 
          remove_sprite(s) 
        }
    sleep(0.1)
end
