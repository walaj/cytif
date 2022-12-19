#ifndef TIFF_READER_H
#define TIFF_READER_H

#include <string>
#include <cassert>
#include <vector>

#include <tiffio.h>

enum TiffReaderType {
  MODE_32_BIT;
  MODE_8_BIT;
};

// template class to store the 2D image data
template <typename T>
struct Raster {
  std::vector<std::vector<T>> raster;
};

class TiffReader {

 public:

  // create an empty TiffReader
 TiffReader() {
   m_tile_width = 0;
   m_tile_height = 0;
 }

  // destroy and close
  ~TiffReader();
  
  // create a new reader from a filename
  TiffReader(const char* c);
  
  // set to whether image data should be read and stored
  // as 32 bit or 8 bit
  int SetDataMode(TiffReaderType t);
  
  template <typename U>
  int __read_image() {

  
  // return the number of IFDs
  size_t NumDirs() const;

  int ReadImage();
  
 private:

  std::string m_filename;

  size_t m_tile_width, m_tile_height;

  size_t m_bits_per_sample, m_sample_format, m_samples_per_pixel;

  TIFF *m_tif;

  size_t m_num_dirs;

  uint32_t m_width, m_height;

  // matrix to store raster data
  // really should be array not vector
  std::vector<std::vector<uint8_t>> m_data; //(width, std::vector<uint8_t>(height));
  
};

#endif
