#include <stdint.h>
#include <iostream>
#include <zlib.h>

#include <vector>
#include <tiffio.h>

#include <memory>
#include <typeinfo>
#include <cmath>
#include <getopt.h>

#include "tiff_header.h"
#include "tiff_image.h"
#include "tiff_reader.h"
#include "tiff_writer.h"
#include "tiff_cp.h"

namespace opt {
  static bool verbose = false;
  static std::string infile;
  static std::string quantfile;
  static std::string markerfile;  
  static std::string outfile;
  static std::string module;

  static std::string redfile;
  static std::string greenfile;
  static std::string bluefile;
  static int threads = 1;
}

#define DEBUG(x) std::cerr << #x << " = " << (x) << std::endl

#define TVERB(msg) \
  if (opt::verbose) {		   \
    std::cerr << msg << std::endl; \
  }

static const char* shortopts = "hvr:g:b:q:c:m:p:C:";
static const struct option longopts[] = {
  { "verbose",                    no_argument, NULL, 'v' },
  { "red",                        required_argument, NULL, 'r' },
  { "green",                      required_argument, NULL, 'g' },
  { "blue",                       required_argument, NULL, 'b' },
  { "quant-file",                 required_argument, NULL, 'q' },
  { "marker-file",                required_argument, NULL, 'm' },  
  { "threads",                    required_argument, NULL, 'c' },
  { "palette",                    required_argument, NULL, 'p' },
  { "channels",                   required_argument, NULL, 'C' },  
  { NULL, 0, NULL, 0 }
};

static const char *RUN_USAGE_MESSAGE =
"Usage: tiffo [module] <options> infile outfile\n"
"Modules:\n"
"  compress - Zero out noise-only tiles for better compression\n"
"  gray2rgb - Convert a 3-channel gray TIFF to a single RGB\n"
"  colorize - Colorize select channels from a cycif tiff\n"
"  mean - Give the mean pixel for each channel\n"
"  csv - <placeholder for csv processing>\n"
  "\n";

static int compress(int argc, char** argv);
static int gray2rgb(int argc, char** argv);
static int findmean(int argc, char** argv);
static int colorize(int argc, char** argv);
static void parseRunOptions(int argc, char** argv);

// process in and outfile cmd arguments
static bool in_out_process(int argc, char** argv);
static bool in_only_process(int argc, char** argv);
static bool check_readable(const std::string& filename);

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

int main(int argc, char **argv) {
  
  // Check if a command line argument was provided
  if (argc < 2) {
    std::cerr << RUN_USAGE_MESSAGE;
    return 1;
  }

  parseRunOptions(argc, argv);
  
  // get the module
  if (opt::module == "gray2rgb") {
    return(gray2rgb(argc, argv));
  } else if (opt::module == "compress") {
    return(compress(argc, argv));
  } else if (opt::module == "colorize") {
    return(colorize(argc, argv));
  } else if (opt::module == "mean") {
    return(findmean(argc, argv));
  } else {
    assert(false);
  }
  return 1;
}

static int findmean(int argc, char** argv) {

  bool die = false;
  const char* shortopts = "v";
  for (char c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
    std::istringstream arg(optarg != NULL ? optarg : "");
    switch (c) {
    case 'v' : opt::verbose = true; break;
    default: die = true;
    }
  }

  if (die || in_only_process(argc, argv)) {
    
    const char *USAGE_MESSAGE =
      "Usage: tiffo findmean [tiff] <options>\n"
      "  Print the mean pixel value of a TIFF\n"
      "  -v, --verbose             Increase output to stderr"
      "\n";
    std::cerr << USAGE_MESSAGE;
    return 1;
  }

  // open a tiff
  // the "m" keeps it from being memory mapped, which for
  // a single pass just causes memory overruns from the C memmap func
  TiffReader reader(opt::infile.c_str());

  // this routine will handle printing output to stdout
  reader.print_means();

  return 0;
}

