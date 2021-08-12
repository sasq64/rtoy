require "os.rb"

include OS

clear()

START_OCTAVE = 2
END_OCTAVE = 6

WIDTH = (display.width / (7*5*4)).to_i
HEIGHT = WIDTH * 4

def use_octave?(note)
    oct = note.to_s[-1].to_i
    oct >= START_OCTAVE && oct <= END_OCTAVE
end

$lookup = [:p] * 100

# Setup white keys
x = 0
canvas.fg = Color::WHITE
Music::NOTES.each_with_index do | note, key |
    if use_octave?(note) && [0,2,4,5,7,9,11].include?(key % 12)
        canvas.rect(x*WIDTH, 0, WIDTH*4-2, WIDTH*12)
        (x...x+4).each { |i| $lookup[i] = note }
        x += 4
    end
end

# Setup black keys
x = 0
canvas.fg = Color::BLACK
Music::NOTES.each_with_index do | note, key |
    n = key % 12
    if use_octave?(note) && [1,3,6,8,10].include?(n)
        canvas.rect((x+3)*WIDTH, 0, WIDTH*2, WIDTH*8)
        (x+3...x+5).each { |i| $lookup[i] = note }
        x += 4
        x += 4 if n == 3 || n == 10
    end
end

def get_note(x,y)
    x /= WIDTH
    if y > WIDTH*8
        x = (x & 0xffffc) + 1
    end
    $lookup[x]
end

music = Music.new

OS.on_click do |x,y|
    $note = get_note(x,y)
    music.play($note)
end

OS.on_drag do |x,y|
    note = get_note(x,y)
    if note != $note
        $note = note
        music.play($note)
    end
end

loop { vsync }
