#ifndef TIFF_WRITER_H
#define TIFF_WRITER_H

#include <string>
#include "tiffio.h"

class TiffWriter {

 public:

  // create an empty TiffWriter
 TiffWriter() {
   m_tile_width = 0;
   m_tile_height = 0;
 }

  // destroy
  ~TiffWriter();

 //
 void SetTag(ttag_t tag, ...);

 TIFF* GetTiff() const { return m_tif; }
 
 private:

  std::string m_filename;

  size_t m_tile_width, m_tile_height;

  TIFF *m_tif;

};

#endif
