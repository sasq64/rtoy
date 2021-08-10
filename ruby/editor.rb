
module OS
    @@current_file = "scratch.rb"
    def edit(f = nil)
        f = @@current_file if not f
        @@current_file = f
        ed = Editor.new
        ed.load(f)
        ed.run_editor()
    end
    module_function :edit
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

        p "SAVE HANDLERS"
        @saved_handlers = OS.get_handlers()
        OS.clear_handlers()
        on_key do |key|
            if key == Key::F5
                @quit_app = true
                @dirty = true
            end
        end

        @quit_app = false
        begin
            p "EDITOR EVAL"
            @dirty = false
            OS.run(@file_name, clear: false) { @quit_app }
            Fiber.yield while !@quit_app
            p "EVAL DONE"
            @quit_app = false
            reset_display()
            clear()
            OS.set_handlers(@saved_handlers)
        rescue Exception => e
            OS.clear_handlers()
            clear()
            reset_display()
            display.console.goto_xy(0,0)
            puts "#{e.to_s}"
            e.backtrace.each { |bt| 
                file,line = bt.split(":")
                puts "  #{file}:#{line}"
            }
            @quit_app = false
            OS.set_handlers(@saved_handlers)
            @dirty = false
        end

    end

    def editor_key(key, mod)

        h = @con.visible_rows-2
        @dirty = true
        case key
        when 0x20..0xffffff
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
                    @cut_line = @line.dup
                    @lines.delete_at(@ypos)
                    goto_line(@ypos)
                end
                if key == 'p'.ord
                    @lines.insert(@ypos, @cut_line.dup)
                    goto_line(@ypos)
                end
            else
                @line.insert(@xpos, key)
                @xpos += 1
            end
        when Key::INSERT
            if mod & 1
                text = System.get_clipboard()
                text.each_char do |t|
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
            @do_quit = true
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

        return unless @dirty
        @dirty = false

        @con.clear
        count = @con.visible_rows-1
        count.times do |y|
            i = y + @scrollpos
            break if i >= @lines.length
            fg = Color::WHITE
            bg = Color::TRANSP
            fg = Color::GREY if @lines[i][0] == '#'.ord
            @con.text(0, y, @lines[i].pack('U*'), fg, bg) if i < @lines.length
        end 

        if @ypos >= @scrollpos
            @con.text(@xpos, @ypos - @scrollpos, @xpos >= @line.length ? " " :
                      @line[@xpos..@xpos].pack('U'), Color::WHITE, Color::ORANGE)
        end

        @con.bg = Color::RED
        @con.clear_line(count)
        @con.bg = Color::TRANSP
        @con.text(0, count, "LINE:#{@ypos+1} - F5 = Run - ESC = Exit",
                  Color::WHITE, Color::RED);
    end

    def set_text(text)

        @lines = []
        text.split("\n").each do |line|
            @lines.append(line.unpack('U*'))
        end
    end

    def get_text()
        @lines.map { |l| l.pack('U*') }.join("\n")
    end

    def load(file)
        p "Editor load"
        @file_name = file
        if File.exists?(file)
            @lines = File.open(file).readlines.map(&:chomp).map { |l| l.unpack("U*") }
        else
            @lines = [[]]
        end
    end

    def save()
        File.open(@file_name, "w+") do |f|
            @lines.each { |h| f.puts(h.pack("U*")) }
        end
    end

    def run_editor()

        @os_handlers = OS.get_handlers()
        OS.clear_handlers()

        @dirty = true
        @do_run = false
        @do_quit = false

        @cut_line ||= ''

        @lines = [[]] if @lines.nil? or @lines.empty?
        @ypos = 0
        @keep_x = 0
        @xpos = 0
        @scrollpos = 0
        @con = Display.default.console
        @savex,@savey = @con.get_xy()

        #@con.buffer(1)

        @line = @lines[0]

        @con.clear
        @con.goto_xy(0,0)
        @key_i = on_key { |key,mod| editor_key(key, mod) }
        @draw_i = on_draw { |t| editor_draw(t) }

        loop do
            p "WAIT"
            Fiber.yield while !@do_run and !@do_quit
            break if @do_quit
            p "RUN"
            @do_run = false
            #@con.buffer(0)
            run_source()
            #@con.buffer(1)
        end
        OS.set_handlers(@os_handlers)
        reset_display()
        clear()

    end


end
