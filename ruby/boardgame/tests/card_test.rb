require 'minitest/autorun'
require_relative '../location'

# Test for locations
class CardTest < MiniTest::Test
  def make_deck
    suits = %w[hearts spades diamonds clubs]
    names = %w[jack queen king ace]
    Location.new('Deck',
                 (0...52).map do |i|
                   suit = suits[i / 13]
                   j = i % 13
                   name = j >= 9 ? names[j - 9] : (j + 2).to_s
                   Token.new("#{name} of #{suit}", sides: 2, id: i)
                 end)
  end

  def test_cards
    hand = Location.new('Hand')
    deck = make_deck
    deck.shuffle
    hand.put(from: deck, count: 5)
    assert_equal 5, hand.size
    assert_equal 47, deck.size
  end
end
