

require 'minitest/autorun'
require_relative '../location.rb'

class LocationTest < MiniTest::Test

    def test_basic
        location = Location.new('board')
        assert_equal 'board', location.name
    end

    def test_tokens

        tokens = [
            Token.new('Red Pawn'),
            Token.new('Green Pawn'),
            Token.new('Blue Pawn'),
            Token.new('Yellow Pawn')
        ]

        board = Location.new('board', tokens)

        assert_equal board.tokens.size, 4
        assert_equal board.tokens[3].label, 'Yellow Pawn'

        board.sort
        assert_equal board.tokens[0].label, 'Blue Pawn'

    end

    def test_move
        tokens = [
            Token.new('A'),
            Token.new('B'),
            Token.new('C'),
            Token.new('D')
        ]

        board = Location.new('board', tokens)

        hand = Location.new('hand')


        hand.put(from: board, count: 2)

        assert_equal hand.size, 2
        assert_equal board.size, 2

        assert_equal hand[0].label, 'A'
        assert_equal hand[1].label, 'B'

        assert_equal board.tokens[0].label, 'C'
        assert_equal board.tokens[1].label, 'D'

        assert_equal 


    end

end