static int gray2rgb(int argc, char** argv) {

  bool die = false;
  const char* shortopts = "v";
  for (char c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
    std::istringstream arg(optarg != NULL ? optarg : "");
    switch (c) {
    case 'v' : opt::verbose = true; break;
    default: die = true;
    }
  }

  if (die || in_out_process(argc, argv)) {
    
    const char *USAGE_MESSAGE =
      "Usage: tiffo gray2rgb [tiff] [tiff out] <options>\n"
      "  Convert a 3-channel grayscale image (8-bit) to RGB\n"
      "  -v, --verbose             Increase output to stderr\n"
      "\n";
    std::cerr << USAGE_MESSAGE;
    return 1;
  }
  
  // open either the red channel or the 3-IFD file
  TIFF *r_itif = TIFFOpen(opt::infile.c_str(), "rm");

  // Open the output TIFF file
  TIFF* otif = TIFFOpen(opt::outfile.c_str(), "w8");
  if (otif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", opt::outfile.c_str());
    return 1;
  }
  
  // copy all of the tags from in to out
  tiffcp(r_itif, otif, false);
  
  // if this is a single 3 IFD file
  MergeGrayToRGB(r_itif, otif);
  
  TIFFClose(r_itif);
  TIFFClose(otif);
  
  return 0;
}


static int compress(int argc, char** argv) {

  bool die = false;
  const char* shortopts = "v";
  for (char c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
    std::istringstream arg(optarg != NULL ? optarg : "");
    switch (c) {
    case 'v' : opt::verbose = true; break;
    default: die = true;
    }
  }

  if (die || in_out_process(argc, argv)) {
    
    const char *USAGE_MESSAGE =
      "Usage: tiffo compress [tiff in] [tiff out] <options>\n"
      "  Zero out tiles with low signal, to improve compression ratio\n"
      "  -v, --verbose             Increase output to stderr\n"
      "\n";
    std::cerr << USAGE_MESSAGE;
    return 1;
  }

  
  // open either the red channel or the 3-IFD file
  TIFF *r_itif = TIFFOpen(opt::infile.c_str(), "rm");
  
  // Open the output TIFF file
  TIFF* otif = TIFFOpen(opt::outfile.c_str(), "w8");
  if (otif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", opt::outfile.c_str());
    return 1;
  }
  // copy all of the tags from in to out
    //tiffcp2(r_itif, otif, false);

  Compress(r_itif, otif);
  
  TIFFClose(r_itif);
  TIFFClose(otif);

  return 0;

}

