require 'readline.rb'

class Repl

    include OS

    p "REPL"
    def run()
        p "RUN"
        clear()
        puts "R-Toy READY. Type 'help' if you need it."
        while true do
            print('→')
            line = IOX.read_line()
            puts ""
            begin
                p "### EXEC START"
                bexec line
                p "### EXEC DONE"
            rescue => e
                puts "Error " + e.message
            end
            #return if OS.running?
            p "SUCCESS"
            x,y = Display.default.console.get_xy()
            p "#{x},#{y}"
            puts("") if x > 0
        end
    end

end
