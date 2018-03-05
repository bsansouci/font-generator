if (Array.length(Sys.argv) !== 3 && Array.length(Sys.argv) !== 4) {
  failwith("Usage: gen-font filename font-size [output-filename]");
};

let fontPath = Sys.argv[1];

let fontSize = float_of_string(Sys.argv[2]);

let baseoutputname =
  Array.length(Sys.argv) === 4 ? Sys.argv[3] : Filename.basename(fontPath);

let output =
  switch (Filename.chop_extension(baseoutputname)) {
  | exception (Invalid_argument(_)) => baseoutputname
  | name => name
  };

let debug = true;

type refbox('a, 'b) = {
  cont: 'a,
  ref: ref('b)
};

type t = refbox(Ftlow.library, unit);

let intfrac_of_float = (dotbits, f) => {
  let d = float(1 lsl dotbits);
  truncate(f *. d);
};

let intfrac6_of_float = intfrac_of_float(6);

let set_char_size = (face, char_w, char_h, res_h, res_v) =>
  Ftlow.set_char_size(
    face.cont,
    intfrac6_of_float(char_w),
    intfrac6_of_float(char_h),
    res_h,
    res_v
  );

let set_charmap = (face, charmap) => Ftlow.set_charmap(face.cont, charmap);

/* no idea what this is */
let dpi = 72.;

let allCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+1234567890-={}|:\"<>?[]\\;',./ ";

let unicode_of_latin = s =>
  Array.init(String.length(s)) @@ (i => Char.code(s.[i]));

let allCharactersEncoded = unicode_of_latin(allCharacters);

module IntMap =
  Map.Make(
    {
      type t = int;
      let compare = compare;
    }
  );

module IntPairMap =
  Map.Make(
    {
      type t = (int, int);
      let compare = ((a1, a2), (b1, b2)) => {
        let first = compare(a1, b1);
        if (first != 0) {
          first;
        } else {
          compare(a2, b2);
        };
      };
    }
  );

type glyphInfoT = {
  width: float,
  height: float,
  atlasX: float,
  atlasY: float,
  bearingX: float,
  bearingY: float,
  advance: float
};

external write_rgba :
  (
    string,
    Bigarray.Array1.t(int, Bigarray.int8_unsigned_elt, Bigarray.c_layout),
    int,
    int,
    bool
  ) =>
  unit =
  "write_png_file_rgb";

let generateFontForRes = res => {
  let output = output ++ "_" ++ string_of_int(res) ++ "x";
  if (debug) {
    print_endline(
      "Creating "
      ++ output
      ++ " font for "
      ++ fontPath
      ++ " with size "
      ++ string_of_float(fontSize)
    );
  };
  let csize = 2 * res * int_of_float(dpi);
  let library = {cont: Ftlow.init(), ref: ref()};
  let done_face = face => Ftlow.done_face(face.cont);
  Gc.finalise(v => Ftlow.close(v.cont), library);
  let new_face = (font, idx) => {
    let face = {
      cont: Ftlow.new_face(library.cont, font, idx),
      ref: ref(library)
    };
    let info = Ftlow.face_info(face.cont);
    Gc.finalise(done_face, face);
    (face, info);
  };
  let (face, _info) = new_face(fontPath, 0);
  set_char_size(face, fontSize, fontSize, csize, csize);
  set_charmap(face, Ftlow.{platform_id: 3, encoding_id: 1});
  let texLength = 4096;
  let bigarrayTextData =
    Bigarray.Array1.create(
      Bigarray.Int8_unsigned,
      Bigarray.C_layout,
      texLength * texLength * 4
    );
  /* @Incomplete @Hack fill bigarray with 0s. We need to add Bigarray.fill to reasongl-interface
     to get this to work. */
  /*for i in 0 to (texLength * texLength - 1) {
      Bigarray.Array1.set bigarrayTextData (i * 4 + 0) 255;
      Bigarray.Array1.set bigarrayTextData (i * 4 + 1) 255;
      Bigarray.Array1.set bigarrayTextData (i * 4 + 2) 255;
      Bigarray.Array1.set bigarrayTextData (i * 4 + 3) 255
    };*/
  Bigarray.Array1.unsafe_set(bigarrayTextData, 0, 255);
  Bigarray.Array1.unsafe_set(bigarrayTextData, 1, 255);
  Bigarray.Array1.unsafe_set(bigarrayTextData, 2, 255);
  Bigarray.Array1.unsafe_set(bigarrayTextData, 3, 255);
  Bigarray.Array1.unsafe_set(bigarrayTextData, 4, 255);
  Bigarray.Array1.unsafe_set(bigarrayTextData, 5, 255);
  Bigarray.Array1.unsafe_set(bigarrayTextData, 6, 255);
  Bigarray.Array1.unsafe_set(bigarrayTextData, 7, 255);
  let prevX = ref(4);
  let prevY = ref(0);
  let nextY = ref(0);
  let chars = ref(IntMap.empty);
  let kerningMap = ref(IntPairMap.empty);
  let maxWidth = ref(0);
  let {Ftlow.has_kerning} = Ftlow.face_info(face.cont);
  let {Ftlow.height: lineHeight} = Ftlow.get_font_metrics(face.cont);
  let lineHeight = float_of_int(lineHeight) /. 64. -. 1.;
  let hackIncrementJustBecause = float_of_int(res * 2 + 1);
  Array.iter(
    c => {
      ignore @@ Ftlow.render_char_raw(face.cont, c, 0, Ftlow.Render_Normal);
      let glyphMetrics = Ftlow.get_glyph_metrics(face.cont);
      maxWidth := max(maxWidth^, glyphMetrics.gm_width / 64);
      let bitmapInfo = Ftlow.get_bitmap_info(face.cont);
      if (prevX^ + bitmapInfo.bitmap_width >= texLength) {
        prevX := 4;
        prevY := nextY^;
      };
      if (has_kerning) {
        let code = Ftlow.get_char_index(face.cont, c);
        Array.iter(
          c2 => {
            let code2 = Ftlow.get_char_index(face.cont, c2);
            let (x, y) = Ftlow.get_kerning(face.cont, code, code2);
            let (x, y) = (float_of_int(x) /. 64., float_of_int(y) /. 64.);
            if (abs_float(x) > 0.00001 || abs_float(y) > 0.00001) {
              kerningMap := IntPairMap.add((c, c2), (x, y), kerningMap^);
            };
          },
          allCharactersEncoded
        );
      };
      chars :=
        IntMap.add(
          c,
          {
            atlasX: float_of_int(prevX^),
            atlasY: float_of_int(prevY^),
            height: float_of_int(bitmapInfo.bitmap_height),
            width: float_of_int(bitmapInfo.bitmap_width),
            bearingX: float_of_int(glyphMetrics.gm_hori.bearingx) /. 64.,
            bearingY:
              float_of_int(glyphMetrics.gm_hori.bearingy)
              /. 64.
              +. hackIncrementJustBecause,
            advance: float_of_int(glyphMetrics.gm_hori.advance) /. 64.
          },
          chars^
        );
      for (y in 0 to bitmapInfo.bitmap_height - 1) {
        for (x in 0 to bitmapInfo.bitmap_width - 1) {
          let level = Ftlow.read_bitmap(face.cont, x, y);
          let y = bitmapInfo.bitmap_height - y;
          let baIndex = (y + prevY^) * texLength + (x + prevX^);
          Bigarray.Array1.unsafe_set(bigarrayTextData, 4 * baIndex, 255);
          Bigarray.Array1.unsafe_set(bigarrayTextData, 4 * baIndex + 1, 255);
          Bigarray.Array1.unsafe_set(bigarrayTextData, 4 * baIndex + 2, 255);
          Bigarray.Array1.unsafe_set(bigarrayTextData, 4 * baIndex + 3, level);
        };
      };
      prevX := prevX^ + bitmapInfo.bitmap_width + 10;
      nextY := max(nextY^, prevY^ + bitmapInfo.bitmap_height) + 10;
    },
    allCharactersEncoded
  );
  {
    let oc = open_out(output ++ ".fnt");
    output_string(
      oc,
      Printf.sprintf(
        "info res=%d face=%s size=%g bold=0 italic=0 charset= unicode= stretchH=100 smooth=1 aa=1 padding=3,3,3,3 spacing=0,0 outline=0\n",
        res,
        Filename.basename(output),
        fontSize
      )
    );
    output_string(
      oc,
      Printf.sprintf(
        "common lineHeight=%g base=25 scaleW=256 scaleH=256 pages=1 packed=0\n",
        lineHeight
      )
    );
    output_string(
      oc,
      Printf.sprintf(
        "page id=0 file=\"%s\"\n",
        Filename.basename(output) ++ ".png"
      )
    );
    output_string(
      oc,
      Printf.sprintf("chars count=%d\n", IntMap.cardinal(chars^))
    );
    IntMap.iter(
      (key, v) =>
        output_string(
          oc,
          Printf.sprintf(
            "char id=%d x=%g y=%g width=%g height=%g xoffset=%g yoffset=%g xadvance=%g page=0 chnl=15\n",
            key,
            v.atlasX,
            v.atlasY +. 1.,
            v.width,
            v.height,
            v.bearingX,
            -. v.bearingY,
            v.advance
          )
        ),
      chars^
    );
    output_string(
      oc,
      Printf.sprintf("kernings count=%d\n", IntPairMap.cardinal(kerningMap^))
    );
    IntPairMap.iter(
      ((first, second), (x, _y)) =>
        output_string(
          oc,
          Printf.sprintf(
            "kerning first=%d second=%d amount=%g\n",
            first,
            second,
            x
          )
        ),
      kerningMap^
    );
    /*

     */
    close_out(oc);
  };
  write_rgba(output ++ ".png", bigarrayTextData, texLength, texLength, true);
};

generateFontForRes(1);

generateFontForRes(2);

print_endline("Successfully created all fonts.");
