

img = Image.from_file('data/8x8tiles.png')

w,h = img.width / 8, img.height / 8
puts "#{w}x#{h}"
tiles = img.split(img.width / 8, img.height / 8)

tiles.each_with_index do |tile, i|
    display.console.set_tile_image('A'.ord + i, tile)
end

