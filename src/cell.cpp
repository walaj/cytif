#include "cell.h"

#include <sstream>

CellHeader::CellHeader(const std::string& input) {

  std::stringstream ss(input);
  std::string f;
  size_t i = 0;

  while (std::getline(ss, f,',')) {
    header_map[f] = i;
    i++;
  }
  
  // x centroid and y centroid positions
  x = header_map.at("X_centroid");
  y = header_map.at("Y_centroid");      
  
}

// constructor from a string
Cell::Cell(const std::string& input, const CellHeader& h) {
  
  std::stringstream ss(input);
  float f;
  size_t i = 0;
  while (ss >> f) {

    if (i == 0)
      cell_id = f;
    else if (i == h.x)
      x = f;
    else if (i == h.y)
      y = f;
    else
      m_elems.push_back(f);
    
    if (ss.peek() == ',')
      ss.ignore();

    ++i;
  }
  
}

