require_relative 'token'

class Card < Token
  def initialize(name)
    super(name, sides: 2)
  end

  def self.make_deck
    suits = %w[clubs spades hearts diamond]
    names = %w[jack queen king ace]
    Location.new('Deck',
                 (0...52).map do |i|
                   suit = suits[i / 13]
                   j = i % 13
                   name = j >= 9 ? names[j - 9] : (j + 2).to_s
                   Token.new("#{name} of #{suit}", sides: 2, id: i)
                 end)
  end

end
