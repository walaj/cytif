#include "tiff_image.h"
#include <cassert>
#include <iostream>

TiffImage::TiffImage(TIFF* tif) {

  // clear the raster data
  // note that TiffImage class is NOT responsible for managing TIFF memory
  clear_raster();

  // add the tiff
  __give_tiff(tif);
  
}

int TiffImage::__give_tiff(TIFF *tif) {

  // this object now stores a pointer to the input tif
  m_tif = tif;
  
  // store the offset of the first tif
  m_current_dir = TIFFCurrentDirOffset(tif);

  if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &m_width)) {
    fprintf(stderr, "ERROR: image does not have a width tag specified\n");
    m_width = 0;
    return 1;
  }
  if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &m_height)) {
    fprintf(stderr, "ERROR: image does not have a length tag specified\n");
    m_height = 0;
    return 1;
  }
  if (!TIFFGetField(tif, TIFFTAG_TILEWIDTH, &m_tilewidth)) {
    m_tilewidth = 0;
    assert(!TIFFIsTiled(tif));
  }
  if (!TIFFGetField(tif, TIFFTAG_TILELENGTH, &m_tileheight)) {
    m_tileheight = 0;
    assert(!TIFFIsTiled(tif));
  }
  if (!TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &m_samples_per_pixel)) {
    fprintf(stderr, "WARNING: image does not have a SAMPLESPERPIXEL tag. Assuming 1\n");
    m_samples_per_pixel = 1;
  }

  if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &m_bits_per_sample)) {
    fprintf(stderr, "WARNING: image does not have a BITSPERSAMPLE tag. Assuming 8\n");
    m_bits_per_sample = 8;
  }
  
  if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &m_photometric)) {
    fprintf(stderr, "WARNING: image does not have a PHOTOMETRIC tag. Assuming MINISBLACK\n");
    m_photometric = PHOTOMETRIC_MINISBLACK;
  }

  // note that optinos for planar config are:
  // PLANARCONFIG_CONTIG 1 -- RGBA RGBA RGBA etc (interleaved)
  // PLANARCONFIG_SEPARATE 2 -- RRRRRR GGGGGG BBBBB etc (separate planes)
  if (!TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &m_planar)) {
    fprintf(stderr, "WARNING: image does not have a PLANARCONFIG tag. Assuming PLANARCONFIG_CONTIG\n");
    m_planar = PLANARCONFIG_CONTIG;
  }

  if (!TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &m_extra_samples))
    m_extra_samples = 0;
  
  /*
   * Check the image to see if TIFFReadRGBAImage can deal with it.
   * 1/0 is returned according to whether or not the image can
   * be handled.  If 0 is returned, emsg contains the reason
   * why it is being rejected.
   */
  char emsg[1024];
  int rgba_ok = TIFFRGBAImageOK(tif, emsg);
  if (!rgba_ok) {
    fprintf(stderr, emsg);
    return 1;
  }

  // set the number of pixels
  m_pixels = static_cast<uint64_t>(m_width) * m_height;

  // set the bits
  if (m_bits_per_sample == 8 && m_samples_per_pixel == 1)
    m_8bit = true;
  else
    m_8bit = false;

  return 0;
}

