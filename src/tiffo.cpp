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

  static std::string palette;
  static std::vector<int> channels;
  
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
"  gray2rgb - Convert a 3-channel gray TIFF to a single RGB\n"
"  colorize - Colorize select channels from a cycif tiff\n"
"  mean - Give the mean pixel for each channel\n"
"  csv - <placeholder for csv processing>\n"
  "\n";
  
static int gray2rgb();
static int findmean();
static int colorize();
static int debugfunc();
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

int main(int argc, char **argv) {
  
  // Check if a command line argument was provided
  if (argc < 2) {
    std::cerr << "Error: missing command line argument" << std::endl;
    std::cerr << RUN_USAGE_MESSAGE;
    return 1;
  }

  parseRunOptions(argc, argv);
  
  // get the module
  if (opt::module == "gray2rgb") {
    return(gray2rgb());
  } else if (opt::module == "colorize") {
    return(colorize());
  } else if (opt::module == "mean") {
    return(findmean());
  } else if (opt::module == "debug") {
    return(debugfunc());
  } else {
    assert(false);
  }
  return 1;
}

int findmean() {

  // open a tiff
  // the "m" keeps it from being memory mapped, which for
  // a single pass just causes memory overruns from the C memmap func
  TiffReader reader(opt::infile.c_str());

  // this routine will handle printing output to stdout
  reader.print_means();

  return 0;
}

static int gray2rgb() {

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
  
  TIFFSetField(otif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(otif, TIFFTAG_SAMPLESPERPIXEL, 3);

  // if this is a single 3 IFD file
  MergeGrayToRGB(r_itif, otif);
  
  TIFFClose(r_itif);
  TIFFClose(otif);
  
  return 0;
}


static int colorize() {
  
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
  Colorize(r_itif, otif, opt::palette, opt::channels);
  
  TIFFClose(r_itif);
  TIFFClose(otif);
  
  return 0;
}



// parse the command line options
static void parseRunOptions(int argc, char** argv) {
  bool die = false;

  if (argc <= 1) 
    die = true;

  bool help = false;
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

  if (! (opt::module == "gray2rgb" || opt::module == "mean" || opt::module == "csv" || opt::module == "debug" || opt::module == "circles" || opt::module == "knn" || opt::module == "colorize") ) {
    std::cerr << "Module " << opt::module << " not implemented" << std::endl;
    die = true;
  }

  if (opt::module == "gray2rgb" && argc != 4)
    die = true;

  if (opt::module == "circles" && opt::quantfile.empty())
    die = true;
  
  if (die || help) 
    {
      std::cerr << "\n" << RUN_USAGE_MESSAGE;
      if (die)
	exit(EXIT_FAILURE);
      else 
	exit(EXIT_SUCCESS);	
    }
}

static int circles() {

  /*
  // make the header
  CellHeader header(opt::quantfile);

  // this the main quant table
  std::cerr << "...reading table" << std::endl;
  CellTable table(opt::quantfile, &header,0);
  
  // open the input tiff
  //TiffReader reader(opt::infile.c_str());
  
  // create a raw tiff to draw circles onto
  TiffWriter writer(opt::outfile.c_str());
  //writer.CopyFromReader(reader);

  writer.SetTag(TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  writer.SetTag(TIFFTAG_SAMPLESPERPIXEL, 1);
  writer.SetTag(TIFFTAG_BITSPERSAMPLE, 8);  
  
  // scale the cell table
  uint32_t w,h;
  table.GetDims(w,h);
  float scale_factor = 1000.0f / w;
  table.scale(scale_factor);
  
  // get the bounds of the new table
  table.GetDims(w,h);

  // mode is 8
  TiffImage circles(w, h, 8);
  circles.setthreads(opt::threads);
  circles.setverbose(opt::verbose);

  // 
  std::cerr << "...making circles" << std::endl;
  circles.DrawCircles(table, 2);

  writer.UpdateDims(circles);
  writer.SetTile(0,0);
  TIFFSetupStrips(writer.get());
  std::cerr << " writing " << std::endl;
  writer.Write(circles);
  */
  return 0;

}

static int debugfunc() {
  /*
  TiffReader reader(opt::infile.c_str());

  reader.print();

  uint64_t* offsets = NULL;
  TIFFGetField(reader.get(), TIFFTAG_STRIPOFFSETS, &offsets);
  
  //std::cout << "...working on mean" << std::endl;
  //reader.print_means();

  TiffImage image(reader);
  std::cerr << "...reading in" << std::endl;
  image.ReadToRaster();

  // shrink it
  float scale_factor = 1000.0f / image.width();
  image.Scale(scale_factor, true, opt::threads);
  
  TiffWriter writer(opt::outfile.c_str());
  writer.CopyFromReader(reader);
  writer.MatchTagsToRaster(image);

  std::cerr << "...writing" << std::endl;
  writer.Write(image);
  
  //myplot();
  return 1;
*/  
  /* std::cerr << " opening file " << opt::infile << std::endl;
  TIFF* in = TIFFOpen(opt::infile.c_str(), "rm");

  std::cerr << " making output file " << opt::outfile << std::endl;
  TIFF* out = TIFFOpen(opt::outfile.c_str(), "w8");

  std::cerr << " making TiffImage and reading to raster" << std::endl;
  TiffImage tin(in);
  tin.setverbose(true);
  tin.ReadToRaster();

  std::cerr << " mean of input " << tin.mean(in) << std::endl;

  std::cerr << " copying tags to output" << std::endl;
  tiffcp(in, out, false);
  
  std::cerr << " writing raster" << std::endl;
  tin.write(out);

  std::cerr << "closing" << std::endl;
  TIFFClose(in);
  TIFFClose(out);

  */
  return 0;
    
  
  
}


int knn() {
  /*
  // make the header
  CellHeader header(opt::quantfile);

  // this the main quant table
  std::cerr << "...reading table" << std::endl;
  CellTable table(opt::quantfile, &header,10000);

  std::vector<double> data = table.ColumnMajor();  
  */

  return 0;
}
