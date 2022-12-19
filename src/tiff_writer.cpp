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

void TiffWriter::SetTile(int h, int w) {
  m_tile_width = w;
  m_tile_height = h;

  TIFFSetField(m_tif, TIFFTAG_TILEWIDTH, m_tile_width);
  TIFFSetField(m_tif, TIFFTAG_TILELENGTH, m_tile_height);
  
}

template <typename U>
int TiffWriter::WriteTiledImage(const Raster<U>& data) {

  // Allocate a buffer for the output tiles
  uint8_t *outtilebuf = (uint8_t*)_TIFFmalloc(TIFFTileSize(otif));
  if (outtilebuf == NULL) {
    fprintf(stderr, "Error allocating memory for tiles\n");
    return 1;
  }
  
  // Write the tiles to the TIFF file
  int num_height_tiles = (int)std::ceil((double)m_height / m_tile_height);
  int num_width_tiles  = (int)std::ceil((double)m_width /  m_tile_width);

  if (m_verbose) {
    std::cerr << " h " << m_width << " num tiles H " << num_height_tiles <<
      " w " << m_width << " num tiles W " << num_width_tiles << std::endl;
    std::cerr << " writing " << m_filename << std::endl;
  }
  
  for (y = 0; y < num_height_tiles; y++) {
    for (x = 0; x < num_width_tiles; x++) {
      
      // Fill the tile buffer with data
      for (int ty = 0; ty < m_tile_height; ty++) {
	for (int tx = 0; tx < m_tile_width; tx++) {
	  //  std::cerr << "[" << y * m_tile_width + tx << "," <<
	  //    y * m_tile_height + ty << "] = " << static_cast<unsigned int>(data[x * m_tile_width+ tx][y * m_tile_height + ty]) << endl;
	  //    std::cerr << (x * num_width_tiles + tx) << "," << y * num_height_tiles + ty << std::endl;
	  if ( (x * m_tile_width + tx) < width && (y * m_tile_height + ty) < height) {
	    outtilebuf[ty * m_tile_width + tx] = data[x * m_tile_width + tx][y * m_tile_height + ty];
	  }
	}
      }

      //std::cerr << " WRITING TILE " << x << " , " << y << " random " << static_cast<unsigned int>(outtilebuf[100]) << std::endl;
      // Write the tile to the TIFF file
      
      if (TIFFWriteTile(otif, outtilebuf, x * m_tile_width, y * m_tile_height, 0, 0) < 0) {
	fprintf(stderr, "Error writing tile at (%d, %d)\n", x, y);
	_TIFFfree(outtilebuf);
	return 1;
      }
      
      
    }
  }

  // Clean up
  _TIFFfree(outtilebuf);

  
}

void TiffWriter::SetField(ttag+t tag, ...) {

  TIFFSetField(m_tif, tag, ...);

}

void TiffWriter::~TiffWriter() {
  TIFFClose(m_tif);
}
