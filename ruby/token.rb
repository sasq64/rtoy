# frozen_string_literal: true

require_relative 'observable'

# Represent a single boardgame token.
class Token
  extend Observable
  include Attributes

  attr_reader :sides, :id, :current_side, :name

  # Global list of all tokens
  @tokens = {}

  # @return [Array] Return all global tokens.
  def self.all
    @tokens.keys
  end

  # @param token [Token] Token to add to global list
  def self.add(token)
    @tokens[token] = true
    Token.signal(token, :add)
  end

  # @param token [Token] Token to remove from global list
  def self.remove(token)
    Token.signal(token, :remove)
    @tokens.delete(token)
  end

  # Create a new token
  # @param name [String] Name of token
  # @param sides [Integer] Number of sides token has
  # @param id [Integer] Id for sorting purposes
  def initialize(name = nil, sides: 1, id: nil)
    @name = name
    @sides = sides
    @id = id.nil? ? name : id
    @current_side = 0
    Token.add(self)
  end

  # @return [String] Name of token
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
    Token.signal(self, :turn_to, side)
    @current_side = side
  end
end
