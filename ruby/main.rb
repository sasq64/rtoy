
module Kernel
    def puts(txt)
        Display.default.console.print(txt.to_s + "\n")
    end
    def print(txt)
        Display.default.console.print(txt.to_s)
    end
end

# Will hook up handlers but not pollute namespace with
# 'global methods'
require 'os.rb'

p "CLEAR"
Display.default.clear()

require 'repl.rb'
require 'editor.rb'

load('ruby/user.rb')


p "RUN"
repl = Repl.new
repl.run()


