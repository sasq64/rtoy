require 'readline.rb'

class Repl

    include OS

    def repl_run()
        clear()
        puts "R-Toy READY. Type 'help' if you need it."
        _repl_b = binding
        rl = LineReader.new('toy.history')
        while true do
            print('→')
            line = rl.read_line()
            puts ""
            begin
                f = Fiber.new { _repl_b.eval(line) }
                while f.alive? do
                    f.resume
                    mods = Input.default.get_modifiers()
                    if Input.default.get_key('c'.ord)
                        break if mods & 0xc0 != 0
                    end
                    Fiber.yield if f.alive?
                end
            rescue => e
                puts "Error " + e.message
            end
            x,y = Display.default.console.get_xy()
            puts("") if x > 0
        end
    end
end
