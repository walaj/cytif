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

#include "cell.h"

/*
#define myascii(c) (((c) & ~0x7f) == 0)

// I wrapped it in a library because it spams too many warnings
extern int wrap_stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);

#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"

#define JC_VORONOI_CLIP_IMPLEMENTATION
#include "jc_voronoi_clip.h"
*/

namespace opt {
  static bool verbose = false;
  static std::string infile;
  static std::string quantfile;
  static std::string outfile;
  static std::string module;

  static std::string redfile;
  static std::string greenfile;
  static std::string bluefile;
}

#define DEBUG(x) std::cerr << #x << " = " << (x) << std::endl

#define TVERB(msg) \
  if (opt::verbose) {		   \
    std::cerr << msg << std::endl; \
  }

#define JC_VORONOI_IMPLEMENTATION

static const char* shortopts = "hvr:g:b:q:";
static const struct option longopts[] = {
  { "verbose",                    no_argument, NULL, 'v' },
  { "red",                        required_argument, NULL, 'r' },
  { "green",                      required_argument, NULL, 'g' },
  { "blue",                       required_argument, NULL, 'b' },
  { "quant-file",                 required_argument, NULL, 'q' },      
  { NULL, 0, NULL, 0 }
};

static const char *RUN_USAGE_MESSAGE =
"Usage: tiffo [module] <options> infile outfile\n"
"Modules:\n"
"  gray2rgb - Convert a 3-channel gray TIFF to a single RGB\n"
"  mean - Give the mean pixel for each channel\n"
"  csv - <placeholder for csv processing>\n"
  "\n";
  
