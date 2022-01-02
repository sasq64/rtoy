# frozen_string_literal: true

require_relative './token'

# Represent a boardgame location that can hold tokens
class Location
  attr_accessor :name, :view, :active, :pile, :flip_to, :tokens

  @view_class = nil
  @locations = {}

  def self.view_class(cls)
    @view_class = cls
    @locations.each_key { |l| l.view = cls.new(l) if l.view.nil? }
  end

  def self.add(location)
    @locations[location] = true
  end

  def self.remove(location)
    @locations.delete location
  end

  def initialize(name = nil, tokens = [])
    @name = name
    Location.add(self)
    @view = @view_class.nil? ? nil : @view_class.new(self)
    @flip_to = nil
    @pile = false
    @active = false
    @tokens = tokens
  end

  def destroy
    @tokens.each(&:destroy)
    Location.remove(self)
    @tokens = []
  end

  # Move tokens from another location to this location
  def put(from:, from_index: 0, to_index: 0, count: 0)
    tokens = from
    if from.instance_of? Location
      tokens = from.tokens
    elsif from.class != Array
      tokens = [from]
    end
    count = tokens.size if count.zero?
    @tokens.insert(to_index, *tokens.slice!(from_index, count))
    @view&.event(self, :update)
  end

  # Create a temporary location with the top 'n' tokens of this
  # location
  def draw(count = 1)
    Location.new('Draw', @tokens.slice!(0...count))
  end

  def size
    @tokens.size
  end

  def [](index)
    @tokens[index]
  end

  def shuffle
    @tokens.shuffle!
    @view&.event(self, :shuffle)
  end

  def sort
    @tokens.sort! { |a, b| a.id <=> b.id }
  end

  def all
    @tokens
  end

  def first
    @tokens.first
  end

  def to_s
    a = tokens.map(&:name)
    a.to_s
  end
end
