#include <stdint.h>
#include <iostream>

#include <vector>
#include <tiffio.h>

#include <memory>
#include <typeinfo>
#include <cmath>

#include "tiff_header.h"
#include "tiff_image.h"
#include "tiff_cp.h"

#define DEBUG(x) std::cerr << #x << " = " << (x) << std::endl

/*
  https://github.com/LuaDist/libtiff/blob/43d5bd6d2da90e9bf254cd42c377e4d99008f00b/libtiff/tiffio.h#L61
  
typedef	uint32 ttag_t;		// directory tag 
typedef	uint16 tdir_t;		// directory index
typedef	uint16 tsample_t;	// sample number
typedef	uint32 tstrip_t;	// strip number
typedef uint32 ttile_t;		// tile number
typedef	int32 tsize_t;		// i/o size in bytes
typedef	void* tdata_t;		// image data ref
typedef	uint32 toff_t;		// file offset 

 */
int main(int argc, char **argv) {

  // read in the TIFF
  TIFF *itif = TIFFOpen(argv[1], "r");

  // messing around with the tiff header
  TiffHeader thead(argv[1]);
  //thead.view_stdout();
  
  // make a new image 
       //TiffImage timage(itif);

  // read the raster
       // timage.ReadToRaster(itif);
  
  // read in the TIFF as a red channel
  std::cout << "...reading red" << std::endl;
  TIFF *r_itif = TIFFOpen(argv[1], "r");
  TiffImage rimage(r_itif);

  std::cout << tiffprint(r_itif);

  rimage.ReadToRaster(r_itif);

  std::cout << " r mean " << rimage.mean(r_itif) << std::endl;
  
  // Open the output TIFF file
  TIFF* otif = TIFFOpen(argv[2], "w8");
  if (otif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", argv[2]);
    return 1;
  }

  // copy all of the tags 
  tiffcp(r_itif, otif, false);

  // copy the data
  //rimage.CopyTags(otif);
  
  // setup the RGB output file
  TiffImage rgb(r_itif);

  TIFFSetField(otif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(otif, TIFFTAG_SAMPLESPERPIXEL, 3);

  // close the red channel, we don't need it anymore
  TIFFClose(r_itif);
  
  // read in the TIFF as a green channel
  std::cout << "...reading green" << std::endl;  
  TIFF *g_itif = TIFFOpen(argv[1], "r");
  TiffImage gimage;
  /*if (!TIFFReadDirectory(g_itif)) {
    fprintf(stderr, "Unable to read second channel\n");
    }*/
  gimage = TiffImage(g_itif);
  gimage.ReadToRaster(g_itif);

  std::cout << " g mean " << gimage.mean(g_itif) << std::endl;
  
  TIFFClose(g_itif);
  
  // read in the TIFF as a blue channel
  std::cout << "...reading blue" << std::endl;
  TIFF *b_itif = TIFFOpen(argv[1], "r");
  TiffImage bimage;
  /*TIFFReadDirectory(b_itif); // advance to 2
  if (!TIFFReadDirectory(b_itif)) { // advance to 3
    fprintf(stderr, "Unable to read third channel\n");
    }*/
  bimage = TiffImage(b_itif); 
  bimage.ReadToRaster(b_itif);

  std::cerr << "...copying to the RGB raster" << std::endl;

  rgb.MergeGrayToRGB(rimage, gimage, bimage);
  
  // write it
  rgb.write(otif);
  
  // write the raster to the output image
  //rimage.write(otif);

  // close the image for writing
  TIFFClose(otif);  

  return 0;
}

