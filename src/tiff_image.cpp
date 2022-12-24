#include "tiff_image.h"
#include <cassert>
#include <iostream>

#define DP(x) fprintf(stderr,"DEBUG %d\n", x)
#define DEBUG(var) printf(#var " = %d\n", var)


TiffImage::TiffImage(TIFF* tif) {

  // clear the raster data
  // note that TiffImage class is NOT responsible for managing TIFF memory
  clear_raster();

  // add the tiff
  __give_tiff(tif);
  
}

int TiffImage::__give_tiff(TIFF *tif) {

  __check_tif(tif);
  
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

  return 0;
}

template <typename T>
T TiffImage::pixel(uint64_t x, uint64_t y, int p) const {

  assert(p == PIXEL_GRAY || p == PIXEL_RED || p == PIXEL_GREEN || p == PIXEL_BLUE || p == PIXEL_ALPHA);
  
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
  
  if (p == PIXEL_GRAY) 
    return static_cast<T*>(m_data)[dpos];

  // else its rgb and so add the pixel offset
  return static_cast<T*>(m_data)[dpos*3 + p];
}

template <typename T>
T TiffImage::element(uint64_t e) const {

  // right now only two types allowed
  static_assert(std::is_same<T, uint32_t>::value || std::is_same<T, uint8_t>::value,
                "T must be either uint32_t or uint8_t");

  // check that the data has been read
  assert(__is_rasterized());
  
  // check that the element is in bounds
  if (e > m_pixels) {
    fprintf(stderr, "ERROR: Accesing out of bound pixel (flattened) at (%ul)\n",e);
    assert(false);
  }
    
  return static_cast<T*>(m_data)[e];
}


