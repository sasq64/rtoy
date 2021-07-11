require 'readline.rb'

class Repl

    include OS

    def run()
        clear()
        puts "R-Toy READY. Type 'help' if you need it."
        _repl_b = binding
        rl = LineReader.new('toy.history')
        while true do
            print('→')
            line = rl.read_line()
            puts ""
            begin
                _repl_b.eval(line)
            rescue => e
                puts "Error " + e.message
            end
            x,y = Display.default.console.get_xy()
            puts("") if x > 0
        end
    end
end
