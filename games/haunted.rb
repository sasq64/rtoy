class HauntedHouse
  @@default_flags = Array.new(36).each_with_index.map { |x, i| [18, 17, 2, 26, 28, 23].include?(i) }
  attr_reader :objects, :descriptions, :message, :flags, :room

  def self.default_flags
    @@default_flags
  end

  def initialize(start_room=57, flags=HauntedHouse.default_flags, carrying=[])
    @room = start_room
    @flags = flags.dup
    @carrying = carrying.dup
    @verbs = [
      nil, "HELP", "CARRYING?", "GO", "N", "S", "W", "E", "U", "D", "GET", "TAKE", "OPEN", "EXAMINE", "READ", "SAY",
      "DIG", "SWING", "CLIMB", "LIGHT", "UNLIGHT", "SPRAY", "USE", "UNLOCK", "LEAVE", "SCORE"
    ]

    @routes = [
      "SE", "WE", "WE", "SWE", "WE", "WE", "SWE", "WS",
      "NS", "SE", "WE", "NW", "SE", "W", "NE", "NSW",
      "NS", "NS", "SE", "WE", "NWUD", "SE", "WSUD", "NS",
      "N", "NS", "NSE", "WE", "WE", "NSW", "NS", "NS",
      "S", "NSE", "NSW", "S", "NSUD", "N", "N", "NS",
      "NE", "NW", "NE", "W", "NSE", "WE", "W", "NS",
      "SE", "NSW", "E", "WE", "NW", "S", "SW", "NW",
      "NE", "NWE", "WE", "WE", "WE", "NWE", "NWE", "W"
    ]

    @descriptions = [
      "Dark Corner", "Overgrown Garden", "By a Large Wood Pile", "Yard by Rubbish",
      "Weed Patch", "Forest", "Thick Forest", "Blasted Tree",
      "Corner of the House", "Entrance to the Kitchen", "Kitchen and Grimy Cooker", "Scullery Door",
      "Room with Inches of Dust", "Rear Turret Room", "Clearing by House", "Path",
      "Side of the House", "Back of the Hallway", "Dark Alcove", "Small Dark Room",
      "Bottom of a Spiral Staircase", "Wide Passage", "Slippery Steps", "Clifftop",
      "Near a Crumbling Wall", "Gloomy Passage", "Pool of Light", "Impressive Vaulted Hallway",
      "Hall by a Thick Wooden Door", "Trophy Room", "Cellar with Barred Window", "Cliff Path",
      "Cupboard with Hanging Coat", "Front Hall", "Sitting Room", "Secret Room",
      "Steep Marble Stairs", "Dining Room", "Deep Cellar with a Coffin", "Cliff Path",
      "Closet", "Front Lobby", "Library of Evil Books", "Study with a Desk and Hole in the Wall",
      "Weird Cobwebby Room", "Very Cold Chamber", "Spooky Room", "Cliff Path by the Marsh",
      "Rubble-Strewn Verandah", "Front Porch", "Front Tower", "Sloping Corridor",
      "Upper Gallery", "Marsh by a Wall", "Marsh", "Soggy Path",
      "By a Twisted Railing", "Path through an Iron Gate", "By Railings", "Beneath the Front Tower",
      "Debris from Crumbling Facade", "Large Fallen Brick Work", "Rotting Stone Arch", "Crumbling Clifftop"
    ]

    @objects = [
      nil, "PAINTING", "RING", "MAGIC SPELLS", "GOBLET", "SCROLL", "COINS", "STATUE", "CANDLESTICK",
      "MATCHES", "VACUUM", "BATTERIES", "SHOVEL", "AXE", "ROPE", "BOAT", "AEROSOL", "CANDLE", "KEY",
      "NORTH", "SOUTH", "WEST", "EAST", "UP", "DOWN",
      "DOOR", "BATS", "GHOSTS", "DRAWER", "DESK", "COAT", "RUBBISH",
      "COFFIN", "BOOKS", "XZANFAR", "WALL", "SPELLS"
    ]

    # Locations for gettable objects
    @locations = [
      nil, 46, 38, 35, 50, 13, 18, 28, 42, 10, 25, 26, 4, 2, 7, 47, 60, 43, 32
    ]
    @light_limit = 60
    @message = "Ok"
  end

  def show_location
    show_room
    show_objects
    puts "============================"
    puts @message
    @message = "What?"
    puts "What will you do now?"
    parse(OS.gets)
    Fiber.yield
    candle
  end

  def parse(input)
    verb, word = get_verb_word(input)
    vi, wi = get_verb_word_index(verb, word)
    word = word.split(' ').map { |w| w.downcase.capitalize }.join(' ') unless word.nil?
    @message = get_message(verb, word, vi, wi)
    return if bats(vi)
    ghosts
    @message == "" if [1, 2, 16, 20, 25].include?(vi)
    display_help if vi == 1
    display_carrying if vi == 2
    if (3..9).include?(vi)
      move(vi, wi)
    end
    get_take(wi) if vi == 10 || vi == 11
    open(wi) if vi == 12
    examine(wi) if vi == 13
    read(wi) if vi == 14
    say(word, wi) if vi == 15
    dig if vi == 16
    swing(wi) if vi == 17
    climb(wi) if vi == 18
    light(wi) if vi == 19
    unlight if vi == 20
    spray(wi) if vi == 21
    use(wi) if vi == 22
    unlock(wi) if vi == 23
    leave(wi) if vi == 24
    score if vi == 25
    return vi, wi
  end

  def get_verb_word(question)
    if question.length > 0
      question = question.split
      question.map! { |q| q.strip.upcase }
      verb = question[0]
      word = ""
      if question.length > 1
        question.shift
        word = question.join(' ')
      end
      return verb, word
    end
    return "", ""
  end

  def bats(vi)
    has_bats = @flags[26] && @room == 13 && Random.new.rand(3) != 2 && vi != 21
    @message = "Bats Attacking!" if has_bats
    has_bats
  end

  def ghosts
    @flags[27] = true if @room == 44 && Random.new.rand(2) == 1 && !@flags[24]
  end

  def candle
    @light_limit -= 1 if @flags[0]
    flags[0] = false if @light_limit < 1
    @message += "\nYour candle is waning!" if @light_limit == 10
    @message += "\nYour candle is out" if @light_limit == 1
  end

  def get_verb_word_index(verb, word)
    vi = @verbs.index(verb)
    wi = @objects.index(word)
    return vi, wi
  end

  def get_message(verb, word, vi, wi)
    return "That's silly" if !vi.nil? && wi.nil? && !word.nil? && !word.empty?
    return "I need two words" if !word.nil? && word.empty?
    return "You don't make sense" if vi.nil? && wi.nil?
    return "You can't '#{verb} #{word}'" if vi.nil? && !wi.nil?
    return "You don't have #{word}" if !vi.nil? && !wi.nil? && wi > 0 && vi > 9 && !@carrying[wi]
  end

  def display_help
    display_list("Words I know:", @verbs.select { |x| !x.nil? })
  end

  def display_carrying
    display_list("You are carrying:", create_carrying(@carrying, @objects))
  end

  def create_carrying(carrying, all_objects)
    objects = []
    carrying.each_with_index do |c, index|
      objects << all_objects[index] if c
    end
    objects
  end

  def display_list(message, list)
    puts "#{message}"
    list.each_with_index do |v, index|
      print "#{v}"
      print "," if index < list.length - 1
    end
    press_enter_to_continue
  end

  def move(vi, wi)
    change = movement(vi, wi)
    if @flags[14]
      @flags[14] = false
      @message = "Crash! You fell out of a tree!"
    elsif @flags[27] && @room == 52
      @message = "Ghosts will not let you move."
    elsif @room == 45 && @carrying[1] && !@flags[34]
      @message = "A magical barrier to the west."
    elsif @room == 54 && !@carrying[15]
      @message = "You're stuck!"
    elsif @carrying[15] and !([53, 54, 55, 47].include?(@room))
      @message = "You can't carry a boat!"
    elsif (27..29).include?(@room) && !@flags[0]
      @message = "Too dark to move."
    elsif @room == 26 && !@flags[0] && (change == -8 || change == 1)
      @message = "You need a light."
    elsif vi == 3 && wi.nil?
      @message = "Go where?"
    else
      if change != 0
        @room += change
        @message = "Ok"
        if @room == 41 && @flags[23]
          @routes[49] = "SW"
          @message = "The door slams shut!"
          @flags[23] = false
        end
      else
        @message = "Can't go that way!"
      end
    end
  end

  def movement(vi, wi)
    change = 0
    direction = 0
    direction = vi - 3 if wi.nil?
    direction = wi - 18 if !wi.nil? && !vi.nil? && vi == 3
    direction = 1 if direction == 5 && @room == 20
    direction = 3 if direction == 6 && @room == 20
    direction = 3 if direction == 5 && @room == 22
    direction = 2 if direction == 6 && @room == 22
    direction = 2 if direction == 5 && @room == 36
    direction = 1 if direction == 6 && @room == 36
    @routes[@room].chars.each do |c|
      change = -8 if c.eql?("N") && direction == 1
      change = 8 if c.eql?("S") && direction == 2
      change = -1 if c.eql?("W") && direction == 3
      change = 1 if c.eql?("E") && direction == 4
    end
    change
  end

  def get_take(wi)
    return if wi.nil?
    if wi > 18
      @message = "I can't get #{@objects[wi]}"
    else
      @message = "It isn't here" if @locations[wi] != @room
      @message = "What #{@objects[wi]}?" if @flags[wi]
      @message = "You already have it" if @carrying[wi]
      if wi > 0 && wi < 19 && @locations[wi] == @room && !@flags[wi]
        @carrying[wi] = true
        @locations[wi] = 65
        @message = "You have the #{@objects[wi]}"
      end
    end
  end

  def open(wi)
    if @room == 43 && (wi == 28 || wi == 29)
      @flags[17] = false
      @message = "Drawer open."
    elsif @room == 28 and wi == 25
      @message = "It's locked."
    elsif @room == 38 and wi == 32
      @message = "That's creepy!"
      @flags[2] = false
    end
  end

  def examine(wi)
    @message = "There is a drawer." if wi == 28 || wi == 29
    if wi == 30
      @flags[18] = false
      @message = "Something here!"
    end
    @message = "That's disgusting!" if wi == 31
    open(wi) if wi == 32
    read(wi) if wi == 33 || wi == 5
    @message = "There's something beyond..." if @room == 43 && wi == 35
  end

  def read(wi)
    @message = "They are demonic works." if @room == 42 && wi == 33
    @message = "Use this word with care 'Xzanfar'." if (wi == 3 || wi == 36) && @carrying[3] && !@flags[34]
    @message = "The script is in an alien tongue." if @carrying[5] && wi == 5
  end

  def say(word, wi)
    @message = "Ok #{word}"
    if @carrying[3] && wi == 34
      @message = "*Magic Occurs*"
      if @room == 45
        @flags[34] = true
      else
        @room = Random.new.rand(64)
      end
    end
  end

  def dig
    if @carrying[12]
      if @room == 30
        @message = "Dug the bars out."
        @descriptions[@room] = "Hole in the wall."
        @routes[@room] = "NSE"
      else
        @message = "You've made a hole."
      end
    end
  end

  def swing(wi)
    if @carrying[14]
      @message = @room == 7 ? "This is no time to play games." : "You swung it"
    elsif @carrying[13] && wi == 13
      if @room == 43
        @descriptions[@room] = "Study with a secret room."
        @routes[@room] = "WN"
        @message = "You broke the thin wall."
      else
        @message = "Whoosh"
      end
    end
  end

  def climb(wi)
    if wi == 14
      if @carrying[14]
        @message = "It isn't attached to anything!"
      elsif !@carrying[14] && @room == 7
        if @flags[14]
          @message = "Going down!"
          @flags[14] = false
        else
          @message = "You see a thick forest and a cliff south."
          @flags[14] = true
        end
      end
    end
  end

  def light(wi)
    if wi == 17 && @carrying[17]
      @message = "It will burn your hands." if !@carrying[8]
      @message = "Nothing to light it with." if !@carrying[9]
      if @carrying[8] && @carrying[9]
        @message = "It casts a flickering light."
        @flags[0] = true
      end
    end
  end

  def unlight
    if @flags[0]
      @flags[0] = false
      @message = "Extinguished."
    end
  end

  def spray(wi)
    if wi == 26 && @carrying[16]
      if @flags[26]
        @flags[26] = false
        @message = "Pfft! Got them."
      else
        @message = "Hissss"
      end
    end
  end

  def use(wi)
    if wi == 10 and @carrying[10] & @carrying[11]
      @message = "Switched on."
      @flags[24] = true
    end
    if @flags[27] && @flags[24]
      @message = "Whizz -- Vacuumed the ghosts up!"
      @flags[27] = false
    end
  end

  def unlock(wi)
    open(wi) if @room == 43 && (wi == 28 || wi == 29)
    if @room == 28 && wi == 25 && !@flags[25] && @carrying[18]
      @flags[25] = true
      @routes[@room] = "SEW"
      @descriptions[@room] = "Huge open door."
      @message = "The key turns!"
    end
  end

  def leave(wi)
    if wi && @carrying[wi]
      @carrying[wi] = nil
      @locations[wi] = @room
      @message = "Done."
    end
  end

  def score
    score = @carrying.reduce(0) { |sum, value| value.nil? ? sum : sum += 1 }
    if score == 17
      if !@carrying[15] && @room != 57
        puts "You have everything.\nReturn to the gate to get the final score"
      elsif @room == 57
        puts "Double score for reaching here!"
        score *= 2
      end
    end
    puts "Your score #{score}"
    if score > 18
      puts "Well done! You finished the game."
      exit(true)
    end
    press_enter_to_continue
  end

  def show_room
    puts "Your location: #{@descriptions[@room]}"
    print "Exits: "
    @routes[@room].chars.each_with_index do |c, index|
      print "#{c}"
      print "," if index < (@routes[@room].length - 1)
    end
   # OS.flush
    puts
  end

  def show_objects
    @locations.each_with_index do |l, i|
      puts "You can see #{@objects[i]}" if @locations[i] == @room && !@flags[i]
    end
  end

  def press_enter_to_continue
    puts "\nPress enter to continue"
    gets
  end

  def welcome
    while true
      #clear
      puts "Haunted House"
      puts "-------------"
      show_location
    end
  end
end

a = gets

HauntedHouse.new.welcome

