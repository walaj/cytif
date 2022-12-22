#ifndef TIFF_IFD_H
#define TIFF_IFD_H

#include <vector>
#include <iostream>
#include <tiffio.h>

class TiffIFD {

 public:

  TiffIFD();
  
 private:
  
  uint32_t m_width, m_height;
  
  size_t m_tile_width, m_tile_height;
  
  size_t m_bits_per_sample, m_sample_format, m_samples_per_pixel;

  std::vector<TiffIFD*> m_ifd_list;

};

#endif
