#include "tiff_image.h"
#include <cassert>
#include <iostream>

#include <string.h>
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

  //std::cerr << " sum " << AddCommas(static_cast<uint64_t>(sum)) << " np " << AddCommas(m_pixels) << std::endl;
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
  } else {
    std::cerr << "TIFF NOT TILED " << std::endl;
  }

  return 0;
  
}

int TiffImage::light_mean(TIFF* tif) const {

  __check_tif(tif);
  
  size_t mode = __get_mode(tif);
  
  void* buf;
  
  if (TIFFIsTiled(tif)) {
    // allocate memory for a single tile
    buf  = _TIFFmalloc(TIFFTileSize(tif) * mode);
  } else {
    buf  = _TIFFmalloc(TIFFScanlineSize(tif) * mode);
  }
  
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
    
    if (verbose)
      std::cerr << "...starting image " <<
	ifd << " with " << AddCommas(static_cast<uint64_t>(np)) <<
	" pixels" << std::endl;
    
    uint64_t m_pix = 0; // running count for verbose command
    
    if (TIFFIsTiled(tif)) {
	for (y = 0; y < m_height; y += m_tileheight) {
	  for (x = 0; x < m_width; x += m_tilewidth) {
	    
	    // Read the tile
	    if (TIFFReadTile(tif, buf, x, y, 0, 0) < 0) {
	      fprintf(stderr, "Error reading tile at (%d, %d)\n", x, y);
	      return 1;
	    }
	    
	    // Get the mean
	    for (int ty = 0; ty < m_tileheight; ty++) {
	      for (int tx = 0; tx < m_tilewidth; tx++) {
		if (x + tx < m_width && y + ty < m_height) {
		  
		  ++m_pix;
		  if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
		    std::cerr << "...working on pixel: " <<
		      AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
		  
		  switch (mode) {
		  case 1:
		    t += static_cast<uint8_t*>(buf)[ty * m_tilewidth + tx];
		    break;
		  case 3:
		    r += static_cast<uint8_t*>(buf)[(ty * m_tilewidth + tx) * 3    ];
		    g += static_cast<uint8_t*>(buf)[(ty * m_tilewidth + tx) * 3 + 1];
		    b += static_cast<uint8_t*>(buf)[(ty * m_tilewidth + tx) * 3 + 2];
		    break;
		  case 4:
		    t += static_cast<uint32_t*>(buf)[ty * m_tilewidth + tx];	      
		    break;
		  }
		}
	      } // tile x loop
	    } // tile y loop
	  } // image x loop
	} // image y loop
      }
      
      // lined image
      else {
	
	uint64_t ls = TIFFScanlineSize(tif);
	for (uint64_t y = 0; y < m_height; y++) {
	  for (uint64_t x = 0; x < ls; x++) {

	    // Read the line
	    if (TIFFReadScanline(tif, buf, y) < 0) {
	      fprintf(stderr, "Error reading line at row %ul\n", y);
	      return 1;
	    }
	    
	    ++m_pix;
	    if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	      std::cerr << "...working on pixel: " <<
		AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	    
	    switch (mode) {
	    case 1:
	      t += static_cast<uint8_t*>(buf)[x];
	    break;
	    case 3:
	      r += static_cast<uint8_t*>(buf)[x * 3    ];
	      g += static_cast<uint8_t*>(buf)[x * 3 + 1];
	      b += static_cast<uint8_t*>(buf)[x * 3 + 2];
	      break;
	    case 4:
	      t += static_cast<uint32_t*>(buf)[x];	      
	      break;
	    }
	  } // x loop
	} // y loop
      } //scanline else
      
      fprintf(stdout, "------ IFD %d ------\n", ifd);
      switch (mode) {
      case 1:
      case 4:
	fprintf(stdout, "Mean: %f\n", t / np);
	std::cerr << " sum " << AddCommas(static_cast<uint64_t>(t)) << " np " << np << std::endl;
	break;
      case 3:
	fprintf(stdout, "Mean: (R) %f (G) %f (B) %f\n", r / np, g / np, b / np);
	break;
      }
      
      ++ifd;
      } while (TIFFReadDirectory(tif));
    
    _TIFFfree(buf);
    
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
  uint64_t x, y;

  for (y = 0; y < m_height; y += o_tile_height) {
    for (x = 0; x < m_width; x += o_tile_width) {
      // Fill the tile buffer with data
      for (uint64_t ty = 0; ty < o_tile_height; ty++) {
	for (uint64_t tx = 0; tx < o_tile_width; tx++) {
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
	m_data = calloc(m_pixels, sizeof(uint8_t));
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
  if (TIFFIsTiled(tif) && m_tilewidth) {
    assert(m_tileheight);
    return(__tiled_read(tif));
  } else {
    return(__lined_read(tif));
  }
  
  return 0;
}

int TiffImage::__lined_read(TIFF* i_tif) {

  size_t mode = __get_mode(i_tif);

  uint64_t ls = TIFFScanlineSize(i_tif);
  
  // allocate memory for a single line
  void* buf = _TIFFmalloc(TIFFScanlineSize(i_tif));

  uint64_t m_pix = 0;
  for (uint64_t y = 0; y < m_height; y++) {

    // Read the line
    if (TIFFReadScanline(i_tif, buf, y) < 0) {
      fprintf(stderr, "Error reading line at row %ul\n", y);
      return 1;
    }
    
    // Copy the tile data into the image buffer
    for (uint64_t x = 0; x < ls; x++) {
      if (x < m_width && y < m_height) {
	
	++m_pix;
	if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	  std::cerr << "...working on pixel: " <<
	    AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	
	uint64_t ind = y * m_width + x;
	
	assert(static_cast<uint8_t*>(m_data)[ind] == 0);
	switch (mode) {
	case 1:
	  static_cast<uint8_t*>(m_data)[ind] =
	    static_cast<uint8_t*>(buf)[x];
	  break;
	case 4:
	  static_cast<uint32_t*>(m_data)[ind] =
	    static_cast<uint32_t*>(buf)[x];
	  break;
	}
      }
    } // x loop
  }
  
  _TIFFfree(buf);
  
  return 0;
}



int TiffImage::__tiled_read(TIFF* i_tif) {

  size_t mode = __get_mode(i_tif);
  
  // allocate memory for a single tile
  void* tile = (void*)calloc(TIFFTileSize(i_tif), sizeof(uint8_t));
  
  uint64_t tile_size = static_cast<uint64_t>(m_tileheight) * m_tilewidth;						 

  // loop through the tiles
  uint64_t x, y;
  uint64_t m_pix = 0;
  for (y = 0; y < m_height; y += m_tileheight) {
    for (x = 0; x < m_width; x += m_tilewidth) {
      
      // Read the tile
      if (TIFFReadTile(i_tif, tile, x, y, 0, 0) < 0) {
	fprintf(stderr, "Error reading tile at (%d, %d)\n", x, y);
	return 1;
      }
      
      // Copy the tile data into the image buffer
      for (uint64_t ty = 0; ty < m_tileheight; ty++) {
	for (uint64_t tx = 0; tx < m_tilewidth; tx++) {
	  uint64_t ind = (y + ty) * m_width + x + tx;
	  if (x + tx < m_width && y + ty < m_height) {
	    
	    ++m_pix;
	    if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	      std::cerr << "...working on pixel: " <<
		AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	    
	    assert(static_cast<uint8_t*>(m_data)[ind] == 0);
	    switch (mode) {
	  case 1:
	    static_cast<uint8_t*>(m_data)[ind] =
	      static_cast<uint8_t*>(tile)[ty * m_tilewidth + tx];
	    break;
	  case 4:
	    static_cast<uint32_t*>(m_data)[ind] =
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

void TiffImage::__gray8assert(TIFF* in) const {

  uint16_t bps, photo;
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bps);
  assert(bps == 8);
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photo);
  assert(photo == PHOTOMETRIC_MINISBLACK);
  
}

int TiffImage::MergeGrayToRGB(TIFF* r, TIFF* g, TIFF* b, TIFF* o) {

  // assert that they are 8 bit images
  __gray8assert(r);
  __gray8assert(g);
  __gray8assert(b);  

  // for tiled images
  if (TIFFIsTiled(r)) {
    
    assert(TIFFTileSize(r) == TIFFTileSize(g));
    assert(TIFFTileSize(r) == TIFFTileSize(b));
    assert(TIFFTileSize(r) == TIFFTileSize(o));    
    
    // allocate memory for a single tile
    uint8_t* r_tile = (uint8_t*)calloc(TIFFTileSize(r), sizeof(uint8_t));
    uint8_t* g_tile = (uint8_t*)calloc(TIFFTileSize(g), sizeof(uint8_t));
    uint8_t* b_tile = (uint8_t*)calloc(TIFFTileSize(b), sizeof(uint8_t));
    void*    o_tile = (void*)calloc(TIFFTileSize(r) * 3, sizeof(uint8_t));  
    
    // loop through the tiles
    uint64_t x, y;
    uint64_t m_pix = 0;
    for (y = 0; y < m_height; y += m_tileheight) {
      for (x = 0; x < m_width; x += m_tilewidth) {
	
	// Read the red tile
	if (TIFFReadTile(r, r_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading red tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
	// Read the green tile
	if (TIFFReadTile(g, g_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading green tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
	// Read the blue tile
	if (TIFFReadTile(b, b_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading blue tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
	// copy the tile
	for (size_t i = 0; i < TIFFTileSize(r); ++i) {
	  
	  ++m_pix;
	  if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	    std::cerr << "...working on pixel: " <<
	      AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	  
	  static_cast<uint8_t*>(o_tile)[i*3    ] = r_tile[i];
	  static_cast<uint8_t*>(o_tile)[i*3 + 1] = g_tile[i];
	  static_cast<uint8_t*>(o_tile)[i*3 + 2] = b_tile[i];
	}
	
	// Write the tile to the TIFF file
	// this function will automatically calculate memory size from TIFF tags
	if (TIFFWriteTile(o, o_tile, x, y, 0, 0) < 0) { 
	  fprintf(stderr, "Error writing tile at (%d, %d)\n", x, y);
	  return 1;
	  
	}
	
      } // end x loop
    } // end y loop
    
    free(o_tile);
    free(r_tile);
    free(g_tile);
    free(b_tile);

  }

  // lined image
  else {

    uint64_t ls = TIFFScanlineSize(r);

    assert(TIFFScanlineSize(r) == TIFFScanlineSize(g));
    assert(TIFFScanlineSize(r) == TIFFScanlineSize(b));    

    // allocate memory for a single line
    uint8_t* rbuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(r));
    uint8_t* gbuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(g));
    uint8_t* bbuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(b));
    uint8_t* obuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(r) * 3);

    if (verbose)
      std::cerr << "--SCANLINE SIZE " << TIFFScanlineSize(r) << std::endl;
    
    uint64_t m_pix = 0;
    for (uint64_t y = 0; y < m_height; y++) {
      
      // Read the red line
      if (TIFFReadScanline(r, rbuf, y) < 0) {
	fprintf(stderr, "Error reading red line at row %ul\n", y);
	return 1;
      }

      // Read the green line
      if (TIFFReadScanline(g, gbuf, y) < 0) {
	fprintf(stderr, "Error reading green line at row %ul\n", y);
	return 1;
      }

      // Read the blue line
      if (TIFFReadScanline(b, bbuf, y) < 0) {
	fprintf(stderr, "Error reading blue line at row %ul\n", y);
	return 1;
      }

      // copy the line
      for (size_t i = 0; i < TIFFScanlineSize(r); ++i) {
	
	++m_pix;
	if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	  std::cerr << "...working on pixel: " <<
	    AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	
	static_cast<uint8_t*>(obuf)[i*3  ] = rbuf[i];
	static_cast<uint8_t*>(obuf)[i*3+1] = gbuf[i];
	static_cast<uint8_t*>(obuf)[i*3+2] = bbuf[i];
      }
      
      // Write the tile to the TIFF file
      // this function will automatically calculate memory size from TIFF tags
      if (TIFFWriteScanline(o, obuf, y) < 0) { 
	fprintf(stderr, "Error writing line row %ul\n", y);
	return 1;
      }
    } // end row loop

    free(obuf);
    free(rbuf);
    free(gbuf);
    free(bbuf);
  }
  
  return 0;
}

int TiffImage::DirCount(TIFF* in) const {
  
  // make sure dircount is right
  int dircount = 0;
  do {
    dircount++;
  } while (TIFFReadDirectory(in));

  TIFFSetDirectory(in, 0);
  return dircount;
  
}

int TiffImage::MergeGrayToRGB(TIFF* in, TIFF* out) const {

  int dircount = DirCount(in);
  
  if (dircount < 3) {
    std::cerr << "Error: Need at least three image IFDs" << std::endl;
    return 1;
  }
  
  // assert that they are 8 bit images
  // assert that they are grayscale
  for (size_t i = 0; i < 3; ++i) {
    TIFFSetDirectory(in, i);
    __gray8assert(in);
  }
  
  // 
  // for tiled images
  TIFFSetDirectory(in, 0);
  if (TIFFIsTiled(in)) {

    uint64_t ts = TIFFTileSize(in);
    for (int i = 1; i < 3; i++) {
      TIFFSetDirectory(in, 1);
      assert(TIFFTileSize(in) == ts);
    }
    
    // allocate memory for a single tile
    uint8_t* r_tile = (uint8_t*)calloc(ts, sizeof(uint8_t));
    uint8_t* g_tile = (uint8_t*)calloc(ts, sizeof(uint8_t));
    uint8_t* b_tile = (uint8_t*)calloc(ts, sizeof(uint8_t));
    void*    o_tile = (void*)calloc(ts* 3, sizeof(uint8_t));  

    // loop through the tiles
    uint64_t x, y;
    uint64_t m_pix = 0;
    for (y = 0; y < m_height; y += m_tileheight) {
      for (x = 0; x < m_width; x += m_tilewidth) {
	
	// Read the red tile
	TIFFSetDirectory(in, 0);
	if (TIFFReadTile(in, r_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading red tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
	// Read the green tile
	TIFFSetDirectory(in, 1);	
	if (TIFFReadTile(in, g_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading green tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
	// Read the blue tile
	TIFFSetDirectory(in, 2);
	if (TIFFReadTile(in, b_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading blue tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
	// copy the tile
	for (size_t i = 0; i < TIFFTileSize(in); ++i) {

	  ++m_pix;
	  if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	    std::cerr << "...working on pixel: " <<
	      AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	  
	  static_cast<uint8_t*>(o_tile)[i*3    ] = r_tile[i];
	  static_cast<uint8_t*>(o_tile)[i*3 + 1] = g_tile[i];
	  static_cast<uint8_t*>(o_tile)[i*3 + 2] = b_tile[i];
	}
	
	// Write the tile to the TIFF file
	// this function will automatically calculate memory size from TIFF tags
	if (TIFFWriteTile(out, o_tile, x, y, 0, 0) < 0) { 
	  fprintf(stderr, "Error writing tile at (%d, %d)\n", x, y);
	  return 1;
	}
	
      } // end x loop
    } // end y loop
    
    free(o_tile);
    free(r_tile);
    free(g_tile);
    free(b_tile);

  }

  // lined image
  else {

    // assert that all of the image scanlines are the same
    TIFFSetDirectory(in, 0);
    uint64_t ls = TIFFScanlineSize(in);

    for (int i = 1; i < 3; i++) {
      TIFFSetDirectory(in, i);
      assert(TIFFScanlineSize(in) == ls);
    }
    
    // allocate memory for a single line
    uint8_t* rbuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(in));
    uint8_t* gbuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(in));
    uint8_t* bbuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(in));
    uint8_t* obuf = (uint8_t*)_TIFFmalloc(TIFFScanlineSize(in) * 3);

    uint64_t m_pix = 0;
    for (uint64_t y = 0; y < m_height; y++) {
      
      // Read the red line
      TIFFSetDirectory(in, 0);
      if (TIFFReadScanline(in, rbuf, y) < 0) {
	fprintf(stderr, "Error reading red line at row %ul\n", y);
	return 1;
      }

      // Read the green line
      TIFFSetDirectory(in, 1);      
      if (TIFFReadScanline(in, gbuf, y) < 0) {
	fprintf(stderr, "Error reading green line at row %ul\n", y);
	return 1;
      }

      // Read the blue line
      TIFFSetDirectory(in, 2);
      if (TIFFReadScanline(in, bbuf, y) < 0) {
	fprintf(stderr, "Error reading blue line at row %ul\n", y);
	return 1;
      }

      // copy the line
      for (size_t i = 0; i < TIFFScanlineSize(in); ++i) {
	
	++m_pix;
	if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	  std::cerr << "...working on pixel: " <<
	    AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	
	static_cast<uint8_t*>(obuf)[i*3  ] = rbuf[i];
	static_cast<uint8_t*>(obuf)[i*3+1] = gbuf[i];
	static_cast<uint8_t*>(obuf)[i*3+2] = bbuf[i];
      }
      
      // Write the tile to the TIFF file
      // this function will automatically calculate memory size from TIFF tags
      if (TIFFWriteScanline(out, obuf, y) < 0) { 
	fprintf(stderr, "Error writing line row %ul\n", y);
	return 1;
      }
    } // end row loop

    free(obuf);
    free(rbuf);
    free(gbuf);
    free(bbuf);

    
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
