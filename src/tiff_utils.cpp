#include "tiff_utils.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>

#include <algorithm> // for std::min and std::max
#include <cstdint>   // for uint16_t and uint8_t

#include "channel.h"

uint8_t affineTransformUint8(uint64_t value, uint64_t A, uint64_t B) {
  
  // Calculate the scaling factor
  uint64_t C = 0;
  uint64_t D = 255;
  double scale = (B > A) ? (double)(D - C) / (B - A) : 0;

  if (value <= A) {
    return C;
  } else if (value >= B) {
    return D;
  } else {
    // Perform the affine transformation for values in range [A, B]
    return static_cast<uint8_t>((value - A) * scale);
  }
}

// Modified combineChannelsToRGB function to return an RGBColor object
RGBColor combineChannelsToRGB(const std::vector<uint16_t>& values, const std::vector<Channel>& channels) {
  
    if (values.size() != channels.size()) {
        throw std::invalid_argument("The length of values and channels vectors must be equal.");
    }

    std::array<uint32_t, 3> rgbSum = {0, 0, 0};
    for (size_t i = 0; i < channels.size(); ++i) {
        uint8_t windowedValue = affineTransformUint8(values[i], channels[i].lowerBound, channels[i].upperBound);
        rgbSum[0] += channels[i].color.r * windowedValue;
        rgbSum[1] += channels[i].color.g * windowedValue;
        rgbSum[2] += channels[i].color.b * windowedValue;
    }

    // Normalize the combined RGB values to fit into uint8_t range
    RGBColor rgb;
    for (size_t i = 0; i < 3; ++i) {
        rgbSum[i] = (rgbSum[i] > 255 * 255) ? 255 * 255 : rgbSum[i]; // Clamp to max value before dividing
        if (i == 0) rgb.r = static_cast<uint8_t>(rgbSum[i] / 255);
        if (i == 1) rgb.g = static_cast<uint8_t>(rgbSum[i] / 255);
        if (i == 2) rgb.b = static_cast<uint8_t>(rgbSum[i] / 255);
    }

    return rgb;
}

// Function to allocate memory for N channels, each of a specified size
uint16_t** allocateChannels(size_t numChannels, size_t tileSize) {
    // Allocate an array of pointers to hold the addresses of the arrays for each channel
    uint16_t** channels = (uint16_t**)calloc(numChannels, sizeof(uint16_t*));
    if (channels == nullptr) {
        std::cerr << "Failed to allocate memory for channel pointers." << std::endl;
        return nullptr;
    }

    // Allocate memory for each channel
    for (size_t i = 0; i < numChannels; ++i) {
        channels[i] = (uint16_t*)calloc(tileSize, sizeof(uint16_t));
        if (channels[i] == nullptr) {
            std::cerr << "Failed to allocate memory for channel " << i << std::endl;
            // Cleanup previously allocated memory before returning
            for (size_t j = 0; j < i; ++j) {
                free(channels[j]);
            }
            free(channels);
            return nullptr;
        }
    }

    return channels;
}

// Function to free the allocated memory for N channels
void freeChannels(uint16_t** channels, size_t numChannels) {
    if (channels != nullptr) {
        for (size_t i = 0; i < numChannels; ++i) {
            free(channels[i]); // Free each channel
        }
        free(channels); // Free the array of pointers
    }
}

static void __gray8assert(TIFF* in) {
  
  uint16_t bps, photo;
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bps);
  assert(bps == 8);
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photo);
  assert(photo == PHOTOMETRIC_MINISBLACK);
  
}

