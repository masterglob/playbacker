#!/bin/sh

set -e

BUILD_DIR=build

echo "=== Configure Debug ==="

cmake -S . \
      -B ${BUILD_DIR} \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug

echo "=== Build ==="

cmake --build ${BUILD_DIR}