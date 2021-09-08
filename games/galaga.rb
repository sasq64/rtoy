ship = add_sprite(Image.from_file('data/ship.png'))
ship.move(display.width/2, display.height - 180)
ship.scale = 4
inp = Input.default
OS.vsync {
    ship.x += 8 if inp.get_key(Key::RIGHT)
    ship.x -= 8 if inp.get_key(Key::LEFT)
}
