require 'readline.rb'

class Repl

    include OS

    p "REPL"
    def run_repl()
        p "RUN"
        clear()
        puts "R-Toy READY. Type 'help' if you need it."
        while true do
            print('→')
            line = IOX.read_line()
            puts ""
            begin
                exec line
            rescue => e
                puts "Error " + e.message
            end
            x,y = Display.default.console.get_xy()
            puts("") if x > 0
        end
    end
end
