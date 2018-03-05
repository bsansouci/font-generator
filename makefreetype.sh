cd vendor/freetype-2.8
./configure --prefix=`pwd`/release --with-png=no --with-bzip2=no --with-zlib=no
make
make install
