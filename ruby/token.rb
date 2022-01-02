# frozen_string_literal: true

# Represent a single boardgame token.
class Token
  attr_reader :sides, :id, :current_side, :view, :name

  @view_class = nil

  def self.view_class(cls)
    @view_class = cls
    @tokens.each_key { |t| t.view = cls.new(t) if t.view.nil? }
  end

  # All tokens
  @tokens = {}

  def self.all
    @tokens.keys
  end

  def self.add(token)
    @tokens[token] = true
  end
  
  def self.remove(token)
    @tokens.delete(token)
  end

  def initialize(name = nil, sides: 1, id: nil)
    p "#{name} created"
    @name = name
    @sides = sides
    @id = id.nil? ? name : id
    @view = @view_class.nil? ? nil : @view_class.new(self)
    @current_side = 0
    Token.add(self)
  end

  def label
    @name
  end

  def destroy
    p "#{name} destroyed"
    Token.remove(self)
  end

  def to_s
    label
  end

  def turn_to(side)
    @current_side = side
  end
end
