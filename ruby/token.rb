
class Token

    attr_accessor :sides, :id, :current_side, :view, :name

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

    def initialize(name = nil, sides: 1, id: nil)
        @name = name
        @sides = sides
        @id = id
        @id = name if @id.nil?
        @view = @@view_class.nil? ? nil : @@view_class.new(self)
        @current_side = 0
        @@tokens[self] = true 
    end

    def label()
        @name
    end

    def to_s
        label
    end

    def turn_to(side)
        @current_side = side
        @view&.turn_to(side)
        @view&.event(self, :side)
    end

end

