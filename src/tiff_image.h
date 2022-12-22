#ifndef TIFF_IMAGE_H
#define TIFF_IMAGE_H

#include <string>
#include <cassert>
#include <vector>
#include <tiffio.h>
#include <sstream>
#include "tiff_cp.h"

#define PIXEL_GRAY 999
#define PIXEL_RED 0
#define PIXEL_GREEN 1
#define PIXEL_BLUE 2
#define PIXEL_ALPHA 3


#include <random>

int getRandomInt() {
    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    // Set the range of the generator to [0, 255]
    std::uniform_int_distribution<> dis(0, 255);

    // Generate and return a random integer
    return dis(gen);
}

// copy tags from one TIFF to another
static int TiffTagCopy(TIFF* o, TIFF* d);

class TiffImage {

 public:

  // create a new image from a TIFF (libtiff)
  TiffImage(TIFF* tif);

  // create an empty iamge
  TiffImage() {}

  ~TiffImage() {
    clear_raster();
  }
  
  // human readable print
  std::string print() const;

  // access the data pointer
  void* data() const { return m_data; }

  // read the image tif to a 2D raster
  int ReadToRaster(TIFF *tif);

  // access a pixel as type T (uint8_t or uint32_t)
  template <typename T>  
  T pixel(uint64_t x, uint64_t y, int p) const;

  // access a pixel as type T (uint8_t or uint32_t) from the flattened index
  template <typename T>  
  T element(uint64_t e) const;

  // copy the image tags from one image to another
  int CopyTags(TIFF *otif) const;

  // write the raster to the image using an already opened TIFF
  int write(TIFF* otif) const;

  // clear the raster and free the memory
  void clear_raster();

  // mean pixel value
  double mean(TIFF* tiff) const;

  // take three 8 bit gray scale images and convert to RGB
  int MergeGrayToRGB(const TiffImage &r, const TiffImage &g, const TiffImage &b);

  /// return the total number of pixels
  uint64_t numPixels() const { return m_pixels; }
  
 private:

  // tif to read from
  TIFF* m_tif = NULL; 

  // raster of the image
  void* m_data = NULL; 

  size_t m_current_dir = 0;
  
  // flag for whether data is stored as 1 byte or 4 bytes
  uint32_t m_width = 0, m_height = 0;

  // total number of pixels in the image
  uint64_t m_pixels = 0;

  // various TIFF flags
  uint32_t m_tilewidth, m_tileheight;
  uint16_t m_photometric, m_planar, m_bits_per_sample, m_samples_per_pixel;
  uint16_t m_extra_samples;

  // allocate the memory for the image raster
  int __alloc(TIFF *tif);

  // sub method to ReadToRaster for tiled image
  int __tiled_read(TIFF *tif);
  
  // pass a new TIFF file
  int __give_tiff(TIFF *tif);

  // write the raster to a tiled image
  int __tiled_write(TIFF* otif) const;

  // has the image had its raster stored to memory
  bool __is_rasterized() const;

  // has the image been assciated with a TIFF
  bool __is_initialized() const;

  size_t __get_mode(TIFF* tif) const;
  
};

#endif
