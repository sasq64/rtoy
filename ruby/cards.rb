require_relative 'boardgame/boardgame'

# Bind View to Token
class TokenView
  include OS

  attr_reader :sprite

  def self.cards
    unless @cards
      img = Image.from_file('data/cards.png').trim(1)
      @cards = img.split(13, 4)
      @cards.each { |c| c.trim(0, 1, 1, 1) }
    end
    @cards
  end

  def initialize(token)
    @token = token
    @sprite = add_sprite(TokenView.cards[token.id])
    @queue = []
  end

  def destroy
    remove_sprite(@sprite)
  end

  def move_to(x, y, _rot = 0)
    p "Move to #{x} #{y}"
    @sprite.move(x, y)
  end
end

Token.listen do |token, what|
  case what
  when :add
    view = TokenView.new(token)
    token['view'] = view
  when :remove
    view = token.get('view')
    token['view'] = nil
    view.destroy
  end
end

# Bind View to Location
class LocationView
  def initialize(location)
    @location = location
    @x, @y, @dx, @dy = case location.name
                       when 'Deck'
                         [50, 50, 0.25 * 3, 0.25 * 3]
                       when 'Hand'
                         [10, 800, 100, 0]
                       else
                         [-1, -1, 0, 0]
                       end
    # update()
  end

  def update
    x = @x
    y = @y
    return if x == -1 && y == -1

    @location.tokens.each do |token|
      token['view'].move_to(x, y)
      x += @dx
      y += @dy
    end
  end
end

Location.listen do |location, what|
  if what == :add
    view = LocationView.new(location)
    location.set('view', view)
  elsif what == :remove
    view = location.get('view')
    location.set('view', nil)
    view.destroy
  end
end

board = Location.new('Board')
hand = Location.new('Hand')

suits = %w[hearts spades diamonds clubs]
names = %w[jack queen king ace]

# Token.view_class(TokenView)
# Location.view_class(LocationView)

deck = Location.new('Deck',
                    (0...52).map do |i|
                      suit = suits[i / 13]
                      j = i % 13
                      name = j >= 9 ? names[j - 9] : (j + 2).to_s
                      Token.new("#{name} of #{suit}", sides: 2, id: i)
                    end)

# deck.view.update()
deck.shuffle

puts deck

deck.sort

puts deck

hand.put(from: deck, from_index: 50, count: 2)

puts hand

temp = deck.draw(5)

puts temp

hand.put(from: temp, to_index: 1)

puts hand
puts hand.size

puts deck.size

vsync {}

# class Deck
#   def self.of_size(count)
#     d = Deck.new
#     d.cards = Array(1..count)
#     d
#   end

#   def self.with_cards(cards)
#     d = Deck.new
#     d.cards = cards
#     d
#   end

#   def cards=(a)
#     @cards = a
#   end

#   def cards
#     @cards
#   end

#   def shuffle
#     @cards.shuffle
#   end

#   def take(count)
#   end
# end

# x = 10
# deck.all.each { |t| t.sprite = add_sprite(cards[t.id]).move(x,x) ; x += 0.25 }

# deck.all.each { |t| tween(t.sprite).to(pos: Vec2.rand(1800,900)) }

# deck = Array(0...52).shuffle

# hand = deck.take(5)

# x = 0
# hand.each { |c| add_sprite(cards[c]).move(x, 0) ; x += 20 }
