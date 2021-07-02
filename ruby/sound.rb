A4 = 440;
A = (-4..4).map { |i| A4 * (2**i) }
puts A

notes4 = (-9..3).map { |i| A[7] * (2**(i/12.0)) }

puts notes4

sound = Audio.load_wav("data/piano.wav")

channel = 0
on_key { |key|
    x = key - '1'.ord
    if x >=0 && x <= 9
        p sound
        audio.play(channel, sound, notes4[x] * 44)
        channel = (channel + 2) % 32
    end
    if key == 'a'.ord
        audio.set_freq(channel-1, 14400)
    end
}

on_drag { |x,y|
    audio.set_freq(channel-1, x * 10)
}
#audio.play(1, 18200, sound)
#audio.play(2, 17200, sound)
#sleep 100

