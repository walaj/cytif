#ifndef TIFF_IMAGE_H
#define TIFF_IMAGE_H

#include <string>
#include <cassert>
#include <vector>
#include <tiffio.h>
#include <sstream>

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
  T pixel(uint64_t x, uint64_t y) const;

  // copy the image tags from one image to another
  int CopyTags(TIFF *otif) const;

  // write the raster to the image using an already opened TIFF
  int write(TIFF* otif) const;

  // clear the raster and free the memory
  void clear_raster();

  // mean pixel value
  double mean() const;
  
 private:

  // tif to read from
  TIFF* m_tif = NULL; 

  // raster of the image
  void* m_data = NULL; 

  size_t m_current_dir = 0;
  
  // flag for whether data is stored as 1 byte or 4 bytes
  bool m_8bit = true;  

  uint32_t m_width, m_height;

  // total number of pixels in the image
  uint64_t m_pixels;

  // various TIFF flags
  uint32_t m_tilewidth, m_tileheight;
  uint16_t m_photometric, m_planar, m_bits_per_sample, m_samples_per_pixel;
  uint16_t m_extra_samples;

  // allocate the memory for the image raster
  int __alloc();

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
  
};

#endif
