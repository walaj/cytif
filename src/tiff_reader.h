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
  
  template <typename U>
  Raster<U>* GetImageData() const;
  
 private:

  std::string m_filename;

  size_t m_num_dirs;

  // matrix to store raster data
  // really should be array not vector
  
  Raster m_data; 

  template <typename U>
  int __read_tiled_image();

};

#endif
