#ifndef TIFF_HEADER_H
#define TIFF_HEADER_H

#include <iostream>
#include <cassert>
#include <string>
#include <memory>
#include <fstream>
#include <cstring>

#define HEADER_BUFF 1024

 // show whether the image file is big or little endian
static const char _little[2] = {'I','I'};
static const char _big[2] = {'M','M'};

class TiffHeader {

 public:

  TiffHeader(const char* c);
  
  TiffHeader(const std::string& fn);
  
  char* data() { return m_data.get(); }

  // send to human readable stdout
  void view_stdout() const;
    
 private:
  
  std::string m_filename;
  
  // smart pointer to the 
  std::unique_ptr<char[]> m_data;

  // the tiff id (e.g. 42 for 32-bit, 43 for 64-bit)
  uint16_t m_tid = 0;

  // offset parameters, namely where does the location of the first offset value
  // start and how long is the offset value (e.g. 32 or 64 bits)
  size_t m_offset_start, m_offset_len;

  // location of the first IFD;
  uint64_t m_first_offset;

  // call internal routines to make the header
  int __construct_header();
  
  // get the location of the first IFD
  int __get_offsets();

  // check that the endianness is specified
  int __endianess_error_check() const;

  // check that the 3rd word of a BigTIFF is 0
  int __big_tiff_error_check() const;
  
};

#endif
