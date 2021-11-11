
# Horizontal sequence of sprites
class SpriteSequence
    include OS

    def initialize(sprites)
        x = 0
        @sprites = sprites
        @x = 0
        @y = 0
        @alpha = 1.0
        @color = @sprites.first.color
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

    def alpha()
        @alpha
    end

    def alpha=(v)
        @alpha = v
        @sprites.each { |spr| spr.alpha = v if spr }
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

class SpriteWord < SpriteSequence

    include OS

    def self.font()
        @@font
    end

    def initialize(word, letters)

        @chars = []
        sprites = []
        to_upper(word).each_char do |c|
            img = letters[c]
            if img
                sprites << add_sprite(img)
                @chars << c
            end
        end
        super(sprites)
    end

    def matches(c)
        first = @chars.find(&:itself)
        return first == c
    end

    def hit()
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
        tween(self).to(alpha: 0).when_done { self.remove() }
        @chars = []
    end
end

class WordLayer
    LETTERS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ'
    def initialize(font, size = 50)
        @font = Font.from_file(font)
        @letters = {}
        @words = []
        img = nil
        LETTERS.each_char do |c|
            img = @font.render(c, Color::WHITE, size)
            @letters[c] = img
        end
    end

    def add_word(text)
        word = SpriteWord.new(text, @letters)
        @words << word
        word
    end

end

class WordTest
    def initialize()
        l = WordLayer.new('data/bedstead.otf')
        w = l.add_word("TEST")
        tween(w).seconds(2.0).to(y: 500).when_done { w.destroy() }
        w.color = Color::RED
        w.key('T')
        OS.vsync do
        end
    end
end

class Words

    include OS

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
        @perfect = SpriteWord.font.render('PERFECT', Color::CYAN, 24)
        @click = Audio.load_wav("data/tick.wav")
        @beep = Audio.load_wav("data/beep.wav")
        @words = []
        @char_size = 30 #img.width
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
                        #word.sprites[0].color = [1,1,1,0.25]
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
        word = SpriteWord.new(text)
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
                |word| word.y += 1
                if word.y >= display.height
                    @game_over = true
                end
            }
            @counter -= 1
        end
    end
end

WordTest.new.run
