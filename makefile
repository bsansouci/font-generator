
all: lib/bs/libTruetypebindings.o lib/bs/Fun.o

lib/bs/Fun.o: src/Fun.c
	ocamlc -ccopt -o -ccopt lib/bs/Fun.o src/Fun.c

lib/bs/libTruetypebindings.o: vendor/camlimages/src/ftintf.c
	ocamlc -ccopt -I`pwd`/vendor/freetype-2.8/release/include/freetype2 -ccopt -o -ccopt lib/bs/libTruetypebindings.o vendor/camlimages/src/ftintf.c
