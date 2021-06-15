require 'os.rb'
require 'repl.rb'
require 'editor.rb'
require 'turtle.rb'

OS.reset_handlers()
Display.default.clear()

OS.boot { Repl.new.run_repl() }


