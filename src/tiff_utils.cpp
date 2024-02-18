#include "tiff_utils.h"
#include <cassert>
#include <iostream>

#include <algorithm> // for std::min and std::max
#include <cstdint>   // for uint16_t and uint8_t

uint8_t affineTransformValue(uint16_t value, uint16_t A, uint16_t B, uint8_t C, uint8_t D) {
  
    // Calculate the scaling factor
    double scale = (B > A) ? (double)(D - C) / (B - A) : 0;
    
    if (value <= A) {
        return C;
    } else if (value >= B) {
        return D;
    } else {
        // Perform the affine transformation for values in range [A, B]
        return static_cast<uint8_t>(C + (value - A) * scale);
    }
}

static void __gray8assert(TIFF* in) {

  uint16_t bps, photo;
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bps);
  assert(bps == 8);
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photo);
  assert(photo == PHOTOMETRIC_MINISBLACK);
  
}

int Colorize(TIFF* in, TIFF* out) {
  
  TIFFSetDirectory(in, 0);

  uint32_t m_height = 0;
  uint32_t m_width = 0;
  
  // I'm going to assume here that all three sub-images have same tile size
  if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &m_width)) {
    std::cerr << " ERROR getting image width " << std::endl;
    return 1;
  }
  if (!TIFFGetField(in, TIFFTAG_IMAGELENGTH, &m_height)) {
    std::cerr << " ERROR getting image height " << std::endl;
    return 1;
  }

  // display number of directories / channels
  int num_dir = TIFFNumberOfDirectories(in);
  std::cerr << "Channels: " << num_dir << std::endl;
  
  TIFFSetDirectory(in, 0);
  if (TIFFIsTiled(in)) {
    
    uint32_t tileheight = 0;
    uint32_t tilewidth = 0;
    
    // I'm going to assume here that all three sub-images have same tile size
    if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth)) {
      std::cerr << " ERROR getting tile width " << std::endl;
      return 1;
    }
    if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tileheight)) {
      std::cerr << " ERROR getting tile height " << std::endl;
      return 1;
    }
    
    uint64_t ts = TIFFTileSize(in);
    for (int i = 1; i < num_dir; i++) {
      TIFFSetDirectory(in, 1);
      assert(TIFFTileSize(in) == ts);
    }

    // allocate memory for a single tile
    uint16_t* r_tile = (uint16_t*)calloc(ts /2, sizeof(uint16_t));
    uint16_t* g_tile = (uint16_t*)calloc(ts /2, sizeof(uint16_t));
    uint16_t* b_tile = (uint16_t*)calloc(ts /2, sizeof(uint16_t));
    void*     o_tile = (void*)calloc(ts / 2 * 3, sizeof(uint8_t));  

    // loop through the tiles
    uint64_t x, y;
    uint64_t m_pix = 0;
    for (y = 0; y < m_height; y += tileheight) {
      for (x = 0; x < m_width; x += tilewidth) {
	
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
	for (size_t i = 0; i < (ts / 2); ++i) {

	  /*
	  ++m_pix;
	  if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	    std::cerr << "...working on pixel: " <<
	      AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	  */

	  /*std::cerr << " r_tile[" << i << "] = " << static_cast<int>(r_tile[i]) << " affine transform " <<
	    static_cast<int>(affineTransformValue(r_tile[i], 1000,16000,0,255)) << std::endl;
	  std::cerr << " g_tile[" << i << "] = " << static_cast<int>(g_tile[i]) << " affine transform " <<
	    static_cast<int>(affineTransformValue(g_tile[i], 0,500,0,255)) << std::endl;
	  std::cerr << " b_tile[" << i << "] = " << static_cast<int>(b_tile[i]) << " affine transform " <<
	    static_cast<int>(affineTransformValue(b_tile[i], 1000,16000,0,255)) << std::endl;
	  */
	  
	  static_cast<uint8_t*>(o_tile)[i*3    ] = 100; //static_cast<uint8_t>(affineTransformValue(i,0, ts/2,0,255));
	  static_cast<uint8_t*>(o_tile)[i*3 + 1] = 0; //static_cast<uint8_t>(affineTransformValue(i,0, ts/2,0,255)); //g_tile[i];
	  static_cast<uint8_t*>(o_tile)[i*3 + 2] = 0; //static_cast<uint8_t>(affineTransformValue(i,0, ts/2,0,255)); //b_tile[i];
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

  return 0;
  
}

int MergeGrayToRGB(TIFF* in, TIFF* out) {

  int dircount = TIFFNumberOfDirectories(in);
  
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
  
  uint32_t m_height = 0;
  uint32_t m_width = 0;
  
  // I'm going to assume here that all three sub-images have same tile size
  if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &m_width)) {
    std::cerr << " ERROR getting image width " << std::endl;
    return 1;
  }
  if (!TIFFGetField(in, TIFFTAG_IMAGELENGTH, &m_height)) {
    std::cerr << " ERROR getting image height " << std::endl;
    return 1;
  }
  
  
  // 
    // for tiled images
  TIFFSetDirectory(in, 0);
  if (TIFFIsTiled(in)) {

    uint32_t tileheight = 0;
    uint32_t tilewidth = 0;
    
    // I'm going to assume here that all three sub-images have same tile size
    if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth)) {
      std::cerr << " ERROR getting tile width " << std::endl;
      return 1;
    }
    if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tileheight)) {
      std::cerr << " ERROR getting tile height " << std::endl;
      return 1;
    }
    
    
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
    for (y = 0; y < m_height; y += tileheight) {
      for (x = 0; x < m_width; x += tilewidth) {
	
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

	  /*
	  ++m_pix;
	  if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	    std::cerr << "...working on pixel: " <<
	      AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	  */
	  
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
	
	/*	++m_pix;
		if (verbose && m_pix % static_cast<uint64_t>(1e9) == 0)
	  std::cerr << "...working on pixel: " <<
	    AddCommas(static_cast<uint64_t>(m_pix)) << std::endl;
	*/
	
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
