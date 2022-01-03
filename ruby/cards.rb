require_relative 'boardgame/boardgame'
require_relative 'boardtoy'

hand = Location.new('Hand')
deck = Card.make_deck
deck.shuffle

hand.put(from: deck, from_index: 50, count: 2)
puts hand

temp = deck.draw(5)
puts temp

hand.put(from: temp, to_index: 1)
puts hand

vsync {}
