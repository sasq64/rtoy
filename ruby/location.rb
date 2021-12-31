require_relative './token.rb'

class Location

    attr_accessor :name, :view, :active, :pile, :flip_to, :tokens

    @@view_class = nil
    @@locations = {}

    def self.view_class(cls)
        @@view_class = cls
        @@locations.keys.each { |l| l.view = cls.new(l) if l.view.nil? }
    end

    def initialize(name = nil, tokens = [])
        @name = name
        @@locations[self] = true
        @view = @@view_class.nil? ? nil : @@view_class.new(self)
        @flip_to = nil
        @pile = false
        @active = false
        @tokens = tokens
    end

    def to_s
        return @name
    end

    # Move tokens from another location to this location
    def put(from:, from_index: 0, to_index: 0, count: 0)

        p from
        p from.class
        tokens = from
        if from.class == Location
            tokens = from.tokens
        elsif from.class != Array
            tokens = [from]
        end
        count = tokens.size() if count == 0
        @tokens.insert(to_index, *tokens.slice!(from_index, count))
        @view&.event(self, :update)
    end

    # Create a temporary location with the top 'n' tokens of this
    # location
    def draw(n = 1)
        Location.new('Draw', @tokens.slice!(0...n))
    end

    def size()
        @tokens.size
    end

    def [](i)
        @tokens[i]
    end

    def shuffle()
        @tokens.shuffle!
        @view&.event(self, :shuffle)
    end

    def sort()
        @tokens.sort! { |a,b| a.id <=> b.id }
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

