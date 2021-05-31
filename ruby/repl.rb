require 'readline.rb'

class Repl

    include OS

    def run_repl()
        clear()
        puts "R-Toy READY. Type 'help' if you need it."
        while true do
            print('→')
            line = IOX.read_line(history: "toy.history")
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