std::string TiffImage::print() const {

  if (!__is_initialized())
    return("no associated TIFF file");
  
  std::stringstream ss;

  ss << "TIFF name: " << TIFFFileName(m_tif) << std::endl;

    // *** uint32		tif_flags;
  // sets a number of options for this TIFF
  // see https://github.com/LuaDist/libtiff/blob/43d5bd6d2da90e9bf254cd42c377e4d99008f00b/libtiff/tiffiop.h#L100
  ss << "------ Image flags ------" << std::endl;  
  ss << "  TIFF Tiled: " << TIFFIsTiled(m_tif) << std::endl;
  ss << "  TIFF Byte swapped: " << TIFFIsByteSwapped(m_tif) << std::endl;
  ss << "  TIFF Upsampled: " << TIFFIsUpSampled(m_tif) << std::endl;
  ss << "  TIFF BigEndian: " << TIFFIsBigEndian(m_tif) << std::endl;
  ss << "  TIFF MSB-2-LSB: " << TIFFIsMSB2LSB(m_tif) << std::endl;
   
  ss << "------ Image tags ------" << std::endl;
  ss << "  Image size: " <<  m_width << "x" << m_height << std::endl;
  ss << "  Extra samples: " << m_extra_samples << std::endl;
  ss << "  Samples per pixel: " << m_samples_per_pixel << std::endl;
  ss << "  Bits per sample: " << m_bits_per_sample << std::endl;
  ss << "  Planar config: " << m_planar << std::endl;    
  ss << "------ Internals -------" << std::endl;
  ss << "  Current offset: 0x" << std::hex << TIFFCurrentDirOffset(m_tif) << std::endl;
  // *** uint32		tif_row;	/* current scanline */
  ss << "  Current row: " << std::dec << TIFFCurrentRow(m_tif) << std::endl;
  // *** tdir_t tif_curdir;	/* current directory (index) */
  ss << "  Current dir: " << TIFFCurrentDirectory(m_tif) << std::endl;

  // *** tstrip_t	tif_curstrip;	/* current strip for read/write */
  if (!m_tilewidth)
    ss << "  Current strip: " << TIFFCurrentStrip(m_tif) << std::endl;

  ss << "  BigTiff? " << (TIFFIsBigTIFF(m_tif) ? "True" : "False") << std::endl;

  // *** int tif_mode;	/* open mode (O_*) */
  // https://github.com/LuaDist/libtiff/blob/43d5bd6d2da90e9bf254cd42c377e4d99008f00b/contrib/acorn/convert#L145
  // O_RDONLY 0
  // O_WRONLY 1
  // O_RDWR 2
  // O_APPEND 8
  // O_CREAT		0x200
  // O_TRUNC		0x40
  ss << "  TIFF read/write mode: " << TIFFGetMode(m_tif) << std::endl;

  // *** int tif_fd -- open file descriptor
  ss << "  TIFF I/O descriptor: " << TIFFFileno(m_tif) << std::endl;

  
  if (m_tilewidth) {
    ss << "------ Tile Info -----" << std::endl;    
    ///
    /// tile support
    ///
    ss << "  Tiles: " << m_tilewidth << "x" << m_tileheight << std::endl;    
    ss << "  Current tile: " << TIFFCurrentTile(m_tif) << std::endl;
    
    // return number of bytes of row-aligned tile
    ss << "  Tile size (bytes): " << TIFFTileSize(m_tif) << std::endl;
    
    // return the number of bytes in each row of a tile
    ss << "  Tile row size (bytes): " << TIFFTileRowSize(m_tif) << std::endl;
    
    // compute the number of tiles in an image
    ss << "  Tile count: " << TIFFNumberOfTiles(m_tif) << std::endl;
    
  }
  
  // *** toff_t	 tif_nextdiroff;	/* file offset of following directory */
  
  // *** toff_t*  tif_dirlist;	/* list of offsets to already seen */
  
  // *** uint16	 tif_dirnumber;  /* number of already seen directories */

  // *** TIFFDirectory	tif_dir;	/* internal rep of current directory */

  // *** TIFFHeader	tif_header;	/* file's header block */
  /* https://github.com/LuaDist/libtiff/blob/43d5bd6d2da90e9bf254cd42c377e4d99008f00b/libtiff/tiff.h#L95
  // TIFF header
  typedef	struct {
  uint16	tiff_magic;	// magic number (defines byte order) 
  #define TIFF_MAGIC_SIZE		2
  uint16	tiff_version;	// TIFF version number 
  #define TIFF_VERSION_SIZE	2
  uint32	tiff_diroff;	// byte offset to first directory 
  #define TIFF_DIROFFSET_SIZE	4
  } TIFFHeader;
  */

  return ss.str();
  
  
}

template <typename T>
T TiffImage::pixel(uint64_t x, uint64_t y) const {

  // right now only two types allowed
  static_assert(std::is_same<T, uint32_t>::value || std::is_same<T, uint8_t>::value,
                "T must be either uint32_t or uint8_t");

  // check that the data has been read
  assert(__is_rasterized());
  
  // check that the
  uint64_t dpos = y * m_width + x; 
  if (dpos > m_pixels || x > m_width || y > m_height) {
    fprintf(stderr, "ERROR: Accesing out of bound pixel at (%ul,%ul)\n",x,y);
    assert(false);
  }
    
  return static_cast<T*>(m_data)[dpos];
}

