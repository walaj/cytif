#include "tiff_reader.h"

TiffReader::~TiffReader() {
  TIFFClose(m_tif);
}


template <typename U>
int TiffReader::__read_tiled_image() {

  // Read the image data
  U *tile = (U*)_TIFFmalloc(TIFFTileSize(itif));
  
  // Read the tiles
  int x, y;
  for (y = 0; y < height; y += tile_height) {
    for (x = 0; x < width; x += tile_width) {
      
      // Read the tile
      //std::cerr << " reading the tile at " << x << "," << y << std::endl;
      if (TIFFReadTile(itif, tile, x, y, 0, 0) < 0) {
	fprintf(stderr, "Error reading tile at (%d, %d)\n", x, y);
	return 1;
      }
      
      // Copy the tile data into the image buffer
      for (int ty = 0; ty < tile_height; ty++) {
	for (int tx = 0; tx < tile_width; tx++) {
	  if (x + tx < width && y + ty < height) {
	    if (m_data_mode_32)
	      data[x+tx][y+ty] = TIFFGetRBGA(tile[ty * tile_width + tx]);
	    else
	      data[x+tx][y+ty] = TIFFGetR(tile[ty * tile_width + tx]);
	  }
	}
      }
    }
  }
  
  _TIFFfree(tile);
}

template <typename U>
Raster<U>* TiffReader::GetImageData() const {
  return &m_data;
}


int TiffReader::ReadImage() {

  if (m_data_mode_32)
    return(__read_tiled_image<uint32_t>());
  else
    return(__read_tiled_image<uint8_t>());

}

int TiffReader::SetDataMode(TiffReaderType t) {
  if (t == MODE_32_BIT)
    m_data_mode_32 = true;
  else if (t == MODE_8_BIT)
    m_data_mode_32 = false;
  else
    return 1;

  return 0;
   
}

TiffReader::TiffReader(const char* c) {
   
   m_tif = TIFFOpen(c, "r");
   
   // Open the input TIFF file
   if (m_tif == NULL) {
    fprintf(stderr, "Error opening %s for reading\n", c);
    return;
  }

   // set the number of directories
   m_num_dirs = TIFFNumberOfDirectories(m_tif);

   m_filename = std::string(c);

   // store basic image properties
   TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &m_width);
   TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &m_height);

  // Get the bits per sample
  TIFFGetField(m_tif, TIFFTAG_BITSPERSAMPLE, &m_bits_per_sample);
  
  // Get the sample format
  TIFFGetField(m_tif, TIFFTAG_SAMPLEFORMAT, &m_sample_format);
  
  // Get the number of channels
  TIFFGetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, &m_samples_per_pixel);
  
  // Get the tile width and height
  TIFFGetField(m_tif, TIFFTAG_TILEWIDTH, &m_tile_width);
  TIFFGetField(m_tif, TIFFTAG_TILELENGTH, &m_tile_height);

  // data mode is either 8 bit or 32 bit
  bool m_data_mode_32 = true;
  
  // instantiate the raster data
  // this could me memory heavy
  Raster<uint32_t> m_data32;
  Raster<uint8_t> m_data8;
  
 }

size_t TiffReader::NumDirs() const {
  return m_num_dirs;
}
