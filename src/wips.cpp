#include <stdint.h>
#include <assert.h>
#include <vector>

#include <fstream>

#include "wips.h"

#include "tiff_writer.h"
#include "tiff_reader.h"
#include "tiff_header.h"

#define TILE_WIDTH 16
#define TILE_HEIGHT 16

using vips::VImage;

int main(int argc, char **argv) {

  if (VIPS_INIT (argv[0])) 
    vips_error_exit (NULL);
  
  if (argc != 3)
    vips_error_exit ("usage: %s input-file output-file", argv[0]);

  // read in the Tiff header and do basic error checks
  // and print the results
  /*
  TiffHeader header(argv[1]);

  header.view_stdout();
  */

  // open the input tiff
  TiffReader itif(argv[1]);

  // open the output tiff
  TiffWriter otif(argv[2]);
  
  int dircount = 0;
    
  // Set the TIFF parameters
  TIFFSetField(otif.GetTiff(), TIFFTAG_IMAGEWIDTH, width); //TILE_WIDTH); //debug width
  TIFFSetField(otif.GetTiff(), TIFFTAG_IMAGELENGTH, height); //TILE_HEIGHT); //debug height
  TIFFSetField(otif.GetTiff(), TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(otif.GetTiff(), TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(otif.GetTiff(), TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  //TIFFSetField(otif.GetTiff(), TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(otif.GetTiff(), TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(otif.GetTiff(), TIFFTAG_TILEWIDTH, TILE_WIDTH);
  TIFFSetField(otif.GetTiff(), TIFFTAG_TILELENGTH, TILE_HEIGHT);
  
  /*
  //debug
  uint8_t* duffer = new uint8_t[TILE_WIDTH * TILE_HEIGHT];
  for (int i = 0; i < TILE_HEIGHT; i++) {
  for (int j = 0; j < TILE_WIDTH; j++) {
  duffer[i * TILE_WIDTH + j] = i * TILE_WIDTH + j;
  }
  }
  TIFFWriteTile(otif, duffer, 0, 0, 0, 0);
  
  return 0;
  */
  
  ///////
  // read in the entire first image
  ///////
  
  //std::vector<std::vector<uint8_t>> data(width, std::vector<uint8_t>(height));

  /*
  // Read the image data
  uint32_t* buff = (uint32_t*) _TIFFmalloc(height * sizeof(uint32_t)); //samples_per_pixel * (bits_per_sample / 8));      
  for (uint32_t y = 0; y < height; y++) {
  TIFFReadScanline(itif, buff, y);
  std::cerr << " line " << y << std::endl;
  for (int i = 0; i < width; ++i)
  data[y * width + i] = TIFFGetR(buff[i]);
  }
  */

  itif.ReadImage();
  
  
  //  DEBUG
  /*
  std::cerr << " data.size  "<< data.size() << " " << data[0].size() << std::endl; 
  std::cerr << "("<<0<<","<<0<<") ="<<data[0][0] << std::endl;
  //debug print the tile
  for (int i = 0; i < 200; i++) {
    for (int j = 0; j < 200; j++) {
      std::cerr << "data[" << i << "][" << j << "] = " << static_cast<unsigned int>(data[i][j]) << std::endl;
    }
  }
  return 0;
  */
  
  dircount++;
  
  // Allocate a buffer for the output tiles
  uint8_t *outtilebuf = (uint8_t*)_TIFFmalloc(TIFFTileSize(otif));
  if (outtilebuf == NULL) {
    fprintf(stderr, "Error allocating memory for tiles\n");
    return 1;
  }
  
  // Write the tiles to the TIFF file
  int num_height_tiles = (int)std::ceil((double)height / TILE_HEIGHT);
  int num_width_tiles  = (int)std::ceil((double)width /  TILE_WIDTH);
  std::cerr << " h " << width << " numh " << num_height_tiles << " w " << width << " numw " << num_width_tiles << std::endl;
  std::cerr << " writing " << argv[2] << std::endl;
  for (y = 0; y < num_height_tiles; y++) {
    for (x = 0; x < num_width_tiles; x++) {
      //std::cerr << "tile " << x << "," << y << std::endl;
      // Fill the tile buffer with data
      //if (x + tx < width && y + ty < height) {
      // data[(y + ty) * width + (x + tx)] = (uint8_t)TIFFGetR(tile[ty * tile_width + tx]);
      for (int ty = 0; ty < TILE_HEIGHT; ty++) {
	for (int tx = 0; tx < TILE_WIDTH; tx++) {
	  //if (x == 16 && y == 16) //debug
	  //  std::cerr << "[" << y * TILE_WIDTH + tx << "," <<
	  //    y * TILE_HEIGHT + ty << "] = " << static_cast<unsigned int>(data[x * TILE_WIDTH+ tx][y * TILE_HEIGHT + ty]) << endl;
	  //    std::cerr << (x * num_width_tiles + tx) << "," << y * num_height_tiles + ty << std::endl;
	  if ( (x * TILE_WIDTH + tx) < width && (y * TILE_HEIGHT + ty) < height) {
	    //if (x == 16 && y == 16)
	      //std::cerr << " PARKING " << std::endl;
	    outtilebuf[ty * TILE_WIDTH + tx] = data[x * TILE_WIDTH + tx][y * TILE_HEIGHT + ty];
	  }
	}
      }

      //std::cerr << " WRITING TILE " << x << " , " << y << " random " << static_cast<unsigned int>(outtilebuf[100]) << std::endl;
      // Write the tile to the TIFF file
      
      if (TIFFWriteTile(otif, outtilebuf, x * TILE_WIDTH, y * TILE_HEIGHT, 0, 0) < 0) {
	fprintf(stderr, "Error writing tile at (%d, %d)\n", x, y);
	_TIFFfree(outtilebuf);
	return 1;
      }
      
      
    }
  }

  // Clean up
  _TIFFfree(outtilebuf);

  return 0;
  
  //} while (TIFFReadDirectory(itif));
  printf("%d directories in %s\n", dircount, argv[1]);
  
  return 0;
  
  // Open the input image using the VImage class
  VImage image = VImage::new_from_file(argv[1],
				       VImage::option()->set("access", VIPS_ACCESS_SEQUENTIAL));
  
  // compute statistics on the image
  //double average = image.avg();
  std::cout << std::string(image.filename()) << " -- " << image.width() << " x " << image.height() << " bands: " << image.bands() << std::endl <<
    " -- format: " << vips_band_fmt[image.format()] << std::endl <<
    " -- interpretation: "<< vips_interpretation_fmt[image.interpretation()] <<
    std::endl;

  //std::cerr << "...loading into memory" << std::endl;
  //const void *d = image.data();

  
  VImage s = image.stats();
  
  return 0;
  
  // Compute the histogram of the image using the VImage::hist_find() method
  std::cout << "...computing histogram" << std::endl;
  VImage hist = image.hist_find();

  hist.write_to_file (argv[2]);
  /*
  
  // Iterate over the histogram data and print the values to the standard output
  for (int i = 0; i < hist.width(); i++)
    {
      std::cout << hist(0,i) << std::endl;
    }
  */
  return 0;
}
