#!/bin/bash
set -e

# Clean previous build
rm -rf build
mkdir build
cd build

# Configure for native
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(sysctl -n hw.ncpu)

echo "Native build complete!"
echo "Run ./build/xboy <rom_path.gb>"
