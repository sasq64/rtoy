# R-Toy
(or: _The Virtual Ruby Home Computer!_)
![](http://apone.org/toy/shot.gif)

R-Toy is what I like to call a Virtual Home Computer. It is
a text based interface to an underlying imaginary computer,
programmed in Ruby.

It is capable of text, bitmap graphics, sprites and sound.

Its ultimate goal is to replicate the magical experience of
playing with, and learning programming on a home computer
of the 80s, but in a way that is approachable to current
generations of humans.

It can be used to play/experiment with basic programming.

It can be used to prototype small games.

Mostly it can be used for fun!

I am looking for help. If you want to help out, send me a
mail: sasq64@gmail.com

## Building

_Always_ remember to fetch the git submodules first:

```
git submodule update --init
```

### Linux/OSX

#### Prerequisites
make, cmake, ruby/rake, SDL2, OpenGL, glew, freetype

#### Build & Run
```
make
bulds/debug/toy
```
### Windows

* Open up the folder in _Visual Studio_ or _Visual Studio Code_.
* Build & Run

### Web (Emscripten)

* Set up emscripten (https://emscripten.org/docs/getting_started/downloads.html)
```
make emtoy
```

Running local web server;
```
cd builds/em
python3 -m http.server
```
Goto http://localhost:8000/toy.html to test

### Raspberry PI

#### Prerequisites
make, cmake, clang-9 ruby/rake, asound, freetype

#### Build & Run
```
make release
bulds/release/toy
```
NOTE: The Makefile uses "-j8" for builds which may be too
much for your PI. Change to -j4 or -j2 as needed.