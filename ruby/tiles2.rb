
clear

img = Image.from_file('data/tiles.png')

w,h = img.width / 16, img.height / 16
puts "#{w}x#{h}"
tiles = img.split(w, h)
t = '0'.ord
zero = 48*17+35
puts tiles.length
ts = tiles[zero..zero+9]
ts.each do |tile|
    display.console.add_tile(t, tile)
    t += 1
end
ts = tiles[zero+48..zero+48+13]
t = 'A'.ord
ts.each do |tile|
    display.console.add_tile(t, tile)
    t += 1
end


display.console.tile_size(16,16)
