# R-Toy
(or: _The Virtual Ruby Home Computer!_)
![](http://apone.org/toy/shot.png)
A ruby play & development environment inspired by the home computers of the 80s.


## Prerequisites

make, cmake, rake, SDL2, OpenGL, glew, freetype

```
git submodule update --init
make
```
```
bulds/debug/toy
```

### Emscripten

* Set up emsripten (https://emscripten.org/docs/getting_started/downloads.html)

MRuby currently doesn't include the compiler into cross builds so the
following steps are a workaround;

```
make mruby
make fix
make mruby
```

```
mkdir builds/em
cd builds/em
cmake ../.. -DCMAKE_CXX_COMPILER=em++ -DCMAKE_BUILD_TYPE=Release
make -j
```

```
python3 -m http.server
```
Goto http://localhost:8000/toy.html to test