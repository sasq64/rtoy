p Input
p Display
include Key
include Linux

def map(a, b, mods = nil)
    a = a.ord if a.class == String
    b = b.ord if b.class == String
    Input.default.map(a, b)
end

p Linux.const_get('KEY_A')

map(KEY_MINUS, '-');
('a'..'z').each do |k|
    map(Linux.const_get('KEY_' + k.upcase), k)
    map(Linux.const_get('KEY_' + k.upcase), k.upcase, :shift)
end

