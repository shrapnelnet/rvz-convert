all: src/main.cpp src/options.cpp submodules preinstall
	cc -Wall -O -std=c++20 src/main.cpp src/options.cpp -o bin/rvz-convert -lstdc++ -pthread

submodules:
	mkdir -p bin
	@echo === Clone submodules ===
	git submodule update --init --recursive --depth=1
	@echo === CMake: Build dolphin at lib/dolphin/cmake-build ===
	cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -S lib/dolphin -B lib/dolphin/cmake-build
	@echo === Build dolphin-tool ===
	cmake --build lib/dolphin/cmake-build --target dolphin-tool -j${nproc}
	make -C lib/wiimms-iso-tools/project -j${nproc}

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
