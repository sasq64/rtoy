require 'readline.rb'

class Repl

    include OS

    def run()
        @readline = ReadLine.new
        clear()
        puts "R-Toy READY. Type 'help' if you need it."
        print('→')
        @readline.start()
        @readline.read_line { |l| eval_line(l) }
    end

    def eval_line(l)
        puts ""
        begin
            eval l
        rescue => e
            puts "Error " + e.message
        end
        p "SUCCESS"
        x,y = Display.default.console.get_xy()
        p "#{x},#{y}"
        puts("") if x > 0
        print("→")
        @readline.read_line { |l| eval_line(l) }
    end

end
