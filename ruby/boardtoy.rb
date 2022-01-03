# frozen_string_literal: true

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
    i = token.id
    i -= 13 if (i % 13) == 12
    i += 1
    @sprite = add_sprite(TokenView.cards[i])
  end

  def destroy
    remove_sprite(@sprite)
  end

  def move_to(x, y)
    p "Move to #{x} #{y}"
    @sprite.move(x, y)
  end

  def size
    @sprite.size
  end

  def inside?(x, y)
    x >= sprite.x && y >= sprite.y &&
      x < sprite.x + sprite.width && y < sprite.y + sprite.height
  end
end

Token.listen do |token, what|
  case what
  when :add
    view = TokenView.new(token)
    token.set('view', view)
  when :remove
    view = token.get('view')
    token.set('view', nil)
    view.destroy
  end
end

# Bind View to Location
class LocationView
  include OS

  def coordinates_for_location(location)
    case location.name
    when 'Deck'
      [50, 50, 0.25 * 3, 0.25 * 3]
    when 'Hand'
      [10, 800, 100, 0]
    else
      [-1, -1, 0, 0]
    end
  end

  def inside?(x, y)
    x >= @x && y >= @y && x < @end_pos.x && y < @end_pos.y
  end

  def click_handler(x, y)
    return unless inside?(x, y)

    p "CLICKED #{@location.name}"
    token = @location.tokens.reverse.find do |t|
      t.get('view').inside?(x, y)
    end
    p "TOKEN #{token.name}" if token
  end

  def initialize(location)
    @location = location
    @x, @y, @dx, @dy = coordinates_for_location(location)
    OS.on_click { |x, y| click_handler(x, y) }
    @end_pos = vec2(@x, @y)
  end

  def update
    return if @location.tokens.empty?

    x = @x
    y = @y
    @location.tokens.each do |token|
      token.get('view').move_to(x, y)
      x += @dx
      y += @dy
    end
    last_token = @location.tokens[-1]
    @end_pos = vec2(x, y) + last_token.get('view').size
  end
end

Location.listen do |location, what|
  case what
  when :add
    view = LocationView.new(location)
    location.set('view', view)
  when :remove
    view = location.get('view')
    location.set('view', nil)
    view.destroy
  when :update
    location.get('view').update
  when :shuffle
    location.get('view').update
  end
end