int Compress(TIFF* in, TIFF* out) {

  // display number of directories / channels
  int num_dir = TIFFNumberOfDirectories(in);
  std::cerr << "Number of channels in image: " << num_dir << std::endl;
  
  // loop each channel
  for (int n = 0; n < num_dir; n++) {
    
    TIFFSetDirectory(in, n);
    std::cerr << "...working on directory " << n << std::endl;
    
    uint32_t m_height = 0;
    uint32_t m_width = 0;
    
    if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &m_width)) {
      std::cerr << " ERROR getting image width " << std::endl;
      return 1;
    }
    if (!TIFFGetField(in, TIFFTAG_IMAGELENGTH, &m_height)) {
      std::cerr << " ERROR getting image height " << std::endl;
      return 1;
    }

    // debug
    std::cerr << "...height " << m_height << " width " << m_width << std::endl;
    
    if (TIFFIsTiled(in)) {
      
      uint32_t tileheight = 0;
      uint32_t tilewidth = 0;
      
      if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth)) {
	std::cerr << " ERROR getting tile width " << std::endl;
	return 1;
      }
      if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tileheight)) {
	std::cerr << " ERROR getting tile height " << std::endl;
	return 1;
      }
      
      uint64_t ts = TIFFTileSize(in);
      
      // allocate memory for a single tile
      uint16_t* itile = (uint16_t*)calloc(ts / 2, sizeof(uint16_t));
      
      if (itile == nullptr) {
	std::cerr << "Memory allocation for channels failed." << std::endl;
	assert(false);
      }
      
      // allocated the output tile
      void*     otile = (void*)calloc(ts / 2, sizeof(uint16_t));  // div by 2 because uint16
      
    // loop through the tiles
      uint64_t x, y;
      uint64_t m_pix = 0;
      for (y = 0; y < m_height; y += tileheight) {
	for (x = 0; x < m_width; x += tilewidth) {
	  
	  // Read the input tile
	  if (TIFFReadTile(in, itile, x, y, 0, 0) < 0) {
	    fprintf(stderr, "Error reading input channel %d tile at (%llu, %llu)\n", n, x, y);
	    return 1;
	  }
	  
	  // get the mean for the tile
	  uint64_t sum = 0;
	  for (size_t i = 0; i < ts / 2; i++) {
	    sum += itile[i];
	  }
      
	  // if mean is above threshold, then copy data
	  if (sum / ts * 2 >= 10)
	    memcpy(otile, itile, (ts / 2) * sizeof(uint16_t));
	  
	  // Write the tile to the TIFF file
	  // this function will automatically calculate memory size from TIFF tags
	  if (TIFFWriteTile(out, otile, x, y, 0, 0) < 0) { 
	    fprintf(stderr, "Error writing tile at (%llu, %llu)\n", x, y);
	    return 1;
	  }
	} // end x loop
      } // end y loop
      
      free(otile);
      free(itile);

      assert(TIFFWriteDirectory(out));
    } // end tiff tiled?

    
  } // end channel loop
  return 0;
}

