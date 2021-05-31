
module OS
    def edit(s)
        e = Editor.new
        e.load(s)
        e.activate
    end
end

class Editor

include OS


def goto_line(y)
    y = 0 if y < 0
    y = @lines.length-1 if y >= @lines.length
    @ypos = y
    @line = @lines[y]
    @xpos = @keep_x
    @xpos = @line.length if @xpos > @line.length
    true
end

def run_source
    save()
    source = get_text()
    Display.default.clear
    @running = true
    @saved_handlers = OS.get_handlers()
    OS.clear_handlers()
    on_key do |key|
        if key == Key::F5
            @running = false
            clear()
            scale(2,2)
            @con.offset(0,0)
            display.console.fg = Color::WHITE
            display.bg = Color::BLUE
            @dirty = true
            OS.set_handlers(@saved_handlers)
        end
    end
    on_draw do
        begin
            eval(source)
        rescue => e
            clear()
            scale(2,2)
            @con.offset(0,0)
            display.console.fg = Color::LIGHT_RED
            display.bg = Color::BLACK
            p e.backtrace[0]
            line = e.backtrace[0].split(":")[1]
            puts "#{e.to_s} in line #{line}"
        end
        loop { Fiber.yield }
    end
end

def editor_key(key, mod)

    if @running
        if key == Key::F5
        end
        return
    end

    h = @con.height/32-2
    @dirty = true
    p "KEY #{key}"
    case key
    when 0x20..0x7e
        if mod & 0xc0 != 0
            if key == 's'.ord
                save()
            end
            if key == 'q'.ord
                OS.end_app()
                clear()
                return
            end
            if key == 'k'.ord
                @cut_line = @line
                @lines.delete_at(@ypos)
                goto_line(@ypos)
            end
            if key == 'p'.ord
                @lines.insert(@ypos, @cut_line)
                goto_line(@ypos)
            end
        else
            p "NORMAL"
            @line.insert(@xpos, key)
            @xpos += 1
        end
    when Key::INSERT
        if mod & 1
            text = System.get_clipboard()
            p text
            text.each_char do |t|
                p t
                 if t == "\n"
                     @lines.insert(@ypos+1, [])
                     goto_line(@ypos+1)
                     @line = @lines[@ypos]
                     @xpos = 0
                 else
                     @line.insert(@xpos, t.ord)
                     @xpos += 1
                 end
            end
        end
    when Key::ESCAPE
        p "EXIT"
        save()
        clear()
        OS.set_handlers(@os_handlers)
        @con.buffer(0)
        @con.goto_xy(@savex, @savey)
        return
    when Key::LEFT
        @xpos -= 1
    when Key::RIGHT
        @xpos += 1
    when Key::TAB
        @line.insert(@xpos, *[32, 32, 32, 32] )
        @xpos += 4
    when Key::PAGE_UP
        goto_line(@ypos - h)
    when Key::PAGE_DOWN
        goto_line(@ypos + h)
    when Key::UP
        goto_line(@ypos - 1)
    when Key::DOWN
        goto_line(@ypos + 1)
    when Key::HOME
        @xpos = 0
    when Key::F5
        @do_run = true
    when Key::END
        @xpos = @line.length
    when Key::ENTER
        rest = @line[@xpos..]
        @lines[@ypos] = @xpos == 0 ? [] : @line[..@xpos-1]
        @lines.insert(@ypos+1, rest)
        goto_line(@ypos + 1)
        @xpos = 0
        if @ypos > 0
            # Simple auto indent
            i = @lines[@ypos -1].index { |i| i != 32 }
            if i
                @line.insert(0, *([32] * i))
                @xpos = i
            end
        end
    when Key::BACKSPACE
        if @xpos > 0
            @xpos -= 1
            @line.slice!(@xpos)
            @lines[@ypos] = @line
        else
            if @ypos > 0
                l = @lines[@ypos-1].length
                @lines[@ypos-1] = @lines[@ypos-1] + @line
                @lines.delete_at(@ypos)
                goto_line(@ypos-1)
                @xpos = l
            end
        end
    end

    if @xpos < 0
        if goto_line(@ypos - 1)
            @xpos = @line.length
        else
            @xpos = 0
        end
    end

    if @xpos > @line.length
        if goto_line(@ypos + 1)
            @xpos = 0
        else
            @xpos = @line.length
        end
    end

    @keep_x = @xpos unless [Key::UP, Key::DOWN].include?(key)

    @scrollpos = @ypos if @ypos < @scrollpos
    @scrollpos = @ypos - h if @ypos >= @scrollpos + h

end

def editor_draw(t)

    if @do_run
        @do_run = false
        run_source()
    end

    return if @running

    @con.clear
    lasty = @con.height/32-1
    @lines.length.times do |y|
        next if y >= lasty
        i = y + @scrollpos
        fg = Color::WHITE
        bg = 0
        fg = Color::GREY if @lines[i][0] == '#'.ord
        @con.text(0, y, @lines[i].pack('U*'), fg, bg) if i < @lines.length
    end 
    if @ypos >= @scrollpos
        @con.text(@xpos, @ypos - @scrollpos, @xpos >= @line.length ? " " :
                  @line[@xpos..@xpos].pack('U'), Color::WHITE, Color::ORANGE)
    end

    @con.bg = Color::RED
    @con.clear_line(lasty)
    @con.bg = 0
    @con.text(0, lasty, "LINE:#{@ypos+1} - F5 = Run - ESC = Exit",
              Color::WHITE, Color::RED);
end

def set_text(text)

    @lines = []
    text.split("\n").each do |line|
        p line
        @lines.append(line.unpack('U*'))
        p @lines.last
    end
end

def get_text()
    @lines.map { |l| l.pack('U*') }.join("\n")
end

def load(file)
    @file_name = file
    if File.exists?(file)
        @lines = File.open(file).readlines.map(&:chomp).map { |l| l.unpack("U*") }
    else
        @lines = [[]]
    end
    p @lines
end

def save()
    File.open(@file_name, "w+") do |f|
        @lines.each { |h| f.puts(h.pack("U*")) }
    end
end


def activate()

    @os_handlers = OS.get_handlers()
    OS.clear_handlers()

    @running = false
    @do_run = false

    @cut_line ||= ''

    @lines = [[]] if @lines.nil? or @lines.empty?
    @ypos = 0
    @keep_x = 0
    @xpos = 0
    @scrollpos = 0
    @con = Display.default.console
    @savex,@savey = @con.get_xy()

    @con.buffer(1)

    @line = @lines[0]

    @con.clear
    @con.goto_xy(0,0)
    @key_i = on_key { |key,mod| editor_key(key, mod) }
    @draw_i = on_draw { |t| editor_draw(t) }

    loop { Fiber.yield }

end


end
