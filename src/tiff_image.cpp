#include "tiff_image.h"
#include <cassert>
#include <iostream>

#include <string.h>
#define DP(x) fprintf(stderr,"DEBUG %d\n", x)
#define DEBUG(var) printf(#var " = %d\n", var)

/*TiffImage::TiffImage(TIFF* tif) {

  // clear the raster data
  // note that TiffImage class is NOT responsible for managing TIFF memory
  clear_raster();

  // add the tiff
  __give_tiff(tif);
  
  }*/

TiffImage::TiffImage(const TiffReader& tr) {

  // make a local copy of this tiff reader
  m_tr = tr;

  m_ifd = tr.CurrentIFD();
  
  // Initial image properties come from the file
  m_height = m_ifd.height;
  m_width = m_ifd.width;
  m_photometric = m_ifd.photometric;
  m_planar = m_ifd.planar;
  m_samples_per_pixel = m_ifd.samples_per_pixel;
  m_bits_per_sample = m_ifd.bits_per_sample;

  m_pixels = static_cast<uint64_t>(m_width) * m_height;
  
}

//template <typename T>
//T TiffImage::pixel(uint64_t x, uint64_t y, int p) const {
uint8_t TiffImage::pixel(uint64_t x, uint64_t y, int p) const {

  assert(p == PIXEL_GRAY || p == PIXEL_RED || p == PIXEL_GREEN || p == PIXEL_BLUE || p == PIXEL_ALPHA);
  
  // right now only two types allowed
  //static_assert(std::is_same<T, uint32_t>::value || std::is_same<T, uint8_t>::value,
  //              "T must be either uint32_t or uint8_t");

  // check that the data has been read
  assert(__is_rasterized());
  
  // check that the pixel is in bounds
  uint64_t dpos = y * m_width + x; 
  if (dpos > m_pixels || x > m_width || y > m_height) {
    fprintf(stderr, "ERROR: Accesing out of bound pixel at (%ul,%ul)\n",x,y);
    assert(false);
  }
  
  if (p == PIXEL_GRAY) 
    return static_cast<uint8_t*>(m_data)[dpos];

  // else its rgb and so add the pixel offset
  return static_cast<uint8_t*>(m_data)[dpos*3 + p];
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

double TiffImage::mean() const {

  if (!__is_rasterized()) {
    std::cerr << "ERROR: needs to be read into raster first" << std::endl;
    return -1;
  }

  size_t mode = GetMode();
  
  double sum = 0;
  
  switch (mode) {
  case 8:
    for (size_t i = 0; i < m_pixels; ++i) {
      sum += static_cast<uint8_t*>(m_data)[i];
    }
    break;
  case 32:
    for (size_t i = 0; i < m_pixels; ++i)
      sum += static_cast<uint32_t*>(m_data)[i];
    break;
  }

  return (sum / m_pixels);

}

int TiffImage::Scale(double scale, bool ismode) {

  if (scale > 1) {
    fprintf(stderr, "Scale must be < 1\n");
    return 1;
  }

  //debug
  //  for (int x = 0; x < 100; x++)
  //  std::cerr << (unsigned int)static_cast<uint8_t*>(m_data)[x] << std::endl;
  
  size_t mode = GetMode();
  
  // New image as a row-major vector
  void* new_image;

  // New image dimensions
  uint64_t new_width = std::ceil(scale * m_width);
  uint64_t new_height = std::ceil(scale * m_height);
  uint64_t w_chunk = std::ceil(1/scale);
  uint64_t h_chunk = std::ceil(1/scale);
  
  switch (mode) {
  case 8:
    new_image = calloc(new_width * new_height, sizeof(uint8_t));
    break;
  case 3:
    new_image = calloc(new_width * new_height, sizeof(uint8_t) * 3);
    break;
  default:
    fprintf(stderr, "not able to understand mode %d\n", mode);
    assert(false);
  }

  //  std::cerr << "new dim " << new_width << " x " << new_height <<
  //  " chunk " << w_chunk << " x " << h_chunk << std::endl;
  
  // Iterate over the new image
  for (uint64_t i = 0; i < new_height; i++) {
    for (uint64_t j = 0; j < new_width; j++) {
      
      // Average the pixels in the corresponding region of the original image
      std::vector<std::vector<int>> sum;
      size_t count = 0;

      // loop and do the averaging
      uint64_t ii_s = std::round(static_cast<double>(i) / scale);
      uint64_t jj_s = std::round(static_cast<double>(j) / scale);
      for (uint64_t ii = ii_s; ii < ii_s + h_chunk; ii++) {
	for (uint64_t jj = jj_s; jj < jj_s + w_chunk; jj++) {
	  ++count;
	  if (jj < m_width && ii < m_height) {
	    switch(mode) {
	    case 8:
	      sum[3].push_back(static_cast<uint8_t*>(m_data)[ii * m_width + jj]);
	      break;
	    case 3:
	      //	      std::cerr << " sum " << sum[0] << " m_data[" << (ii * m_width + jj)*3 << "] = " << 
		//	(unsigned int)static_cast<uint8_t*>(m_data)[(ii * m_width + jj)*3] << std::endl;
	      sum[0].push_back(static_cast<uint8_t*>(m_data)[(ii * m_width + jj)*3    ]);
	      sum[1].push_back(static_cast<uint8_t*>(m_data)[(ii * m_width + jj)*3 + 1]);
	      sum[2].push_back(static_cast<uint8_t*>(m_data)[(ii * m_width + jj)*3 + 2]);
	      break;
	    }
	  }
	}
      }

      std::vector<double> sump = {0,0,0,0};
      for (size_t x = 0; x < 4; ++i) {
	if (ismode) {
	  sump[x] = getMode(sum[x]);
	} else {
	  sump[x] = getMean(sum[x]);	  
	}
      }
      
      // allocate the smaller image
      switch(mode) {
      case 8:
	static_cast<uint8_t*>(new_image)[i * new_width + j] = sump[3];
	break;
      case 3:
	static_cast<uint8_t*>(new_image)[(i * new_width + j)*3    ] = sump[0];
	static_cast<uint8_t*>(new_image)[(i * new_width + j)*3 + 1] = sump[1];
	static_cast<uint8_t*>(new_image)[(i * new_width + j)*3 + 2] = sump[2];
	break;
      }
    }
  }

  // transfer the data
  free(m_data);
  m_data = new_image;

  m_width = new_width;
  m_height = new_height;
  m_pixels = static_cast<uint64_t>(m_width) * m_height;
  
  // set the new width and height
  /*if (!TIFFSetField(m_outtif, TIFFTAG_IMAGEWIDTH, new_width)) {
    fprintf(stderr, "ERROR: image (shrunk) failed to set width\n");
    m_width = 0;
    return 1;
  }
  
  if (!TIFFSetField(m_outtif, TIFFTAG_IMAGELENGTH, new_height)) {
    fprintf(stderr, "ERROR: image (shrunk) failed to set height\n");
    m_height = 0;
    return 1;
    }*/
  
  return 0;
  
}

int TiffImage::__alloc() {

  assert(m_pixels);
  
  size_t mode = GetMode(); 

  if (m_data) {
    std::cerr << "m_data not empty, freeing. Are you sure you want this?" << std::endl;
    free(m_data);
  }
  
  switch (mode) {
      case 8:
	m_data = calloc(m_pixels, sizeof(uint8_t));
	//fprintf(stderr, "alloc size %f GB\n", m_pixels / 1e9);
	break;
      case 32:
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

int TiffImage::ReadToRaster() {

  m_data = m_ifd.ReadRaster();
  
  if (m_data == NULL)
    return 1;
  
  return 0;
}

void TiffImage::clear_raster() {

  if (m_data == NULL) {
    //fprintf(stderr, "Warning: no raster to clear\n");
    return;
  }

  _TIFFfree(m_data);

  m_data = NULL;

}

bool TiffImage::__is_rasterized() const {

  if (m_data == NULL) {
    fprintf(stderr, "Warning: Trying to write empty image. Try running ReadToRaster first\n");    
    return false;
  }
  return true;
}


uint8_t TiffImage::GetMode() const {

  // RGB
  if (m_samples_per_pixel == 3) 
    return 3;
  else if (m_bits_per_sample == 8) 
    return 8;
  else if (m_bits_per_sample == 16) 
    return 16;
  else if (m_bits_per_sample == 32)
    return 32;
  else if (m_samples_per_pixel == 4)
    return 4;
  return 0;
  
}

int TiffImage::DrawCircles(const CellTable& table) {
  
  // allocate the raster memory
  __alloc();

  int radius = 10;
  
  std::vector<std::pair<float,float>> circle_shape = get_circle_points(radius);

  uint64_t m_cell = 0;
  for (const auto& c : table) {
    if (m_cell % 100000 == 0 && verbose) {
      std::cerr << "...drawing circle for cell " << AddCommas<uint64_t>(m_cell) <<std::endl;
    }
    m_cell++;
    
    for (const auto& s: circle_shape) {
      uint64_t xpos = std::round(c.x + s.first);
      uint64_t ypos = std::round(c.y + s.second);
      //      std::cerr << " c.x " << c.x << " s.first " << s.first << " c.y " << c.y <<
      //	" s.second " << s.second << " xpos " << xpos << " ypos " << ypos << std::endl;
      if (ypos < m_height && xpos < m_width) {
	static_cast<uint8_t*>(m_data)[ypos * m_width + xpos] = static_cast<uint8_t>(255);
      }
    }
  }
  
  return 0;
}
