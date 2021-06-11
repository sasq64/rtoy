# R-Toy
(or: _The Virtual Ruby Home Computer!_)
![](http://apone.org/toy/shot.gif)
A ruby play & development environment inspired by the home computers of the 80s.


## Prerequisites

make, cmake, ruby/rake, SDL2, OpenGL, glew, freetype

```
git submodule update --init
make
```
```
bulds/debug/toy
```

### Emscripten

* Set up emsripten (https://emscripten.org/docs/getting_started/downloads.html)
```
make emtoy
```

Running local web server;
```
cd builds/em
python3 -m http.server
```
Goto http://localhost:8000/toy.html to test
