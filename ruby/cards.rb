class Token

    attr_accessor :sides, :id, :current_side, :view, :name, :game, :before, :after

    @@view_class = nil

    def self.view_class(cls)
        @@view_class = cls
        @@tokens.keys.each { |t| t.view = cls.new(t) if t.view.nil? }
    end

    # All tokens
    @@tokens = {}

    def self.all()
        @@tokens.keys
    end

    def initialize(name = nil, sides = 1, id = nil)
        @name = name
        @sides = sides
        @id = id
        @view = @@view_class.nil? ? nil : @@view_class.new(self)
        @current_side = 0
        Location.void.add(self)
        @@tokens[self] = true 
    end

    def location()
        @location
    end

    def location=(l)
        @location = l
        @view&.event(self, :location)
    end

    def label()
        @name
    end

    def to_s
        label
    end

    def turn_to(side)
        @view&.turn_to(side)
        @view&.event(self, :side)
    end

    def move_to(loc)
        loc.add self
    end

end

class Location


    attr_accessor :name, :view, :active, :pile, :flip_to, :tokens

    @@view_class = nil
    @@locations = {}

    def self.view_class(cls)
        @@view_class = cls
        @@locations.keys.each { |l| l.view = cls.new(l) if l.view.nil? }
    end

    def initialize(name = nil)
        @name = name
        @@locations[self] = true
        @view = @@view_class.nil? ? nil : @@view_class.new(self)
        @flip_to = nil
        @pile = false
        @active = false
        @tokens = []
    end

    def to_s
        return @name
    end

    def add(tok)

        if tok.class == Location
            tok = tok.tokens.dup
        end

        tok = [ tok ] unless tok.class == Array

        for t in tok
            fail unless t.class == Token
            from = t.location
            t.location = self
            if from 
                from.tokens.delete(t)
            end
            t.turn_to @flip_to unless @flip_to.nil?
            @tokens << t
        end
        @view&.event(self, :update)
    end

    def draw(n = 1)
        new_loc = Location.new('Draw')
        new_loc.tokens = @tokens.slice!(0...n)
        puts @tokens.size
        new_loc.tokens.each { |t| t.location = new_loc }
        puts new_loc.tokens.size
        new_loc
    end

    def shuffle
        @tokens.shuffle!
        @view&.event(self, :shuffle)
    end

    def all()
        @tokens
    end

    def first()
        @tokens.first
    end

    @@void = Location.new("Void")
    def self.void()
        @@void
    end

    def contents
        a = tokens.map { |t| t.name }
        a.to_s
    end
end

class Card < Token

    attr_accessor :state

    def initialize(name = nil)
        super(name, 2)
    end
end

module Renderable
    def sprite()
        return @sprite
    end

    def sprite=(s)
        @sprite = s
    end
end

#class Token
#    include Renderable
#end


class TokenView
    include OS
    img = Image.from_file('data/cards.png').trim(1)
    @@cards = img.split(13, 4)
    @@cards.each { |c| c.trim(0,1,1,1) }

    def initialize(token)
        @token = token
        @sprite = add_sprite(@@cards[token.id])
        @queue = []
    end

    def sprite
        @sprite
    end


    def move_to(x, y, rot = 0)
        p "Move to #{x} #{y}"
        if x != @tx || y != @ty
            @tx,@ty,@rot = x,y,rot
            @queue << [x,y,rot]
        end
        update()
    end

    def update()
        unless @current_tween
            if @queue.size > 0
                x,y,rot = @queue.shift
                p "Tween #{@token.name} to #{x}, #{y}"
                #@sprite.move(x,y)
                @current_tween = tween(@sprite).to(pos: [x,y]).
                    to(rotation: rot).when_done {
                        @current_tween = nil ; update() }
            end
        end
    end

    def event(token, evt)
        if evt == :location
            puts "## Moving #{token.name} to >> #{token.location.name}\n"
        end
    end
end

class LocationView
    def initialize(location)
        @location = location
        @x,@y,@dx,@dy = case location.name
        when 'Deck'
            [50,50,0.25*3,0.25*3]
        when 'Hand'
            [10,800,100,0]
        else
            [-1,-1,0,0]
        end
        #update()
    end

    def update()
        x = @x
        y = @y
        return if (x == -1 && y ==-1)
        @location.tokens.each do |token|
            token.view.move_to(x, y)
            x += @dx
            y += @dy
        end
    end

    def event(location, evt)
        if evt == :shuffle
            @location.tokens.each do |token|
                sprite = token.view.sprite
                p = sprite.pos
                p2 = p + Vec2.rand(20,20)
                #[p[0] + rand(20) - 10, p[1] + rand(20 -10)]
                #token.view.move_to(p2[0], p2[1], Math::PI)
                #token.view.move_to(p[0], p[1], 2 * Math::PI)
                #tween(sprite).to(pos: p2).to(rotation: Math::PI).when_done { tween(sprite).to(pos: p).to(rotation: Math::PI * 2) }
            end
            puts "## Shuffling #{location.name}"
        elsif evt == :update
            update()
        end
    end
end

board = Location.new('Board')
deck = Location.new('Deck')
hand = Location.new('Hand')

suits = ['hearts', 'spades', 'diamonds', 'clubs']
names = ['jack', 'queen', 'king']

Token.view_class(TokenView)
Location.view_class(LocationView)

tokens = (0...52).map { |i|
    suit = suits[i / 13]
    j = i % 13
    name = j >= 10 ? names[j-10] : (j+1).to_s
    name = 'ace' if j == 0
    Token.new("#{name} of #{suit}", 2, i)
}

deck.add(tokens)
deck.view.update()
deck.shuffle()

temp = deck.draw(5)

puts temp.contents

hand.add(temp)

puts hand.tokens.size

puts deck.tokens.size

vsync {}



# class Deck
#     def self.of_size(count)
#         d = Deck.new
#         d.cards = Array(1..count)
#         d
#     end

#     def self.with_cards(cards)
#         d = Deck.new
#         d.cards = cards
#         d
#     end

#     def cards=(a)
#         @cards = a
#     end

#     def cards
#         @cards
#     end

#     def shuffle
#         @cards.shuffle
#     end

#     def take(count)
#     end
# end


#x = 10
#deck.all.each { |t| t.sprite = add_sprite(cards[t.id]).move(x,x) ; x += 0.25 }

#deck.all.each { |t| tween(t.sprite).to(pos: Vec2.rand(1800,900)) }

# deck = Array(0...52).shuffle

# hand = deck.take(5)

# x = 0
# hand.each { |c| add_sprite(cards[c]).move(x, 0) ; x += 20 }

