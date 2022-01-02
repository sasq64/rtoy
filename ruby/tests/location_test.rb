require 'minitest/autorun'
require_relative '../location'

# Test for locations
class LocationTest < MiniTest::Test

  def test_basic
    location = Location.new('board')
    assert_equal 'board', location.name
  end

  def test_tokens
    assert_equal 0, Token.all.size
    tokens = [
      Token.new('Red Pawn'),
      Token.new('Green Pawn'),
      Token.new('Blue Pawn'),
      Token.new('Yellow Pawn')
    ]

    board = Location.new('board', tokens)

    assert_equal 4, board.tokens.size
    assert_equal 'Yellow Pawn', board.tokens[3].label

    board.sort
    assert_equal 'Blue Pawn', board.tokens[0].label

    assert_equal 4, Token.all.size
    board.destroy
    assert_equal 0, Token.all.size
  end

  def test_move
    assert_equal 0, Token.all.size
    tokens = [
      Token.new('A'),
      Token.new('B'),
      Token.new('C'),
      Token.new('D')
    ]

    board = Location.new('board', tokens)

    hand = Location.new('hand')

    p Token.all.size
    hand.put(from: board, count: 2)

    p Token.all.size
    assert_equal 2, hand.size
    assert_equal 2, board.size

    assert_equal 'A', hand[0].label
    assert_equal 'B', hand[1].label

    assert_equal 'C', board.tokens[0].label
    assert_equal 'D', board.tokens[1].label

    assert_equal 4, Token.all.size
    board.destroy
    assert_equal 2, Token.all.size
    hand.destroy
    assert_equal 0, Token.all.size
  end

end
