#ifndef TIFF_READER_H
#define TIFF_READER_H

#include <string>
#include <cassert>
#include <vector>
#include <tiffio.h>

#include "tiff_header.h"

class TiffReader {

 public:

  // create an empty TiffReader
  TiffReader() {}

  // destroy and close
  ~TiffReader();
  
  // create a new reader from a filename
  TiffReader(const char* c);
  
  // set to whether image data should be read and stored
  // as 32 bit or 8 bit
  //int SetDataMode(TiffReaderType t);
  
  //template <typename U>
  //int __read_image() {
  
  // return the number of IFDs
  size_t NumDirs() const;
  
  int ReadImage();
  
 private:

  std::string m_filename;

  size_t m_num_dirs;

  TiffHeader m_tif_header;

  //template <typename U>
  //int __read_tiled_image();

};

#endif