static int gray2rgb();
static int findmean();
static int csvproc();
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
  else if (opt::module == "csv") {
    return(csvproc());
  } else if (opt::module == "debug") {
    return(debugfunc());
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

static int csvproc() {

  std::ifstream       file(opt::quantfile);
  std::string         line;

  CellTable table;
  
  // parse the header from the quant table
  CellHeader header;
  if (std::getline(file, line)) {
    header = CellHeader(line);
  } else {
    std::cerr << "Error reading CSV header from " << opt::infile;
    return 1;
  }
  
  size_t cellid = 0;
  
  // loop the table and build the CellTable
  while (std::getline(file, line)) {

    ++cellid;
    Cell cel(line, header);
    table.addCell(cel);

    if (cellid % 100000 == 0 && opt::verbose)
      std::cerr << "...reading cell " << AddCommas(static_cast<uint32_t>(cellid)) << std::endl;
  }

  // open the input tiff
  TIFF *itif = TIFFOpen(opt::infile.c_str(), "rm");
  
  // create a raw tiff to draw circles onto
  TIFF* otif = TIFFOpen(opt::outfile.c_str(), "w8");
  if (otif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", opt::outfile);
    return 1;
  }

  // copy all of the tags from in to out
  tiffcp(itif, otif, false);
  
  TIFFSetField(otif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(otif, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(otif, TIFFTAG_BITSPERSAMPLE, 8);  

  TiffImage circles(otif);

  circles.setverbose(opt::verbose);

  circles.DrawCircles(otif, table);

  std::cerr << " writing " << std::endl;
  circles.write(otif);
  TIFFClose(otif);
  
  /*
  // VORONOI
  int NPOINT = 10;
  int i;
  jcv_rect bounding_box = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
  jcv_diagram diagram;
  jcv_point points[NPOINT];
  const jcv_site* sites;
  jcv_graphedge* graph_edge;
  
  memset(&diagram, 0, sizeof(jcv_diagram));

  srand(0);
  for (i=0; i<NPOINT; i++) {
    points[i].x = ((float)rand()/(1.0f + (float)RAND_MAX));
    points[i].y = ((float)rand()/(1.0f + (float)RAND_MAX));
  }

  printf("# Seed sites\n");
  for (i=0; i<NPOINT; i++) {
    printf("%f %f\n", (double)points[i].x, (double)points[i].y);
  }

  jcv_diagram_generate(NPOINT, (const jcv_point *)points, &bounding_box, (const jcv_clipper*)0, &diagram);

  printf("# Edges\n");
  sites = jcv_diagram_get_sites(&diagram);
  for (i=0; i<diagram.numsites; i++) {

    graph_edge = sites[i].edges;
    while (graph_edge) {
      // This approach will potentially print shared edges twice
      printf("%f %f\n", (double)graph_edge->pos[0].x, (double)graph_edge->pos[0].y);
      printf("%f %f\n", (double)graph_edge->pos[1].x, (double)graph_edge->pos[1].y);
      graph_edge = graph_edge->next;
    }
  }

  jcv_diagram_free(&diagram);
  */
  
  return 0;
}

static int gray2rgb() {
  
  // if we have three separate RGB file
  if (!opt::redfile.empty()) {
    if (opt::greenfile.empty() || opt::bluefile.empty()) {
      std::cerr << "Error: Must specify red, green and blue files" << std::endl;
      return 1;
    }

   // the way its parsed, the first non-labeled file is in.
    // so we just change it to the outfile since the infiles are
    // set by the -r, -g, -b flags
    opt::outfile = opt::infile; 
    
  } else {
    opt::redfile = opt::infile; // in this case, red file is also red,green,blue
  }

  // messing around with the tiff header
  //TiffHeader thead(opt::infile);
  //thead.view_stdout();

  // open either the red channel or the 3-IFD file
  TIFF *r_itif = TIFFOpen(opt::redfile.c_str(), "rm");
  TiffImage rimage(r_itif);  
  rimage.setverbose(opt::verbose);

  // Open the output TIFF file
  TIFF* otif = TIFFOpen(opt::outfile.c_str(), "w8");
  if (otif == NULL) {
    fprintf(stderr, "Error opening %s for writing\n", opt::outfile);
    return 1;
  }
  
  // copy all of the tags from in to out
  tiffcp(r_itif, otif, false);
  
  // setup the RGB output file
  TiffImage rgb(r_itif);
  rgb.setverbose(opt::verbose);
  
  TIFFSetField(otif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(otif, TIFFTAG_SAMPLESPERPIXEL, 3);

  // if this is a single 3 IFD file
  if (opt::greenfile.empty()) {

    // new way
    rgb.MergeGrayToRGB(r_itif, otif);
    TIFFClose(r_itif);
  }

  // we have three separate files
  else {
    
    // read in the TIFF as a green channel
    TIFF *g_itif = TIFFOpen(opt::greenfile.c_str(), "rm");
    TiffImage gimage;
    gimage.setverbose(opt::verbose);
    gimage = TiffImage(g_itif);
    
    // read in the TIFF as a blue channel
    TIFF *b_itif = TIFFOpen(opt::bluefile.c_str(), "rm");
    TiffImage bimage;
    bimage = TiffImage(b_itif);
    bimage.setverbose(opt::verbose);
    
    rgb.MergeGrayToRGB(r_itif, g_itif, b_itif, otif);
  }
  
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
    case 'r' : arg >> opt::redfile; break;
    case 'g' : arg >> opt::greenfile; break;
    case 'b' : arg >> opt::bluefile; break;
    case 'q' : arg >> opt::quantfile; break;            
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
  
  if (! (opt::module == "gray2rgb" || opt::module == "mean" || opt::module == "csv" || opt::module == "debug") ) {
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

static int debugfunc() {


  std::cerr << " opening file " << opt::infile << std::endl;
  TIFF* in = TIFFOpen(opt::infile.c_str(), "rm");

  std::cerr << " making output file " << opt::outfile << std::endl;
  TIFF* out = TIFFOpen(opt::outfile.c_str(), "w8");

  std::cerr << " making TiffImage and reading to raster" << std::endl;
  TiffImage tin(in);
  tin.setverbose(true);
  tin.ReadToRaster(in);

  std::cerr << " mean of input " << tin.mean(in) << std::endl;

  std::cerr << " copying tags to output" << std::endl;
  tiffcp(in, out, false);
  
  std::cerr << " writing raster" << std::endl;
  tin.write(out);

  std::cerr << "closing" << std::endl;
  TIFFClose(in);
  TIFFClose(out);

  return 0;
    
  
  
}
