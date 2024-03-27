#include "tiff_utils.h"
#include "tiff_cp.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <cstring>
#include <algorithm> // for std::min and std::max and std::fill_n
#include <cstdint>   // for uint16_t and uint8_t

#include "channel.h"

#define MEAN_THRESHOLD 300
#define DIFF_THRESHOLD 300

// Define the struct with x and y members
struct Point {
    int x, y;
  
  // Equality comparison operator is needed for unordered_set
  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
};

// Specialize std::hash for the Point struct
namespace std {
  template<>
  struct hash<Point> {
    std::size_t operator()(const Point& p) const noexcept {
      // Combine the hash of x and y in some way.
      // A common approach is to use std::hash<int>() and then combine them.
      // This example uses the shifting method to combine hashes but there are better methods.
      return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
  };
}

// Macro to get a TIFF tag from the input and set it on the output.
// Assumes `in` is the source TIFF* and `out` is the destination TIFF*.

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

int Mask(TIFF* in, TIFF* out) {

  // display number of directories / channels
  int num_dir = TIFFNumberOfDirectories(in);
  std::cerr << "Number of channels in image: " << num_dir << std::endl;

  // vector index is channel num
  std::vector<std::unordered_set<Point>> xy_to_keep;
  
  // loop each channel
  for (int n = 0; n < num_dir; n++) {

    // drop factor
    float drop = 0;
    
    TIFFSetDirectory(in, n);

#ifdef WRITEOUT    
    // Create a new directory for the output file
    if (n > 0) {  // No need to create the first directory, it's automatically created
      // Finalize the previous directory and create new one
      if (!TIFFWriteDirectory(out)) {
	std::cerr << "Error: Could not write output directory " << n << std::endl;
	return 1;
      }
    }

    // copy fields
    tiffcpjw(in, out);

    assert(TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_LZW));
#else
    // get image dimensions
    uint64_t m_height = 0;
    uint64_t m_width = 0;
    TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &m_width);
    TIFFGetField(in, TIFFTAG_IMAGELENGTH, &m_height);
#endif
    
    if (TIFFIsTiled(in)) {

      // copy tile info
      uint64_t tileheight = 0, tilewidth = 0;
#ifdef WRITEOUT      
      COPY_TIFF_TAG(in, out, TIFFTAG_TILEWIDTH, tilewidth);
      COPY_TIFF_TAG(in, out, TIFFTAG_TILELENGTH, tileheight);
#else
      TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth);
      TIFFGetField(in, TIFFTAG_TILELENGTH, &tileheight);
#endif
      
      // tile calculation
      assert(tileheight);
      assert(tilewidth);      
      uint32_t tiles_across = (m_width + tilewidth - 1) / tilewidth;
      uint32_t tiles_down   = (m_height + tileheight - 1) / tileheight;
      uint32_t tiles_per_image = tiles_across * tiles_down;

      uint64_t ts = TIFFTileSize(in);
      
      // allocate memory for a single tile
      uint16_t* itile = (uint16_t*)calloc(ts/ 2, sizeof(uint16_t));
      if (itile == nullptr) {
	std::cerr << "Memory allocation for channels failed." << std::endl;
	assert(false);
      }
      