int Colorize(TIFF* in, TIFF* out, const std::string& palette_file,
	     const std::vector<int>& channels_to_run) {
  
  // display number of directories / channels
  int num_dir = TIFFNumberOfDirectories(in);
  std::cerr << "Number of channels in image: " << num_dir << std::endl;

  ////// READ THE PALETTE
  ChannelVector channels;
  std::ifstream file(palette_file);
  std::string line; 
  std::vector<std::string> headerItems; 
  while (std::getline(file, line)) {

    std::stringstream ss(line);
    std::string item;
    std::vector<std::string> headerItems;
    
    // Split the header line into parts based on comma
    while (std::getline(ss, item, ',')) {
      headerItems.push_back(item);
    }

    // Check if the header has exactly 7 elements
    if (headerItems.size() != 7) {
      std::cerr << "Error: The header does not contain exactly 7 elements." << std::endl;
      return -1; 
    }

    // read the actual channel information
    while (std::getline(file, line)) {
      if (!line.empty()) {
	channels.emplace_back(line); 
      }
    }
  }

  // input checking
  if (channels_to_run.size() == 0) {
    std::cerr << "no channels selected" << std::endl;
    return 0;
  }
  
  // subset to just the channels that we want to colorize
  ChannelVector channels_to_run_map;
  for (auto n : channels_to_run)
    channels_to_run_map.push_back(channels.at(n));

  // error checking
  int channel_max = *std::max_element(channels_to_run.begin(), channels_to_run.end());
  if (channel_max >= channels.size()) {
    fprintf(stderr, "Error: Max channel %d is larger than number of channels in the palette %zu\n", channel_max, channels.size());
    return -1;
  }

  // error checking
  if (num_dir <= channel_max) {
    fprintf(stderr, "Error: Max channel %d is larger than number of channels in the image %d\n", channel_max, num_dir);
    return -1;
  }
  
  // print
  for (const auto& i : channels_to_run)
    std::cerr << "Channel: " << channels.at(i) << std::endl;
  
  if (TIFFIsTiled(in)) {
    
    // check that all of the tile sizes are the same
    TIFFSetDirectory(in, channels_to_run[0]);
    uint64_t ts = TIFFTileSize(in);
    for (int i = 1; i < channels_to_run.size(); i++) {
      TIFFSetDirectory(in, channels_to_run[i]);
      assert(TIFFTileSize(in) == ts);
    }

    // get the image data for this
    uint32_t tileheight = 0;
    uint32_t tilewidth = 0;
    uint32_t m_height = 0;
    uint32_t m_width = 0;
    
    if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &m_width)) {
      std::cerr << " ERROR getting image width " << std::endl;
      return 1;
    }
    if (!TIFFGetField(in, TIFFTAG_IMAGELENGTH, &m_height)) {
      std::cerr << " ERROR getting image height " << std::endl;
      return 1;
    }
    
    if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth)) {
      std::cerr << " ERROR getting tile width " << std::endl;
      return 1;
    }
    if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tileheight)) {
      std::cerr << " ERROR getting tile height " << std::endl;
      return 1;
    }
    
    // allocate memory for a single tile
    uint16_t** channels = allocateChannels(channels_to_run.size(), ts / 2);
    if (channels == nullptr) {
      std::cerr << "Memory allocation for channels failed." << std::endl;
      assert(false);
    }

    // allocated the RGB tile
    void*     o_tile = (void*)calloc(ts / 2 * 3, sizeof(uint8_t));  // div by 2 because uint16 -> uint8, then *3 because R, G, B
    
    // storage for pixel values
    std::vector<uint16_t> pixel_values;
    pixel_values.resize(channels_to_run.size());
    
    // loop through the tiles
    uint64_t x, y;
    uint64_t m_pix = 0;
    for (y = 0; y < m_height; y += tileheight) {
      for (x = 0; x < m_width; x += tilewidth) {
	
	// copy in the tiles from channels
	size_t channel_num = 0;
	for (const auto& m : channels_to_run) {
	  
	  // Read the red tile
	  TIFFSetDirectory(in, m);
	  if (TIFFReadTile(in, channels[channel_num], x, y, 0, 0) < 0) {
	    fprintf(stderr, "Error reading channel %d tile at (%llu, %llu)\n", m, x, y);
	    return 1;
	  }
	  channel_num++;
	}
	
	// copy the tile to the RGB, pixel by pixel
	for (size_t i = 0; i < (ts / 2); ++i) {
	  
	  // copy the channel number + values to a map
	  int n = 0;
	  for (const auto& m : channels_to_run) {
	    pixel_values[n] = channels[n][i];
	    n++;
	  }

	  //RGBColor rgb = RGBColor(0,0,0);
	  RGBColor rgb =
	    combineChannelsToRGB(pixel_values, channels_to_run_map);
	  
	  static_cast<uint8_t*>(o_tile)[i*3    ] = rgb.r;
	  static_cast<uint8_t*>(o_tile)[i*3 + 1] = rgb.g; //affineTransformUint8(g_tile[i], 200, 1500);
	  static_cast<uint8_t*>(o_tile)[i*3 + 2] = rgb.b; //ffineTransformUint8(b_tile[i], 400, 1500);
	}
	
	// Write the tile to the TIFF file
	// this function will automatically calculate memory size from TIFF tags
	if (TIFFWriteTile(out, o_tile, x, y, 0, 0) < 0) { 
	  fprintf(stderr, "Error writing tile at (%llu, %llu)\n", x, y);
	  return 1;
	}
	
      } // end x loop
    } // end y loop

    // Free the allocated memory
    freeChannels(channels, channels_to_run.size());
    
    free(o_tile);

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
	  fprintf(stderr, "Error reading red tile at (%llu, %llu)\n", x, y);
	  return 1;
	}
	
	// Read the green tile
	TIFFSetDirectory(in, 1);	
	if (TIFFReadTile(in, g_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading green tile at (%llu, %llu)\n", x, y);
	  return 1;
	}
	
	// Read the blue tile
	TIFFSetDirectory(in, 2);
	if (TIFFReadTile(in, b_tile, x, y, 0, 0) < 0) {
	  fprintf(stderr, "Error reading blue tile at (%llu, %llu)\n", x, y);
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
	  fprintf(stderr, "Error writing tile at (%llu, %llu)\n", x, y);
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
	fprintf(stderr, "Error reading red line at row %llu\n", y);
	return 1;
      }

      // Read the green line
      TIFFSetDirectory(in, 1);      
      if (TIFFReadScanline(in, gbuf, y) < 0) {
	fprintf(stderr, "Error reading green line at row %llu\n", y);
	return 1;
      }

      // Read the blue line
      TIFFSetDirectory(in, 2);
      if (TIFFReadScanline(in, bbuf, y) < 0) {
	fprintf(stderr, "Error reading blue line at row %llu\n", y);
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
	fprintf(stderr, "Error writing line row %llu\n", y);
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
