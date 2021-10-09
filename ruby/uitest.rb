
require 'os.rb'

require 'ui.rb'
require 'vec2.rb'
require 'tween.rb'

class TestUI

    def initialize()
        Display.default.clear()
        @frame = UI::Frame.new
        @button = UI::Button.new([10,10], [100,20], "Click me") {
            p "CLICKED"
        }
        @frame.add_element(@button)
    end
end

ui = TestUI.new
loop { vsync }
