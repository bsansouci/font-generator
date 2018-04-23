# Font Generator

Simple font generator that takes in any TTF or OTF font and generates `myfont.fnt` and `myfont.png` to be consumed by [Reprocessing](https://github.com/schmavery/reprocessing).

```
Usage: gen-font filename font-size [output-filename]
```

# Implicit dependency
This depends on `libpng` which you can get by running `brew install libpng` on OSX or `apt-get install libpng-dev` on ubuntu.