int TiffImage::CopyTags(TIFF *otif) const {

  if (!__is_initialized())
    return 1;
  
  TIFFSetField(otif, TIFFTAG_IMAGEWIDTH, m_width); 
  TIFFSetField(otif, TIFFTAG_IMAGELENGTH, m_height); 
  TIFFSetField(otif, TIFFTAG_SAMPLESPERPIXEL, m_samples_per_pixel);
  TIFFSetField(otif, TIFFTAG_BITSPERSAMPLE, m_bits_per_sample);
  //TIFFSetField(otif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(otif, TIFFTAG_TILEWIDTH, m_tilewidth);
  TIFFSetField(otif, TIFFTAG_TILELENGTH, m_tileheight);
  TIFFSetField(otif, TIFFTAG_PHOTOMETRIC, m_photometric);
  TIFFSetField(otif, TIFFTAG_PLANARCONFIG, m_planar);
  TIFFSetField(otif, TIFFTAG_EXTRASAMPLES, m_extra_samples);

  return 0;
}

double TiffImage::mean() const {

  if (!__is_initialized())
    return -1;
  if (!__is_rasterized())
    return -1;

  double sum = 0;
  uint64_t n; // calculated num pixels. Should be same as m_pixels
  
  if (m_8bit) {
    n = sizeof(m_data) / sizeof(uint8_t);
    assert(n == m_pixels);
    for (size_t i = 0; i < n; ++i)
      sum += static_cast<uint8_t*>(m_data)[i];
  } else {
    n = sizeof(m_data) / sizeof(uint32_t);
    assert(n == m_pixels);
    for (size_t i = 0; i < n; ++i)
      sum += static_cast<uint32_t*>(m_data)[i];
  }

  return (sum / n);

}

int TiffImage::write(TIFF* otif) const {

  // raster has to be read first
  if (m_data == NULL) {
    fprintf(stderr, "ERROR: Trying to write empty image. Try running ReadToRaster first\n");
    return 1;
  }

  // write a tile image
  if (TIFFIsTiled(otif)) {
    return(__tiled_write(otif));
  }

  return 0;
  
}

int TiffImage::__tiled_write(TIFF* otif) const {

  assert(TIFFIsTiled(otif));

  uint32_t o_tile_width = 0;
  uint32_t o_tile_height = 0;
  assert(TIFFGetField(otif, TIFFTAG_TILEWIDTH, &o_tile_width));
  assert(TIFFGetField(otif, TIFFTAG_TILELENGTH, &o_tile_height));

  // error check that tile dimensions
  if (o_tile_width % 16) {
    fprintf(stderr, "ERROR: output tile dims must be multiple of 16, it is: %d\n", o_tile_width);
    return 1;
  }
  if (o_tile_height % 16) {
    fprintf(stderr, "ERROR: output tile dims must be multiple of 16, it is: %d\n", o_tile_height);
    return 1;
  }

  // Allocate a buffer for the output tiles
  void* buf = _TIFFmalloc(TIFFTileSize(otif));
  if (buf == NULL) {
    fprintf(stderr, "Error allocating memory for tiles\n");
    return 1;
  }

  // loop through the output image tiles and write the pixels
  // from the raster
  int x, y;
  for (y = 0; y < m_height; y += o_tile_height) {
    for (x = 0; x < m_width; x += o_tile_width) {
      // Fill the tile buffer with data
      for (int ty = 0; ty < o_tile_height; ty++) {
	for (int tx = 0; tx < o_tile_width; tx++) {
	  uint64_t xp = x + tx; // want 64 bits to be able to store really big d_pos, since this is the nth pixel
	  uint64_t yp = y + ty;
	  uint64_t d_pos = yp * m_width + xp; // index of pixel in the entire image
	  uint64_t t_pos = ty * o_tile_width + tx; // index of pixel in this particular tile
	  if (xp < m_width && yp < m_height) {
	    if (m_8bit) {
	      // static_cast<uint8_t*>(buf)[t_pos] =
	      //static_cast<uint8_t*>(m_data)[d_pos];
	      static_cast<uint8_t*>(buf)[t_pos] = pixel<uint8_t>(xp, yp);
	    } else {
	      //static_cast<uint32_t*>(buf)[t_pos] = static_cast<uint32_t*>(m_data)[d_pos];
	      static_cast<uint32_t*>(buf)[t_pos] = pixel<uint32_t>(xp, yp); 
	    }
	  }
	}
      }
      
      // Write the tile to the TIFF file
      // this function will automatically calculate memory size from TIFF tags
      if (TIFFWriteTile(otif, buf, x, y, 0, 0) < 0) { 
	fprintf(stderr, "Error writing tile at (%d, %d)\n", x, y);
	_TIFFfree(buf);
	return 1;
      }
    }
  }
  
  // Clean up
  _TIFFfree(buf);

  return 0;
}

