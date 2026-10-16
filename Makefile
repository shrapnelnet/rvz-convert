all: src/main.cpp src/options.cpp submodules preinstall
	cc -Wall -O -std=c++20 src/main.cpp src/options.cpp -o bin/rvz-convert -lstdc++ -pthread

submodules:
	mkdir -p bin
	git submodule update --init --recursive --depth=1
	cmake -S lib/dolphin -B lib/dolphin/cmake-build
	make -C lib/dolphin/cmake-build dolphin-tool
	make -C lib/wiimms-iso-tools/project

preinstall: submodules
	cp lib/wiimms-iso-tools/project/bin/wit bin/
	cp lib/dolphin/cmake-build/Binaries/dolphin-tool bin/

clean:
	rm -rf lib/dolphin/cmake-build/*
	rm -rf lib/dolphin/cmake-build-windows/*
	rm bin/*

install:
	cp bin/* /usr/local/bin

uninstall:
	rm -i /usr/local/bin/wit /usr/local/bin/rvz-convert /usr/local/bin/dolphin-tool
