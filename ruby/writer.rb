clear
font = Font.from_file('data/Ubuntu-B.ttf')
text = %w(
  This is the text that should be written to the screen.
  It should also wrap when we run out of horizontal space.
  When the sprites reach their final destination they
  are removed, and we draw the text into the canvas instead.
)

margin = 20
pos = vec2(20,0)
space = 30
edge = display.width - margin

sprites = text.map do |word|
    img = font.render(word, 100)
    if pos.x + img.width > edge
        pos = vec2(margin, pos.y + 100)
    end
    s = add_sprite(img).move(*pos)
    s.alpha = 0
    pos += [img.width + space, 0]
    s
end

sprites.each do |s|
    tween(obj: s, seconds: 2.0).
      target(from_pos: vec2(display.width,800), func: :in_bounce).
      target(to_alpha: 1, func: :smooth_step).
      target(to_rot: 2 * Math::PI).
      when_done {
          display.canvas.draw(s.x, s.y, s.img) 
          remove_sprite(s) 
      }
      sleep(10)
end
