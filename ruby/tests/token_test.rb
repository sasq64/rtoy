require 'minitest/autorun'
require_relative '../token'

# Test Tokens
class TokenTest < MiniTest::Test

  # Test that we can create a token
  def test_basic
    token = Token.new('card')
    assert_equal 'card', token.label
    assert_equal token.current_side, 0
  end

  def test_sides
    token = Token.new('dice', sides: 6)
    token.turn_to(3)
    assert_equal token.current_side, 3
  end
end
