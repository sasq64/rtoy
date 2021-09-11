include Key
include Linux

def map(a, b, mods = 0)
    a = a.ord if a.class == String
    b = b.ord if b.class == String
    Input.default.map(a, b, mods)
end
def dmap(a, b, c)
    a = a.ord if a.class == String
    b = b.ord if b.class == String
    c = c.ord if c.class == String
    Input.default.map(a, b, 0)
    Input.default.map(a, c, 1)
end

map(KEY_ENTER, Key::ENTER)
map(KEY_SPACE, ' ')
map(KEY_LEFTSHIFT, Key::LEFT_SHIFT)
map(KEY_RIGHT, Key::RIGHT)
map(KEY_LEFT, Key::LEFT)
map(KEY_UP, Key::UP)
map(KEY_DOWN, Key::DOWN)
map(KEY_BACKSPACE, Key::BACKSPACE)
map(KEY_PAGEUP, Key::PAGE_UP)
map(KEY_PAGEDOWN, Key::PAGE_DOWN)
map(KEY_DELETE, Key::DEL)
map(KEY_ESC, Key::ESCAPE)
map(KEY_END, Key::END_)
map(KEY_HOME, Key::HOME)
dmap(KEY_LEFTBRACE, '[', '{')
dmap(KEY_RIGHTBRACE, ']', '}')
dmap(KEY_BACKSLASH, '\\', '|')
dmap(KEY_COMMA, ',', '<')
dmap(KEY_DOT, '.', '>')
dmap(KEY_SLASH, '/', '?')
dmap(KEY_MINUS, '-', '_')
dmap(KEY_EQUAL, '=', '+')
dmap(KEY_SEMICOLON, ';', ':')
dmap(KEY_APOSTROPHE, '\'', '"');
map(KEY_TAB, Key::TAB)


('a'..'z').each do |k|
    map(Linux.const_get('KEY_' + k.upcase), k)
    map(Linux.const_get('KEY_' + k.upcase), k.upcase, 1)
end

n = ')!@#$%^&*('
('0'..'9').each do |k|
    map(Linux.const_get('KEY_' + k.upcase), k)
    map(Linux.const_get('KEY_' + k.upcase), n[k.ord - '0'.ord], 1)
end

(1..12).each do |k|
    map(Linux.const_get('KEY_F' + k.to_s), Key.const_get('F' + k.to_s))
end

