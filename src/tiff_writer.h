#ifndef TIFF_WRITER_H
#define TIFF_WRITER_H

#include <string>
#include <memory>
#include "tiffio.h"
#include "tiff_reader.h"
#include "tiff_image.h"

class TiffWriter {

 public:

  // create an empty TiffWriter
  TiffWriter() {}
  
  TiffWriter(const TiffReader& tr);

  TiffWriter(const char* c);
    
  // destroy
  ~TiffWriter() {}

  bool isTiled();
  
  //
  void SetTag(ttag_t tag, ...);
  
  void SetTile(int h, int w);

  int Write(const TiffImage& ti);

  void MatchTagsToRaster(const TiffImage& ti);
  
  void CopyFromReader(const TiffReader& tr);

  uint8_t GetMode() const;
  
 private:

  std::string m_filename;

  std::shared_ptr<TIFF> m_tif;

  bool m_verbose = true;

  int __tiled_write(const TiffImage& ti) const;
  int __lined_write(const TiffImage& ti) const;  

};

#endif
