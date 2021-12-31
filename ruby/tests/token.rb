
require 'minitest/autorun'
require_relative '../token.rb'

class TokenTest < MiniTest::Test

    def test_basic
        token = Token.new('card')
        assert_equal 'card', token.label
    end

    def test_sides
        token = Token.new('dice', sides: 6)
        assert_equal token.current_side, 0
        token.turn_to(3)
        assert_equal token.current_side, 3
    end
end
