#ifndef TIFF_IFD_H
#define TIFF_IFD_H

class TiffIFD {

 public:

  TiffIFD();
  
 private:
  
  uint32_t m_width, m_height;
  
  size_t m_tile_width, m_tile_height;
  
  size_t m_bits_per_sample, m_sample_format, m_samples_per_pixel;
  
};

#endif
