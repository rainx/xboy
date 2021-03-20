all:
	kaitai-struct-compiler -t cpp_stl --cpp-standard 11 --cpp-namespace kaitai-struct-gen cartridge-header.ksy && \
	mv cartridge_header.cpp ../src/kaitai-struct-gen/cartridge-header.cpp && \
	mv cartridge_header.h ../src/kaitai-struct-gen/cartridge-header.h && \
	sed -i '' -e  's/cartridge_header\.h/cartridge-header.h/g' ../src/kaitai-struct-gen/cartridge-header.cpp