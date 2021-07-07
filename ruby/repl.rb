require 'readline.rb'

class Repl

    include OS

    def run_repl()
        clear()
        puts "R-Toy READY. Type 'help' if you need it."
        _repl_b = binding
        while true do
            print('→')
            line = IOX.read_line(history: "toy.history")
            puts ""
            begin
                #block = Proc.new { 
                    _repl_b.eval(line)
                #}
                #f = Fiber.new(&block)
                #while f.alive? do
                #    f.resume
                #    Fiber.yield if f.alive?
                #end
            rescue => e
                puts "Error " + e.message
            end
            x,y = Display.default.console.get_xy()
            puts("") if x > 0
        end
    end
end