      // loop through the tiles
      uint64_t x, y;
      uint64_t m_pix = 0;
      size_t num_tiles = 0;
      for (y = 0; y < m_height; y += tileheight) {
	for (x = 0; x < m_width; x += tilewidth) {
	  
	  num_tiles++;
	  // Read the input tile
	  if (TIFFReadTile(in, itile, x, y, 0, 0) < 0) {
	    fprintf(stderr, "Error reading input channel %d tile at (%llu, %llu)\n", n, x, y);
	    return 1;
	  }

	  // sort a copy of this array to get quantiles
	  int arrSize = ts / 2;
	  //uint16_t arrCopy[arrSize];
	  uint16_t* arrCopy = new uint16_t[arrSize];
	  std::memcpy(arrCopy, itile, arrSize * sizeof(uint16_t)); 
	  std::sort(arrCopy, arrCopy + arrSize);

	  uint16_t percentile_95 = arrCopy[static_cast<int>(0.95 * arrSize)];
	  uint16_t percentile_5 = arrCopy[static_cast<int>(0.05 * arrSize)];
	  uint16_t diff = percentile_95 - percentile_5;

	  delete[] arrCopy;
	  
	  // get the mean for the tile
	  uint64_t sum = 0;
	  for (size_t i = 0; i < arrSize; i++) {
	    sum += itile[i];
	  }

	  // if mean is above threshold, then copy data
	  if ( (sum / arrSize) >= MEAN_THRESHOLD || diff > DIFF_THRESHOLD) {
	    //memcpy(otile, itile, (arrSize) * sizeof(uint16_t));
	    // OLD // std::fill_n(otile, arrSize, static_cast<uint8_t>(255));

	    //Note that std::memset sets every byte to the specified value.
	    // If otile actually points to an array of uint16_t,
	    // and you use std::memset(otile, 255, arrSize * sizeof(uint16_t));
	    //each byte is set to 0xFF, meaning that each uint16_t element will
	    //be 0xFFFF because memset works on a byte level.
	    
	    Point p;
	    p.x = x;
	    p.y = y;
	    
	    // this is a GOOD tile
	    if (n == xy_to_keep.size()) {
	      std::unordered_set<Point> pair;
	      pair.insert(p);
	      xy_to_keep.push_back(pair);
	    } else {
	      xy_to_keep[n].insert(p);
	    }

	    std::cerr << " keeping " << x << "," << y << " Mean " <<
	      (sum / arrSize) << " diff " << diff << std::endl;
	  } else {
	    std::cerr << " mean: " << (sum / arrSize)  << " 5% " <<
	    percentile_5 << " 9% " << percentile_95 <<  " diff " <<
	    diff << std::endl;
#ifdef WRITEOUT	    
	    std::memset(otile, 255, arrSize * sizeof(uint8_t));
#endif	    
	    drop++;
	  }

	  // x2 is because two bytes
	  //std::cout << (sum / ts * 2) << std::endl;
	  
	  // Write the tile to the TIFF file
	  // this function will automatically calculate memory size from TIFF tags
#ifdef WRITEOUT	  
	  if (TIFFWriteTile(out, otile, x, y, 0, 0) < 0) { 
	    fprintf(stderr, "Error writing tile at (%llu, %llu)\n", x, y);
	    return 1;
	  }
#endif	  
	} // end x loop
      } // end y loop

#ifdef WRITEOUT      
      std::cerr << "...finished channel " << n << " - " <<
	m_height << " x " << m_width << 
	" drop rate " << (drop/num_tiles) << std::endl;
      free(otile);
#endif      
      free(itile);
      
    } // end tiff tiled?
    
    // not implemented yet, but this is TIFF is row
    else {
#ifdef WRITEOUT      
      // strip offsets
      uint32_t rows_per_strip = 0;    
      COPY_TIFF_TAG_QUIET(in, out, TIFFTAG_ROWSPERSTRIP, rows_per_strip);
      uint32_t strips_per_image = 0;
      if (rows_per_strip)
	strips_per_image = std::floor((m_height + rows_per_strip - 1) / rows_per_strip);
#endif      
      // missing STRIPOFFSETS
      // missing STRIPBYTECOUNTS
    }
    break; // debug
  } // end channel loop

  // loop

  ///////
  // write the image
  ///////

  // copy the tifftags
  TIFFSetDirectory(in, 0);
  tiffcpjw(in, out);

  TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
  TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);

  // get image dimensions
  uint64_t m_height = 0;
  uint64_t m_width = 0;
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &m_width);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &m_height);
  
  // get tile sizes
  uint64_t ts = TIFFTileSize(in);
  uint64_t tileheight = 0, tilewidth = 0;
  COPY_TIFF_TAG(in, out, TIFFTAG_TILEWIDTH, tilewidth);
  COPY_TIFF_TAG(in, out, TIFFTAG_TILELENGTH, tileheight);
  
  // loop through the tiles
  uint64_t x, y;
  uint64_t m_pix = 0;
  size_t num_tiles = 0;
  for (y = 0; y < m_height; y += tileheight) {
    for (x = 0; x < m_width; x += tilewidth) {
      
      // allocated the output tile
      void* otile = (void*)calloc(ts / 2, sizeof(uint8_t));  // div by 2 because uint16
      
      Point p;
      p.x = x;
      p.y = y;
      if (xy_to_keep[0].find(p) != xy_to_keep[0].end()) {
	std::memset(otile, 255, ts /2 * sizeof(uint8_t));	
      }
      if (TIFFWriteTile(out, otile, x, y, 0, 0) < 0) { 
	fprintf(stderr, "Error writing tile at (%llu, %llu)\n", x, y);
	return 1;
      }
    }
  }
  
  if (!TIFFWriteDirectory(out)) {
    std::cerr << "Could not write final output directory " << std::endl;
    return 1;
  }
  
  return 0;
}

int Colorize(TIFF* in, TIFF* out, const std::string& palette_file,
	     const std::vector<int>& channels_to_run,
	     bool verbose) {

  // set compression
  TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
  
  // display number of directories / channels
  int num_dir = TIFFNumberOfDirectories(in);
  if (verbose)
    std::cerr << "Number of channels in image: " << num_dir << std::endl;

  ////// READ THE PALETTE
  ChannelVector channels;
  std::ifstream file(palette_file);
  std::string line; 
  std::vector<std::string> headerItems; 
  while (std::getline(file, line)) {
    if (!line.empty() || line.at(0) == '#') {
      channels.emplace_back(line); 
    }
  }

  // input checking
  if (channels_to_run.size() == 0) {
    std::cerr << "no channels selected" << std::endl;
    return 0;
  }

  // subset to just the channels that we want to colorize
  ChannelVector channels_to_run_map;
  for (auto n : channels_to_run) {
    channels_to_run_map.push_back(channels.at(n));
  }

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
  if (verbose)
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
    size_t tile_num = 1;

    // num tiles, using divisor trick to round up
    int num_tiles = ((m_height + tileheight - 1) / tileheight) * ((m_width + tilewidth - 1) / tilewidth);
    
    for (y = 0; y < m_height; y += tileheight) {
      if (verbose)
	std::cerr << "...working on tile " << tile_num << " of " << num_tiles << std::endl;
      for (x = 0; x < m_width; x += tilewidth) {
	tile_num++;
	// copy in the 3tiles from channels
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
