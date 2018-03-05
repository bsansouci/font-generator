#include <stdio.h>
#include <string.h>
#include <math.h>

#include <png.h>

#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/bigarray.h>
#include <caml/fail.h>



void unsafe_update_float32(value arr, value index, value mul, value add) {
  float *data = Caml_ba_data_val(arr);
  data[Int_val(index)] = floor(data[Int_val(index)] * Double_val(mul) + Double_val(add));
}

CAMLprim value caml_rdtsc( )
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return Val_int( ((unsigned long long)lo)|( ((unsigned long long)hi)<<32 ));
}

CAMLprim value write_png_file_rgb( name, buffer, width, height, with_alpha )
     value name;
     value buffer;
     value width;
     value height;
     value with_alpha;
{
  CAMLparam5 ( name, buffer, width, height, with_alpha );

  FILE *fp;
  png_structp png_ptr;
  png_infop info_ptr;

  int w, h;
  int a;

  w = Int_val(width);
  h = Int_val(height);
  a = Bool_val(with_alpha);

  if (( fp = fopen(String_val(name), "wb")) == NULL ){
    failwith("png file open failed");
  }

  if ((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                        NULL, NULL, NULL)) == NULL ){
    fclose(fp);
    failwith("png_create_write_struct");
  }

  if( (info_ptr = png_create_info_struct(png_ptr)) == NULL ){
    fclose(fp);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    failwith("png_create_info_struct");
  }

  /* error handling */
  // if (setjmp(png_ptr->jmpbuf)) {
  //   /* Free all of the memory associated with the png_ptr and info_ptr */
  //   png_destroy_write_struct(&png_ptr, &info_ptr);
  //   fclose(fp);
  //   /* If we get here, we had a problem writing the file */
  //   failwith("png write error");
  // }

  /* use standard C stream */
  png_init_io(png_ptr, fp);

  /* we use system default compression */
  /* png_set_filter( png_ptr, 0, PNG_FILTER_NONE |
     PNG_FILTER_SUB | PNG_FILTER_PAETH ); */
  /* png_set_compression...() */

  png_set_IHDR( png_ptr, info_ptr, w, h,
                8 /* fixed */,
                a ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB, /* fixed */
                /*interlace*/ 0,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT );

  /* infos... */

  png_write_info(png_ptr, info_ptr);

  {
    int rowbytes, i;
    png_bytep *row_pointers;
    char *buf = Caml_ba_data_val(buffer);

    row_pointers = (png_bytep*)stat_alloc(sizeof(png_bytep) * h);

    rowbytes= png_get_rowbytes(png_ptr, info_ptr);
#if 0
    printf("rowbytes= %d width=%d\n", rowbytes, w);
#endif
    for(i=0; i< h; i++){
      row_pointers[i] = (png_bytep)(buf + rowbytes * i);
    }

    png_write_image(png_ptr, row_pointers);
    stat_free((void*)row_pointers);
  }

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);

  CAMLreturn(Val_unit);
}
