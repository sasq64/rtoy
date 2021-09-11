CMAKE_FLAGS = -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

ifeq (, $(shell which cmake))
  $(error "`cmake` required; 'brew install cmake' or 'apt install cmake'")
endif

toy : mruby debug 
	cmake --build builds/debug -- -j8 toy

release: mruby builds/release/cmake_install.cmake
	cmake --build builds/release -- -j8 toy

emtoy : emruby embuild
	cmake --build builds/em -- -j8

test : debug
	cmake --build builds/debug -- -j8 hex_test

builds/em/cmake_install.cmake :
	rm -rf builds/em
	cmake -Bbuilds/em -H. -DCMAKE_CXX_COMPILER=em++ -DCMAKE_C_COMPILER=emcc -DCMAKE_RANLIB=`which emranlib` -DCMAKE_AR=`which emar` -DCMAKE_BUILD_TYPE=Release

builds/debug/cmake_install.cmake :
	rm -rf builds/debug
	cmake -Bbuilds/debug -H. ${CMAKE_FLAGS} -DCMAKE_BUILD_TYPE=Debug

builds/release/cmake_install.cmake :
	rm -rf builds/release
	cmake -Bbuilds/release -H. ${CMAKE_FLAGS} -DCMAKE_BUILD_TYPE=RelWithDebInfo

compile_commands.json : builds/debug/compile_commands.json
	rm -f compile_commands.json
	ln -s builds/debug/compile_commands.json .

debug : builds/debug/cmake_install.cmake compile_commands.json


embuild : builds/em/cmake_install.cmake

mruby :
	MRUBY_CONFIG=mruby.cfg rake -f external/mruby/Rakefile -j8 -v

emruby :
	MRUBY_CONFIG=emruby.cfg rake -f external/mruby/Rakefile -j8 -v

