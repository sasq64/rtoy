
class Music
    A4 = 440;
    A = (-4..4).map { |i| A4 * (2**i) }
    puts A

    keys = [ :c, :cs, :d, :ds, :e, :f, :fs, :g, :gs, :a, :as, :b ]

    # Hash from note to freq
    ALL_KEYS = {}
    NOTES = []

    (0..8).each do |oct|
        notes = (-9..3).map { |i| A[oct] * (2**(i/12.0)) }
        (0..11).each do |note|
            note_sym = (keys[note].to_s + oct.to_s).to_sym
            NOTES << note_sym
            ALL_KEYS[note_sym] = notes[note]
        end
    end

    def initialize()
        @audio = Audio.default
        @sound = Audio.load_wav("data/piano.wav")
        @channel = 0
        @tempo = 0.2
    end 

    def tempo=(t)
        @tempo = t
    end

    def tempo() @tempo ; end

    def current_sound=(sound)
        @sound = sound
    end

    def current_sound() @sound ; end

    def play(*notes)
        notes.each do |note|
            unless note == :p
                if note.class == Array
                    note.each do |n|
                        @audio.play(@channel, @sound, ALL_KEYS[n])
                        @channel += @sound.channels
                        @channel = 0 if @channel == 32
                    end
                else
                    @audio.play(@channel, @sound, ALL_KEYS[note])
                    @channel += @sound.channels
                    @channel = 0 if @channel == 32
                end
            end
            # TODO: Queue up notes instead
            # OS.sleep @tempo
        end
    end


end

