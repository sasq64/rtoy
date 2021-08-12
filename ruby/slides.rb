require 'os.rb'
require 'tween.rb'
require 'vec2.rb'

OS.display.bg = Color::WHITE
OS.clear()


# Slide Contract:

# Should not allocate any graphical resources on creationg
# start() will allow the Slide to animate and render to the canvas
# A slide can be stepped until :showing
# At this point all information is visible
# Calling exit() or stepping again will cause the slide to clear its
# content from the screen. The canvas must be empty after a call to
# exit, but sprites can be animating until :clear
#
# step() - Start or step to next state in slide. Returns state
# state() - Returns :unstarted, :in_progress, :showing, :exiting, :clear
# show() - Jump directly to showing everything
# exit() - Clear the slide contents from the screen

class Slide

    include OS

    def initialize()
        @font = Font.from_file('data/Ubuntu-B.ttf')
        @fixed = Font.from_file('data/Hack.ttf')
        @ypos = 10
        @tweens = []
        @blocks = []
        @block_no = 0
        @header_sizes = [120, 80, 64, 48]
        @anim = :none
        @justify = :left

    end

    def justify(j)
        @justify = j
    end

    def anim(a)
        @anim = a
    end

    def done?
        @block_no >= @blocks.size
    end

    def pause()
        @blocks << [:pause, 0, '']
    end

    def finish()
        # Finish all ongoing tweens
        @tweens.each do |tween|
            tween.finish()
        end
    end

    def step()

        # Finish all ongoing tweens
        @tweens.each do |tween|
            tween.finish()
        end

        @tweens = []

        while @block_no < @blocks.size
            block = @blocks[@block_no]
            @block_no += 1
            type,arg,txt,anim = *block
            p "BLOCK #{type} #{arg} #{txt}"
            p @header_sizes[arg]            
            case type
            when :h
                @ypos += add(@font.render(txt, Color::BLACK, @header_sizes[arg]),
                             @ypos, anim)
            when :p
                x = 20
                y = 0
                width = canvas.width
                line = []
                lines = []
                ss = @font.get_size(' ', 48)
                txt.split.each do |word|
                    w,h = @font.get_size(word, 48)
                    y = h if y = 0
                    if w + x > width
                        lines << line.join(' ')
                        line = []
                        x = 20
                        y += h
                    end
                    line << word
                    x += w
                end
                txt.split("\n").each do |line|
                    @ypos += add(@font.render(line, Color::BLACK, 48), @ypos, anim)
                end
            when :code
                txt.split("\n").each do |line|
                    @ypos += add(@fixed.render(line, Color::BLACK, 48), @ypos, anim)
                end
            when :pause
                break
            when :space
                @ypos += arg
            end
        end
    end

    def heading(txt, level = 1)
        @blocks << [:h, level, txt, @anim]
    end

    def sub(txt)
        heading(txt, 3)
    end

    def para(txt)
        @blocks << [:p, 0, txt, @anim]
    end

    def code(txt)
        @blocks << [:code, 0, txt, @anim]
    end

    def space(y = 20)
        @blocks << [:space, 20, '', @anim]
    end

    def h1(txt)
        heading(txt, 1)
    end

    def add(img, y, what)

        if @justify == :center
            target = [(canvas.width - img.width) / 2, y]
        else
            target = [20, y]
        end

        if what == :none
            canvas.draw(target[0], target[1], img)
            return img.height
        end

        if what == :from_right
            pos = [canvas.width, y]
        else
            pos = [-img.width, y]
        end
        s = add_sprite(img).move(*pos)

        if what == :fade
            s.alpha = 0
            s.move(*target)
            @tweens << tween(s).seconds(2.0).to(alpha: 1.0).
                when_done {
                    canvas.draw(s.x, s.y, s.img) 
                    remove_sprite(s) 
                }
            return img.height
        end

        @tweens << tween(s).seconds(2.0).fn(:out_bounce).to(pos: target).
            when_done {
                canvas.draw(s.x, s.y, s.img) 
                remove_sprite(s) 
            }
        img.height
    end

end

class SlideDeck

    include OS

    def initialize(&block)
        @slides = []
        if block
            self.instance_exec(&block)
        end
    end

    def slide(&block)
        s = Slide.new
        s.instance_exec(&block)
        @slides << s
    end

    def play()
        no = 1
        tot = @slides.size
        @slides.each do |slide|
            canvas.clear()
            canvas.text(canvas.width - 90, canvas.height - 60, "#{no}/#{tot}", 48)
            while !slide.done?
                slide.step()
                OS.read_key()
            end
            no += 1
            slide.finish()
        end
    end
end

slides = SlideDeck.new {
    slide {
        anim :from_right
        justify :center
        h1 "R-Toy"
        anim :from_left
        sub "The Virtual Ruby Home Computer"
    }

    slide {
        anim :fade
        para "Trying to replicate the effect an 80s home computer had
    on a nerd like me."
        space 30
        code '10 PRINT "HELLO"
    20 GOTO 10'
    }

    slide {
        anim :from_right
        h1 "Alternatives"
        para "• UNITY"
        para "• AOZ STUDIO (AMOS)"
        para "• PYGAME"
        para "• FLASH / ACTIONSCRIPT"
        para "• MODDING / MINECRAFT"
        para "• SONIC PI"
    }
}

p "PLAY"

slides.play
OS.loop { OS.vsync() }

