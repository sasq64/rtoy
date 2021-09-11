require 'os.rb'
require 'tween.rb'
require 'repl.rb'
require 'editor.rb'
require 'turtle.rb'

require 'linux.rb'
require 'keymap.rb'

OS.reset_handlers()
Display.default.clear()

if Settings::SYSTEM == :raspberry_pi
    Display.default.mouse_ptr(Image.from_file("data/pointer.png"))
end

cmd = Settings::BOOT_CMD
OS.boot {
    if cmd && !cmd.empty?
        eval(cmd)
    else
        Repl.new.repl_run()
    end
}


