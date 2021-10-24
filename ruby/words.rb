

class SpriteSequence
    include OS

    def initialize(sprites)
        x = 0
        @sprites = sprites
        @x = 0
        @y = 0
        @sprites.each do |spr|
            spr.x = x
            x += spr.width
        end
    end

    def length()
        return @sprites.length
    end

    def move(x,y)
        @x = x
        @y = y
        @sprites.each { |spr| spr.move(x,y) if spr ; x += spr.width }
    end

    def x()
        @x
    end

    def x=(v)
        @x = v
        @sprites.each { |spr| spr.x = v if spr ; v += spr.width }
    end

    def y()
        @y
    end

    def y=(v)
        @y = v
        @sprites.each { |spr| spr.y = v if spr }
    end

    def color=(v)
        @color = v
        @sprites.each { |spr| spr.color = v if spr }
    end

    def sprites
        @sprites
    end

    def remove()
        @sprites.each { |spr| remove_sprite(spr) }
        @sprites = []
    end
end


class Words

    include OS

    class Word
        include OS


        def initialize(word, letters)
            x = 0
            @sprites = []
            @chars = []
            @speed = 1 # rand() * 10.0 / word.size
            @x = 0
            @y = 0
            to_upper(word).each_char do |c|
                p c
                img = letters[c]
                if img
                    @w = img.width unless @w
                    spr = add_sprite(img)
                    spr.x = x
                    x += @w
                    @chars << c
                    @sprites << spr
                end
            end
        end

        def speed()
            @speed
        end

        def length()
            return @sprites.length
        end

        def move(x,y)
            @x = x
            @y = y
            @sprites.each { |spr| spr.move(x,y) if spr ; x += spr.width }
        end

        def x()
            @x
        end

        def x=(v)
            @x = v
            @sprites.each { |spr| spr.x = v if spr ; v += spr.width }
        end

        def y()
            @y
        end

        def y=(v)
            @y = v
            @sprites.each { |spr| spr.y = v if spr }
        end

        def color=(v)
            @color = v
            @sprites.each { |spr| spr.color = v if spr }
        end

        def key(c)
            (0...@chars.length).each do |i|
                next unless @chars[i]
                if @chars[i] == c
                    @sprites[i].color = [1.0, 1.0, 1.0, 0.25]
                    #tween(@sprites[i]).to(scale: 0.1)
                    @chars[i] = nil
                    return true
                end
                return false
            end
            return false
        end

        def done()
            # TODO better algo
            @chars.each do |c|
                return false if c
            end
            return true
        end

        def sprites
            @sprites
        end

        def destroy()
            @sprites.each do |spr|
                next unless spr
                tween(spr).to(alpha: 0).when_done { remove_sprite(spr) }
            end
            @chars = []
            @sprites = []
        end
    end

    LETTERS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ'

    def to_char(key)
        return nil unless key >= 0x20 && key < 0xffff
        c = [key].pack('U*')
        case c
        when 'å', '['
            'Å'
        when 'ä', '\''
            'Ä'
        when 'ö', ';'
            'Ö'
        else
            c.upcase
        end
    end

    def initialize()
        font = Font.from_file('data/bedstead.otf')
        @speed = 150
        @dict = []
        @start = [0] * 20
        last_length = 0
        File.open('data/ord').readlines.each do |w|
            word = w.chomp("\n")
            if word.length > last_length
                last_length = word.length
                @start[last_length] = @dict.length
            end
            @dict << word
        end
        @letters = {}
        img = nil
        LETTERS.each_char do |c|
            img = font.render(c, Color::WHITE, 50)
            @letters[c] = img
        end
        @perfect = font.render('PERFECT', Color::CYAN, 24)
        @click = Audio.load_wav("data/tick.wav")
        @beep = Audio.load_wav("data/beep.wav")
        @words = []
        @char_size = img.width
        @current_word = nil
        @score = 0
        @combo = 0
        @timer = Timer.default
        @last_t = @timer.seconds - 10
        @counter = 0

        on_key do |key|
            key = to_char(key)
            next unless key
            ok = false

            if not @current_word
                @words.each do |word|
                    if word.key(key)
                        @current_word = word
                        word.color = Color::RED
                        word.sprites[0].color = [1,1,1,0.25]
                        ok = true
                        break
                    end
                end
            end

            if !ok && @current_word
                if @current_word.key(key)
                    ok = true
                end
            end

            if ok
                t = @timer.seconds
                if t - @last_t < 0.4
                    @combo += 1
                end
                @last_t = t
            else
                @combo = 1
            end


            Audio.default.play(ok ? @click : @beep)

            
            if @current_word && @current_word.done
                if @combo >= @current_word.length
                    s = add_sprite(@perfect).move(@current_word.x, @current_word.y)
                    tween(s).to(x: -500).when_done { remove_sprite(s) }
                end
                @speed -= 1 if @speed > 40
                #@counter = 0
                @score += (@current_word.length * @combo)
                @combo = 1
                @current_word.destroy()
                @words.delete(@current_word)
                @current_word = nil
            end
        end
    end

    def add_word(text)
        p text
        word = Word.new(text, @letters)
        word.x = 500
        @words << word
        word
    end

    def run()
        console.enabled = false
        display.bg = Color::BLACK
        counter = 0

        vsync do
            canvas.clear
            canvas.text(display.width - 280,0,"Score #{@score}  X#{@combo}", 40)
            if @game_over
                canvas.text(display.width - 1000, display.height - 200,"GAME OVER", 80)
                next
            end
            if @counter <= 0
                l = rand(8)
                i = rand(@start[2 + l] - @start[1 + l]) + @start[1]
                w = add_word(@dict[i])
                w.color = Color::GREEN
                w.move(rand(display.width - (w.length + 1) * @char_size), -50)
                @counter = @speed
            end
            @words.each { 
                |word| word.y += word.speed
                if word.y >= display.height
                    @game_over = true
                end
            }
            @counter -= 1
        end
    end
end

Words.new.run
