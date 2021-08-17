require 'os.rb'
require 'repl.rb'
require 'editor.rb'
require 'turtle.rb'

OS.reset_handlers()
Display.default.clear()

cmd = Settings::BOOT_CMD
OS.boot {
    if cmd && !cmd.empty?
        eval(cmd)
    else
        Repl.new.repl_run()
    end
}