int TiffImage::__alloc() {

  assert(m_pixels);

  if (m_8bit) {
    m_data = _TIFFmalloc(m_pixels * sizeof(uint8_t));
    //fprintf(stderr, "alloc size %f GB\n", m_pixels / 1e9);
  } else {
    m_data = _TIFFmalloc(sizeof(uint32_t) * m_pixels);
    //fprintf(stderr, "alloc size %f GB\n", m_pixels * 4 / 1e9); 
  }

  if (m_data == NULL) {
    fprintf(stderr, "ERROR: unable to allocate image raster\n");
    return 1;
  }

  return 0;
}

int TiffImage::ReadToRaster(TIFF* tif) {

  // clear the old raster
  clear_raster();
  
  // re-read if this is a different TIFF
  // not ideal behavior
  if (tif != m_tif) {
    fprintf(stderr, "Warning: reading differt TIFF than object initialized as\n");
    fprintf(stderr, "Warning: this may not be what you want\n");    
    __give_tiff(tif);
  }

    
  // re-read if this is different image within tiff
  // NB: to get here, m_tif == tif
  else if (TIFFCurrentDirOffset(tif) != m_current_dir) {
    fprintf(stderr, "re-reading new dir in TIFF %s\n", TIFFFileName(m_tif));
    // when I give this object the same TIFF object, it will still read
    // the *current* image, which is different than the one already stored
    __give_tiff(tif);
  }
  
  // just in time allocation of memory
  __alloc();
  
  // read in tiled image
  if (m_tilewidth) {
   assert(m_tileheight);
    return(__tiled_read(tif));
  }
  
  return 0;
}

int TiffImage::__tiled_read(TIFF* i_tif) {

  // allocate memory for a single tile
  void* tile = _TIFFmalloc(TIFFTileSize(i_tif));

  // loop through the tiles
  int x, y;
  for (y = 0; y < m_height; y += m_tileheight) {
    for (x = 0; x < m_width; x += m_tilewidth) {
      
      // Read the tile
      if (TIFFReadTile(m_tif, tile, x, y, 0, 0) < 0) {
	fprintf(stderr, "Error reading tile at (%d, %d)\n", x, y);
	return 1;
      }

      // Copy the tile data into the image buffer
      for (int ty = 0; ty < m_tileheight; ty++) {
	for (int tx = 0; tx < m_tilewidth; tx++) {
	  if (x + tx < m_width && y + ty < m_height) {
	    if (m_8bit) {
	      static_cast<uint8_t*>(m_data)[(y + ty) * m_width + x + tx] =
		static_cast<uint8_t*>(tile)[ty * m_tilewidth + tx];
	    } else {
	      static_cast<uint32_t*>(m_data)[(y + ty) * m_width + x + tx] =
		static_cast<uint32_t*>(tile)[ty * m_tilewidth + tx];
	    }
	  }
	} // tile x loop
      } // tile y loop
    } // image x loop
  } // image y loop

  _TIFFfree(tile);
    
  return 0;
  
}


void TiffImage::clear_raster() {

  if (m_data == NULL) {
    //fprintf(stderr, "Warning: no raster to clear\n");
    return;
  }

  _TIFFfree(m_data);

}

bool TiffImage::__is_initialized() const {
  if (m_tif == NULL) {
    fprintf(stderr, "Warning: No image TIFF\n");
    return false;
  }
  return true;
}

bool TiffImage::__is_rasterized() const {
  if (m_data == NULL) {
    fprintf(stderr, "Warning: Trying to write empty image. Try running ReadToRaster first\n");    
    return false;
  }
  return true;
}
