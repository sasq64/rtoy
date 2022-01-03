# frozen_string_literal: true

require_relative 'token'
require_relative 'observable'

# Represent a boardgame location that can hold tokens
class Location
  extend Observable

  attr_accessor :name, :active, :pile, :flip_to, :tokens

  @locations = {}

  def self.add(location)
    @locations[location] = true
  end

  def self.remove(location)
    @locations.delete location
  end

  # Create new Boardgame location, such as a _Deck_ of cards
  # or the _board.
  # @param name String Name of location
  # @param tokens [Array<Token>] List of initial tokens for location
  def initialize(name = nil, tokens = [])
    @name = name
    Location.add(self)
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
  # @param from 
  def put(from:, from_index: 0, to_index: 0, count: 0)
    tokens = from.tokens
    count = tokens.size if count.zero?
    @tokens.insert(to_index, *tokens.slice!(from_index, count))
    Location.signal(self, :update)
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
    Location.signal(self, :shuffle)
  end

  def each 
    @tokens.each
  end
  
  def sort
    @tokens.sort! { |a, b| a.id <=> b.id }
  end

  def sort!
    sort
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
