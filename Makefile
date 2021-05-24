CMAKE_FLAGS = -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

toy : mruby debug
	cmake --build builds/debug -- -j8 toy

test : debug
	cmake --build builds/debug -- -j8 hex_test

builds/debug/cmake_install.cmake :
	rm -rf builds/debug
	cmake -Bbuilds/debug -H. ${CMAKE_FLAGS} -DCMAKE_BUILD_TYPE=Debug

compile_commands.json : builds/debug/compile_commands.json
	rm -f compile_commands.json
	ln -s builds/debug/compile_commands.json .

debug : builds/debug/cmake_install.cmake compile_commands.json

mruby :
	MRUBY_CONFIG=mruby.cfg rake -f external/mruby/Rakefile -j8 -v

emruby :
	MRUBY_CONFIG=emruby.cfg rake -f external/mruby/Rakefile -j8 -v

fix :
	cp external/mruby/build/host/bin/* emhost/mrbc/bin
	chmod +x emhost/mrbc/bin/mrbc
