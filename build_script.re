open Bsb_internals;

let (+/) = Filename.concat;

gcc("lib" +/ "bs" +/ "Fun.o", ["src" +/ "Fun.c"]);

gcc(
  ~includes=[
    "vendor" +/ "freetype-2.8" +/ "release" +/ "include" +/ "freetype2",
  ],
  "lib" +/ "bs" +/ "libTruetypebindings.o",
  ["vendor" +/ "camlimages" +/ "src" +/ "ftintf.c"],
);
