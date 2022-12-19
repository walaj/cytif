#include "tiff_writer.h"

TiffWriter::TiffWriter(const char* c) {

  m_tif=TIFFOpen(c, "w");
  
  // Open the output TIFF file
  if (m_tif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", c);
    return;
  }

  m_filename = std::string(c);
  
}

void TiffWriter::SetField(ttag+t tag, ...) {

  TIFFSetField(m_tif, tag, ...);

}

void TiffWriter::~TiffWriter() {
  TIFFClose(m_tif);
}
