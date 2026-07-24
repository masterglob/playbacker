# Projet playbacker - wrapper CMake

BUILD_DIR := build

.PHONY: all configure build clean rebuild debug release run

all: build


configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug


build: configure
	cmake --build $(BUILD_DIR)


debug: configure
	cmake --build $(BUILD_DIR) --verbose


release:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR)


clean:
	cmake --build $(BUILD_DIR) --target clean


rebuild: clean build


run:
	./$(BUILD_DIR)/playbacker