double TiffImage::mean(TIFF* tif) const {

  if (!__is_initialized())
    return -1;
  if (!__is_rasterized())
    return -1;

  size_t mode = __get_mode(tif);
  
  double sum = 0;
  
  switch (mode) {
  case 1:
    for (size_t i = 0; i < m_pixels; ++i) {
      sum += static_cast<uint8_t*>(m_data)[i];
    }
    break;
  case 4:
    for (size_t i = 0; i < m_pixels; ++i)
      sum += static_cast<uint32_t*>(m_data)[i];
    break;
  }
  
  return (sum / m_pixels);

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

int TiffImage::light_mean(TIFF* tif) const {

  __check_tif(tif);
  
  size_t mode = __get_mode(tif);

  // allocate memory for a single tile
  void* tile  = _TIFFmalloc(TIFFTileSize(tif) * 3);

  // loop through the tiles
  int x, y;

  size_t ifd = 0;

  double np = static_cast<uint64_t>(m_width) * m_height;
  
  do {

    __check_tif(tif);

    double r = 0;
    double g = 0;
    double b = 0;
    double t = 0;
    
    for (y = 0; y < m_height; y += m_tileheight) {
      for (x = 0; x < m_width; x += m_tilewidth) {
	
	// Read the tile
	if (TIFFReadTile(tif, tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
	// Get the mean
	for (int ty = 0; ty < m_tileheight; ty++) {
	  for (int tx = 0; tx < m_tilewidth; tx++) {
	    if (x + tx < m_width && y + ty < m_height) {
	      switch (mode) {
	      case 1:
		t += static_cast<uint8_t*>(tile)[ty * m_tilewidth + tx];
		break;
	      case 3:
		r += static_cast<uint8_t*>(tile)[(ty * m_tilewidth + tx) * 3    ];
		g += static_cast<uint8_t*>(tile)[(ty * m_tilewidth + tx) * 3 + 1];
		b += static_cast<uint8_t*>(tile)[(ty * m_tilewidth + tx) * 3 + 2];
		break;
	      case 4:
		t += static_cast<uint32_t*>(tile)[ty * m_tilewidth + tx];	      
		break;
	      }
	    }
	  } // tile x loop
	} // tile y loop
      } // image x loop
    } // image y loop

    fprintf(stdout, "------ IFD %d ------\n", ifd);
    switch (mode) {
    case 1:
    case 4:
      fprintf(stdout, "Mean: %f\n", t / np);
      break;
    case 3:
      fprintf(stdout, "Mean: (R) %f (G) %f (B) %f\n", r / np, g / np, b / np);    
    }

    ++ifd;
  } while (TIFFReadDirectory(tif));
    
  _TIFFfree(tile);
    
  return 0;
  
}

int TiffImage::__tiled_write(TIFF* otif) const {

  assert(TIFFIsTiled(otif));

  uint32_t o_tile_width = 0;
  uint32_t o_tile_height = 0;
  assert(TIFFGetField(otif, TIFFTAG_TILEWIDTH, &o_tile_width));
  assert(TIFFGetField(otif, TIFFTAG_TILELENGTH, &o_tile_height));

  uint16_t samples_per_pixel;
  uint16_t bits_per_sample;
  assert(TIFFGetField(otif, TIFFTAG_BITSPERSAMPLE, &bits_per_sample));
  assert(TIFFGetField(otif, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel));

  int m_mode = __get_mode(otif);
  assert(TIFFTileSize(otif) / (o_tile_width * o_tile_height) == m_mode);

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
      if (y==384)
      // Fill the tile buffer with data
      for (int ty = 0; ty < o_tile_height; ty++) {
	for (int tx = 0; tx < o_tile_width; tx++) {
	  uint64_t xp = x + tx; // want 64 bits to be able to store really big d_pos, since this is the nth pixel
	  uint64_t yp = y + ty;
	  uint64_t d_pos = yp * m_width + xp; // index of pixel in the entire image
	  uint64_t t_pos = ty * o_tile_width + tx; // index of pixel in this particular tile
	  if (xp < m_width && yp < m_height) {
	    if (m_mode == 1) {
	      static_cast<uint8_t*>(buf)[t_pos] = pixel<uint8_t>(xp, yp, PIXEL_GRAY);
	    } else if (m_mode == 4){
	      static_cast<uint32_t*>(buf)[t_pos] = pixel<uint32_t>(xp, yp, PIXEL_GRAY); 
	    } else if (m_mode == 3) {
	      t_pos = t_pos * 3;
	      static_cast<uint8_t*>(buf)[t_pos]     = pixel<uint8_t>(xp, yp, PIXEL_RED);
	      static_cast<uint8_t*>(buf)[t_pos + 1] = pixel<uint8_t>(xp, yp, PIXEL_GREEN);
	      static_cast<uint8_t*>(buf)[t_pos + 2] = pixel<uint8_t>(xp, yp, PIXEL_BLUE);
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

int TiffImage::__alloc(TIFF* tif) {

  assert(m_pixels);

  size_t mode = __get_mode(tif);
  
  switch (mode) {
      case 1:
	m_data = _TIFFmalloc(m_pixels * sizeof(uint8_t));
	//fprintf(stderr, "alloc size %f GB\n", m_pixels / 1e9);
	break;
	case 4:
	  m_data = _TIFFmalloc(sizeof(uint32_t) * m_pixels);
	  //fprintf(stderr, "alloc size %f GB\n", m_pixels * 4 / 1e9);
	  break;
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
  __alloc(tif);
  
  // read in tiled image
  if (m_tilewidth) {
   assert(m_tileheight);
    return(__tiled_read(tif));
  }
  
  return 0;
}

int TiffImage::__tiled_read(TIFF* i_tif) {

  size_t mode = __get_mode(i_tif);
  
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
	    switch (mode) {
	    case 1:
	      static_cast<uint8_t*>(m_data)[(y + ty) * m_width + x + tx] =
		static_cast<uint8_t*>(tile)[ty * m_tilewidth + tx];
	      break;
	    case 4:
	      static_cast<uint32_t*>(m_data)[(y + ty) * m_width + x + tx] =
		static_cast<uint32_t*>(tile)[ty * m_tilewidth + tx];
	      break;
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

int TiffImage::MergeGrayToRGB(const TiffImage &r, const TiffImage &g, const TiffImage &b) {

  // will need a bunch of error checking here
  clear_raster();

  assert(r.numPixels() == g.numPixels());
  assert(g.numPixels() == b.numPixels());

  m_pixels = r.numPixels();
  
  // allocate memory for a new TIFF
  m_data = _TIFFmalloc(m_pixels * 3 * sizeof(uint8_t));
  
  if (m_data == NULL) {
    fprintf(stderr, "Unable to allocate memory in MergeGrayToRGB\n");
    return 1;
  }

  for (size_t i = 0; i < m_pixels * 3; ++i) {

    static_cast<uint8_t*>(m_data)[i    ] = r.element<uint8_t>(i / 3);
    static_cast<uint8_t*>(m_data)[i + 1] = g.element<uint8_t>(i / 3);
    static_cast<uint8_t*>(m_data)[i + 2] = b.element<uint8_t>(i / 3);        
    
    /*    uint32_t rgb = r.element<uint8_t>(i);
    rgb = (rgb << 8) | g.element<uint8_t>(i);
    rgb = (rgb << 8) | b.element<uint8_t>(i);
    //rgb = (rgb << 8) | static_cast<uint8_t>(255); // alpha
    static_cast<uint32_t*>(m_data)[i] = rgb;
    //static_cast<uint8_t*>(m_data)[i] = r.element<uint8_t>(i);
    */
  }

  return 0;
}

size_t TiffImage::__get_mode(TIFF* tif) const {

  // RGB
  if (GetIntTag(tif, TIFFTAG_SAMPLESPERPIXEL) == 3)
    return 3;
  else if (GetIntTag(tif, TIFFTAG_BITSPERSAMPLE) == 8)
    return 1;
  else if (GetIntTag(tif, TIFFTAG_BITSPERSAMPLE) == 16)
    return 2;
  else if (GetIntTag(tif, TIFFTAG_BITSPERSAMPLE) == 32)
    return 4;
  else if (GetIntTag(tif, TIFFTAG_SAMPLESPERPIXEL) == 4)
    return 5;
  return 0;
  
}

int TiffImage::__check_tif(TIFF* tif) const {

  if (tif == NULL || tif == 0) {
    fprintf(stderr, "Error: TIFF is NULL\n");
    return 1;
  }
  return 0;
}
