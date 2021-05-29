# Will hook up handlers but not pollute namespace with
# 'global methods'
require 'os.rb'
require 'repl.rb'
require 'editor.rb'
require 'turtle.rb'

OS.reset_handlers()

load('ruby/user.rb')

p "CLEAR"
Display.default.clear()

p "RUN"
repl = Repl.new
repl.run()


