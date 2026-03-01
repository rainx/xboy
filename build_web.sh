#!/bin/bash
set -e

# Set up Emscripten environment
if [ -z "$EMSDK" ]; then
	echo "Error: EMSDK environment variable not set"
	echo "Please run 'source /path/to/emsdk/emsdk_env.sh' first"
	exit 1
fi

# Clean previous build
rm -rf build_web
mkdir build_web
cd build_web

# Configure for web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
emmake make -j$(sysctl -n hw.ncpu)

echo "WebAssembly build complete!"
echo "Open build_web/xboy.html in your browser"
