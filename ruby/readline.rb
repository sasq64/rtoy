# get_line
#
# remeber @ypos
#

require 'complete.rb'

class LineReader
    extend MethAttrs

    include OS
    include Complete

    class_doc! "Read lines of text from the user using the default console"

    def initialize(history = nil)
        @history_file = history
        if @history_file and File.exists?(@history_file)
            @history = File.open(@history_file).readlines.
                map(&:chomp).map { |l| l.unpack("U*") }
        else
            @history = []
        end
        @history_pos = @history.size

        @last_len = 0
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
        text.each_char do |t|
            @line.insert(@pos, *(t.unpack('U')))
            @pos += 1
        end
    end

    doc! "Put a key into the line reader and handle it"
    def handle_key(key, mod)

        self.accept() unless key == Key::TAB

        case key
        when 0x20..0xffffff
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
                l = complete(@line.pack('U*'), OS)
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
            @con.print(@line.pack('U*') + ' ')
            return false if @line.empty?
            if @history[-1] != @line then
                @history.append @line
                if @history_file
                    File.open(@history_file, "w+") do |f|
                        @history.each { |h| f.puts(h.pack("U*")) }
                    end
                end
            end
            @history_pos = @history.length
            return true
        when Key::BACKSPACE
            if @pos > 0
                @pos -= 1
                @line.slice!(@pos)
            end
        end

        @pos = 0 if @pos < 0
        @pos = @line.length if @pos > @line.length
        return false

    end

    doc! "Draw the line reader at it's given position"
    def draw()
        @con.goto_xy(@xpos, @ypos)
        @con.print(@line[0..@pos-1].pack('U*')) unless @pos == 0
        @con.print(@pos < @line.length ? @line[@pos].chr : ' ', Color::WHITE, Color::ORANGE) 
        @con.print(@line[@pos+1..].pack('U*')) if @pos < @line.length-1
        if @last_len >= @line.length
            @con.print(' ' * (@last_len - @line.length + 1))
        end
        @last_len = @line.length
    end

    doc! "Read a line of text from the user"
    def read_line()
        @xpos,@ypos = @con.get_xy()
        @pos = 0
        @line = []
        loop {
            OS.vsync
            draw()
            key = OS.read_key
            if handle_key(key, 0)
                return @line.pack('U*')
            end
        }
    end

    def self.read_line(history = nil)
        rl = LineReader.new(history)
        return rl.read_line
    end

end

