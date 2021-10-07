#pragma once
// 00 - 1F : Most common keys
// 80 - 9F : Less common keys
// 0020 - FFFFFF : Unicode

// This is a mix of known physical buttons, and actual characters. Most of
// the time this makes sense.
// If you want to map 'the key to left of backspace', that will not work
// with this system.

// A simple input system only sends pressed events, but allows
// you to read keys currently held

// We have 8 standard modifiers. They can be read as keys, _but_ also
// affects other keys. You will get SHIFT, 'A' if you press shift+A,
// and not SHIFT, 'a'.

// Top bits of 32-bit word are device id. Read this if for a multiplayer
// game if you need to figure out which player is which.
// Device=0 is usually keyboard

// Keys that are commonly not found on the same decide can overlap, ie
// keyboard buttons and gamepad buttons.


#define DEVICE_ID(k) ((k>>24)&0xf)
#define IS_SHIFT(k) ((k&0x00fffff7) == RKEY_LSHIFT)


enum RKey : unsigned int
{
    RKEY_NONE = 0,
    RKEY_RIGHT = 1,
    RKEY_DOWN = 2,
    RKEY_LEFT = 3,
    RKEY_UP = 4,

    RKEY_FIRE = 5,
    RKEY_A = 5,

    RKEY_X = 6,
    RKEY_Y = 7,

    RKEY_BACKSPACE = 8,
    RKEY_B = 8,
    RKEY_TAB = 9,
    RKEY_SELECT = 9,
    RKEY_ENTER = 10,
    RKEY_START = 10,
    RKEY_END = 11,
    RKEY_R1 = 11,
    RKEY_HOME = 12,
    RKEY_L1 = 12,
    RKEY_DELETE = 13,

    RKEY_PAGEDOWN = 14,
    RKEY_R2 = 14,
    RKEY_PAGEUP = 15,
    RKEY_L2 = 15,

    RKEY_INSERT = 16,

    RKEY_ESCAPE = 0x1b,

    RKEY_SPACE = ' ',

    RKEY_F1 = 0x80,
    RKEY_F2,
    RKEY_F3,
    RKEY_F4,
    RKEY_F5,
    RKEY_F6,
    RKEY_F7,
    RKEY_F8,
    RKEY_F9,
    RKEY_F10,
    RKEY_F11,
    RKEY_F12,

    RKEY_LSHIFT = 0x90,
    RKEY_LCTRL,
    RKEY_LALT,
    RKEY_LWIN,
    RKEY_RSHIFT = 0x98,
    RKEY_RCTRL,
    RKEY_RALT,
    RKEY_RWIN,

};

