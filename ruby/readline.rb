# get_line
#
# remeber @ypos
#

require 'complete.rb'

class LineReader

    include OS
    include Complete

    def initialize(history: nil)
        @history_file = history
        @line = []
        @pos = 0
        @ypos = 0
        @xpos = 0
        @history = []
        @history_pos = 0
        @dirty = true
        @con = Display.default.console
    end

    def history_up()
        if @history_pos > 0 then
            _,y = @con.get_xy()
            @history_pos -= 1
            text(1,y, ' ' * (@line.length + 1))
            @line = @history[@history_pos].clone
            @pos = @line.length
        end
    end

    def history_down()
        if @history_pos < @history.length-1 then
            _,y = @con.get_xy()
            @history_pos += 1
            text(1,y, ' ' * (@line.length + 1))
            @line = @history[@history_pos].clone
            @pos = @line.length
        end
    end


    def paste()
        text = Input.get_clipboard()
        p text
        p "CLIP"
        p @line
        p @pos
        text.each_char do |t|
            p t
            @line.insert(@pos, *(t.unpack('U')))
            @pos += 1
            p @line
            p @pos
        end
        p @line
    end

    def readline_key(key, mod)

        self.accept() unless key == Key::TAB

        @dirty = true
        p "RL KEY #{key}\n"

        case key
        when 0x20..0x7e
            @line.insert(@pos, key)
            @pos += 1
        when Key::F3
            paste()
        when Key::INSERT
            if mod & 1
                paste()
            end
        when Key::LEFT
            @pos -= 1
        when Key::RIGHT
            @pos += 1
        when Key::UP
            history_up()
        when Key::TAB
            unless @line.empty?
                l = complete(@line.pack('U*'))
                unless l.empty?
                    @line = l.unpack('U*')
                    @pos = @line.length
                end
            end
        when Key::HOME
            @pos = 0
        when Key::END
            @pos = @line.length
        when Key::DOWN
            history_down()
        when Key::ENTER
            @history_pos = @history.length
            @con.goto_xy(@xpos,@ypos)
            @con.print(@line.pack('U*') + '   ')
            return if @line.empty?
            if @history[-1] != @line then
                @history.append @line
                if @history_file
                    File.open(@history_file, "w+") do |f|
                        @history.each { |h| f.puts(h.pack("U*")) }
                    end
                end
            end
            @history_pos = @history.length
            readline_finish()
        when Key::BACKSPACE
            if @pos > 0
                @pos -= 1
                @line.slice!(@pos)
            end
        end

        @pos = 0 if @pos < 0
        @pos = @line.length if @pos > @line.length

    end

    def readline_finish()
        p @handler
        @handler.call(@line.pack('U*')) if @handler
        @handler = nil
        @line = []
    end

    def readline_draw(t)
        return if not @dirty
        @dirty = false
        @con.goto_xy(@xpos, @ypos)
        @con.print(@line[0..@pos-1].pack('U*')) unless @pos == 0
        @con.print(@pos < @line.length ? @line[@pos].chr : ' ', -1, Color::ORANGE) 
        @con.print(@line[@pos+1..].pack('U*')) if @pos < @line.length-1
        @con.print('   ')
    end

    def start()
        @key_i = on_key { |key,mod| readline_key(key, mod) }
        @draw_i = on_draw { |t| readline_draw(t) }
        if @history_file and File.exists?(@history_file)
            @history = File.open(@history_file).readlines.map(&:chomp).map { |l| l.unpack("U*") }
        else
            @history = []
        end
        @history_pos = @history.size
    end

    def stop()
        remove_handler(:key, @key_i)
        remove_handler(:draw, @draw_i)
    end

    def read_line(&block)
        @dirty = true
        @handler = block if block_given?
        @xpos,@ypos = @con.get_xy()
    end

end

module IOX
    def self.read_line(history: nil)
        name = nil
        rl = LineReader.new(history: history)
        rl.read_line { |l| p "RD" ; name = l }
        rl.start
        p "RL START"
        Fiber.yield while name.nil?
        p "RL STOP"
        rl.stop
        return name
    end
end

