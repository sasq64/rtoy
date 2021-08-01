
canvas.fg = Color::WHITE
WIDTH = 40
HEIGHT = WIDTH * 4

def white_key(no)
    [no*WIDTH, 20, WIDTH-6, HEIGHT]
end

def black_key(no)
    [no*WIDTH + WIDTH*2/3, 20, WIDTH/2, HEIGHT*2/3]
end

30.times do |i|
    canvas.rect(*white_key(i))
end

canvas.fg = Color::BLACK
30.times do |i|
    n = i % 7
    next if n == 2 || n == 6
    canvas.rect(*black_key(i))
end

sound = Audio.load_wav("data/piano.wav")

on_click do |x,y|
    Audio.default.play(0,sound, x)
end
