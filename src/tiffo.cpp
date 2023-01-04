#include <stdint.h>
#include <iostream>

#include <vector>
#include <tiffio.h>

#include <memory>
#include <typeinfo>
#include <cmath>
#include <getopt.h>

#include "tiff_header.h"
#include "tiff_image.h"
#include "tiff_cp.h"

namespace opt {
  static bool verbose = false;
  static std::string infile;
  static std::string outfile;
  static std::string module;
}

#define DEBUG(x) std::cerr << #x << " = " << (x) << std::endl

#define TVERB(msg) \
  if (opt::verbose) {		   \
    std::cerr << msg << std::endl; \
  }

static const char* shortopts = "v";
static const struct option longopts[] = {
  { "verbose",                    no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 }
};

static const char *RUN_USAGE_MESSAGE =
"Usage: tiffo [module] <options> infile outfile\n"
"Modules:\n"
"  gray2rgb - Convert a 3-channel gray TIFF to a single RGB\n"
"  mean - Give the mean pixel for each channel\n"
  "\n";
  
static int gray2rgb();
static int findmean();
static void parseRunOptions(int argc, char** argv);

/*
  https://github.com/LuaDist/libtiff/blob/43d5bd6d2da90e9bf254cd42c377e4d99008f00b/libtiff/tiffio.h#L61
  
typedef	uint32 ttag_t;		// directory tag 
typedef	uint16 tdir_t;		// directory index
typedef	uint16 tsample_t;	// sample number
typedef	uint32 tstrip_t;	// strip number
typedef uint32 ttile_t;		// tile number
typedef	int32 tsize_t;		// i/o size in bytes
typedef	void* tdata_t;		// image data ref
typedef	uint32 toff_t;		// file offset 

 */

static TIFF* tiffload(const char* file, const std::string &mode) {

  // read in the TIFF
  TIFF* tif = TIFFOpen(file, mode.c_str());

  if (tif == NULL || tif == 0) {
    fprintf(stderr, "Unable to open tif: %s\n", file);
    return NULL;
  }
  
  return tif;;
  
}

int main(int argc, char **argv) {
  
  // Check if a command line argument was provided
  if (argc < 2) {
    std::cerr << "Error: missing command line argument" << std::endl;
    std::cerr << RUN_USAGE_MESSAGE;
    return 1;
  }

  //opt::module = argv[1];
  
  parseRunOptions(argc, argv);
  
  // get the module
  for (int i = 0; i < argc; i++)
  if (opt::module == "gray2rgb")
    return(gray2rgb());
  else if (opt::module == "mean")
    return(findmean());
  else {
  }

  return 1;
}

int findmean() {

  // open a tiff
  // the "m" keeps it from being memory mapped, which for
  // a single pass just causes memory overruns from the C memmap func
  TIFF* tif = tiffload(opt::infile.c_str(), "rm");

  TiffImage im(tif);
  im.setverbose(opt::verbose);

  // this routine will handle printing output to stdout
  im.light_mean(tif);

  return 0;
}

static int gray2rgb() {

  TIFF* itif = tiffload(opt::infile.c_str(), "rm");

  // messing around with the tiff header
  TiffHeader thead(opt::infile);
  //thead.view_stdout();
  
  // read in the TIFF as a red channel
  TVERB("...reading red");
  TIFF *r_itif = TIFFOpen(opt::infile.c_str(), "rm");
  TiffImage rimage(r_itif);  
  rimage.setverbose(opt::verbose);
  rimage.ReadToRaster(r_itif);

  TVERB("R mean: " << rimage.mean(r_itif));

  //debug writing raster
  /*std::cerr << " writing raster" << std::endl;
  TIFF *debug = TIFFOpen("debug.tif", "w8");
  tiffcp(r_itif, debug, opt::verbose);
  rimage.write(debug);
  TIFFClose(debug);
  return 0;
  */
  
  // Open the output TIFF file
  TIFF* otif = TIFFOpen(opt::outfile.c_str(), "w8");
  if (otif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", opt::outfile);
    return 1;
  }

  // copy all of the tags 
  tiffcp(r_itif, otif, false);

  // setup the RGB output file
  TiffImage rgb(r_itif);

  TIFFSetField(otif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(otif, TIFFTAG_SAMPLESPERPIXEL, 3);

  // close the red channel, we don't need it anymore
  TIFFClose(r_itif);
  
  // read in the TIFF as a green channel
  TVERB("...reading green");
  TIFF *g_itif = TIFFOpen(opt::infile.c_str(), "rm");
  TiffImage gimage;
  gimage.setverbose(opt::verbose);  
  if (!TIFFReadDirectory(g_itif)) {
    fprintf(stderr, "Unable to read second channel\n");
  }
  gimage = TiffImage(g_itif);
  gimage.ReadToRaster(g_itif);

  TVERB("G mean: " << gimage.mean(g_itif));
  
  TIFFClose(g_itif);
  
  // read in the TIFF as a blue channel
  std::cerr << "...reading blue" << std::endl;
  TIFF *b_itif = TIFFOpen(opt::infile.c_str(), "rm");
  TiffImage bimage;
  TIFFReadDirectory(b_itif); // advance to 2
  if (!TIFFReadDirectory(b_itif)) { // advance to 3
    fprintf(stderr, "Unable to read third channel\n");
  }
  bimage = TiffImage(b_itif);
  bimage.setverbose(opt::verbose);
  bimage.ReadToRaster(b_itif);
  
  TVERB("B mean: " << bimage.mean(b_itif));
  
  TVERB("...copying to the RGB raster");
  rgb.MergeGrayToRGB(rimage, gimage, bimage);
  
  // write it
  TVERB("...writing output to file: " << TIFFFileName(otif));
  rgb.write(otif);
  
  // close the image for writing
  TIFFClose(otif);  

  return 0;
}

// parse the command line options
static void parseRunOptions(int argc, char** argv) {
  bool die = false;

  if (argc <= 2) 
    die = true;

  bool help = false;
  std::stringstream ss;

  int sample_number = 0;
  
  std::string tmp;
  for (char c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
    std::istringstream arg(optarg != NULL ? optarg : "");
    switch (c) {
    case 'v' : opt::verbose = true; break;
    case 'h' : help = true; break;
    default: die = true;
    }
  }
  
  // Process any remaining no-flag options
  while (optind < argc) {
    if (opt::module.empty()) {
      opt::module = argv[optind];      
    } else if (opt::infile.empty()) {
      opt::infile = argv[optind];
    } else if (opt::outfile.empty()) {
      opt::outfile = argv[optind];
    } 
    optind++;
  }
  
  if (! (opt::module == "gray2rgb" || opt::module == "mean") ) {
    std::cerr << "Module " << opt::module << " not implemented" << std::endl;
    die = true;
  }
  
  if (die || help) 
    {
      std::cerr << "\n" << RUN_USAGE_MESSAGE;
      if (die)
	exit(EXIT_FAILURE);
      else 
	exit(EXIT_SUCCESS);	
    }
}