static int colorize(int argc, char** argv) {

  bool die = false;
  std::string palette;
  std::vector<int> channels;
  
  const char* shortopts = "vc:p:";
  for (char c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
    std::istringstream arg(optarg != NULL ? optarg : "");
    switch (c) {
    case 'v' : opt::verbose = true; break;
    case 'p' : arg >> palette; break;      
    case 'c' : 
      {
      std::string token;
      while (std::getline(arg, token, ',')) 
	channels.push_back(std::stoi(token));
      }
      break;  
      
    default: die = true;
    }
  }
  

  if (die || in_out_process(argc, argv)) {
    const char *USAGE_MESSAGE =
      "Usage: tiffo colorize [16-bit tiff] [rgb tiff] <options>\n"
      "  Color a 16-bit multichannel tiff to certain channels and with pre-specified palette\n"
      "    -c                Comma-separated list of channels (e.g. 0,1,4,5)\n"
      "    -p                Palette file of format: number,name,r,g,b,lower,upper\n"
      "\n";
    std::cerr << USAGE_MESSAGE;
    return 1;
  }

  // open either the red channel or the 3-IFD file
  TIFF *r_itif = TIFFOpen(opt::infile.c_str(), "rm");

  // Open the output TIFF file
  TIFF* otif = TIFFOpen(opt::outfile.c_str(), "w8");
  if (otif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", opt::outfile.c_str());
    return 1;
  }
  
  // copy all of the tags from in to out
  //tiffcp(r_itif, otif, true);

  //std::cerr << tiffprint(r_itif) << std::endl;

  uint32_t tileheight, tilewidth, width, height;
  assert(TIFFGetField(r_itif, TIFFTAG_IMAGEWIDTH, &width));
  assert(TIFFGetField(r_itif, TIFFTAG_IMAGELENGTH, &height));
  TIFFSetField(otif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(otif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(otif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
 
  if (!TIFFGetField(r_itif, TIFFTAG_TILEWIDTH, &tilewidth)) {
    std::cerr << " ERROR getting tile width " << std::endl;
    return 1;
  }
  if (!TIFFGetField(r_itif, TIFFTAG_TILELENGTH, &tileheight)) {
    std::cerr << " ERROR getting tile height " << std::endl;
    return 1;
  }

  if (!TIFFSetField(otif, TIFFTAG_TILEWIDTH, tilewidth)) {
    fprintf(stderr, "ERROR: unable to set tile width %ul on writer\n", tilewidth);
    return 1;
  }
  if (!TIFFSetField(otif, TIFFTAG_TILELENGTH, tileheight)) {
    fprintf(stderr, "ERROR: unable to set tile height %ul on writer\n", tileheight);
    return 1;
  }
  
  TIFFSetField(otif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(otif, TIFFTAG_SAMPLESPERPIXEL, 3);
  TIFFSetField(otif, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(otif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  //TIFFSetField(otif, TIFFTAG_IMAGEDESCRIPTION, "");
  //TIFFSetField(otif, TIFFTAG_SOFTWARE, "");

  //std::cerr << tiffprint(otif) << std::endl;
  
  // if this is a single 3 IFD file
  Colorize(r_itif, otif, palette, channels, opt::verbose);
  
  TIFFClose(r_itif);
  TIFFClose(otif);
  
  return 0;
}


// parse the command line options
static void parseRunOptions(int argc, char** argv) {
  bool die = false;

  if (argc <= 1) 
    die = true;
  else
    opt::module = argv[1];
  
  /*  bool help = false;
  std::stringstream ss;

  int sample_number = 0;
  
  std::string tmp;
  for (char c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
    std::istringstream arg(optarg != NULL ? optarg : "");
    switch (c) {
    case 'v' : opt::verbose = true; break;
    case 'r' : arg >> opt::redfile; break;
    case 'g' : arg >> opt::greenfile; break;
    case 'p' : arg >> opt::palette; break;
    case 'b' : arg >> opt::bluefile; break;
    case 'q' : arg >> opt::quantfile; break;
    case 'm' : arg >> opt::markerfile; break;      
    case 'c' : arg >> opt::threads; break;      
    case 'h' : help = true; break;
    case 'C' : 
      {
      std::string token;
      while (std::getline(arg, token, ',')) 
	opt::channels.push_back(std::stoi(token));
      }
      break;  
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
  */
  
  if (! (opt::module == "gray2rgb" || opt::module == "mean" || opt::module == "compress" || opt::module == "debug" || opt::module == "colorize") ) {
    std::cerr << "Module " << opt::module << " not implemented" << std::endl;
    die = true;
  }

  //if (opt::module == "gray2rgb" && argc != 4)
  //  die = true;

  if (die) {
      std::cerr << "\n" << RUN_USAGE_MESSAGE;
      if (die)
	exit(EXIT_FAILURE);
      else 
	exit(EXIT_SUCCESS);	
    }
}

// return TRUE if you want the process to die and print message
static bool in_only_process(int argc, char** argv) {
  
  optind++;
  // Process any remaining no-flag options
  size_t count = 0;
  while (optind < argc) {
    if (opt::infile.empty()) {
      opt::infile = argv[optind];
    }
    count++;
    optind++;
  }

  // there should be only 1 non-flag input
  if (count > 1)
    return true;
  
  // die if no inputs provided
  if (count == 0)
    return true;
  
  if (!check_readable(opt::infile) && opt::infile != "-") {
    std::cerr << "Error: File " << opt::infile << " not readable/exists" << std::endl;
    return true;
  }
  
  return opt::infile.empty();

}


// return TRUE if you want the process to die and print message
static bool in_out_process(int argc, char** argv) {
  
  optind++;
  // Process any remaining no-flag options
  size_t count = 0;
  while (optind < argc) {
    if (opt::infile.empty()) {
      opt::infile = argv[optind];
    } else if (opt::outfile.empty()) {
      opt::outfile = argv[optind];
    }
    count++;
    optind++;
  }

  // there should be only 2 non-flag input
  if (count > 2)
    return true;
  // die if no inputs provided
  if (count == 0)
    return true;
  
  if (!check_readable(opt::infile) && opt::infile != "-") {
    std::cerr << "Error: File " << opt::infile << " not readable/exists" << std::endl;
    return true;
  }

  
  return opt::infile.empty() || opt::outfile.empty();

}


static bool check_readable(const std::string& filename) {

  std::ifstream file(filename);

  bool answer = file.is_open();
  
  // Close the file if it was opened
  if (file) {
    file.close();
  }

  return answer;
}
