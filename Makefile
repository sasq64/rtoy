CMAKE_FLAGS = -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

ifeq (, $(shell which cmake))
  $(error "`cmake` required; 'brew install cmake' or 'apt install cmake'")
endif

toy : mruby debug flite
	cmake --build builds/debug -- -j8 toy

emtoy : emruby embuild emflite
	cmake --build builds/em -- -j8

test : debug
	cmake --build builds/debug -- -j8 hex_test

builds/em/cmake_install.cmake :
	rm -rf builds/em
	cmake -Bbuilds/em -H. -DCMAKE_CXX_COMPILER=em++ -DCMAKE_RANLIB=`which emranlib` -DCMAKE_AR=`which emar` -DCMAKE_BUILD_TYPE=Release

builds/debug/cmake_install.cmake :
	rm -rf builds/debug
	cmake -Bbuilds/debug -H. ${CMAKE_FLAGS} -DCMAKE_BUILD_TYPE=Debug

compile_commands.json : builds/debug/compile_commands.json
	rm -f compile_commands.json
	ln -s builds/debug/compile_commands.json .

debug : builds/debug/cmake_install.cmake compile_commands.json

embuild : builds/em/cmake_install.cmake

mruby :
	MRUBY_CONFIG=mruby.cfg rake -f external/mruby/Rakefile -j8 -v

emruby :
	MRUBY_CONFIG=emruby.cfg rake -f external/mruby/Rakefile -j8 -v

emflite : external/flite/build/x86_64-emscripten/lib/libflite.a

external/flite/build/x86_64-emscripten/lib/libflite.a :
	#cd external/flite ; CC=emcc AR=emar RANLIB=emranlib ./configure --with-audio=none --disable-sockets
	cp flite.emconf external/flite/config/config
	#make -C external/flite clean
	make -C external/flite -j

external/flite/config.log :
	(cd external/flite ; ./configure)

flite : external/flite/config.log external/flite/build/x86_64-linux-gnu/lib/libflite.a

external/flite/build/x86_64-linux-gnu/lib/libflite.a :
	#cd external/flite ; ./configure --with-audio=none --disable-sockets
	cp flite.conf external/flite/config/config
	#make -C external/flite clean
	make -C external/flite -j
