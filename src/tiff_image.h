#ifndef TIFF_IMAGE_H
#define TIFF_IMAGE_H

#include <string>
#include <cassert>
#include <vector>
#include <tiffio.h>

class TiffImage {

 public:

  TiffImage();

 private:
  
  uint32_t m_width, m_height;
  
};

#endif
