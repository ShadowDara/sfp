# Makefile for Fling

dbuild:
	echo Building Debug
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build

drun:
	$(MAKE) dbuild
	./build/fling-lang

build:
	echo Building Release
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build

run:
	$(MAKE) build
	./build/fling-lang

.PHONY: build
