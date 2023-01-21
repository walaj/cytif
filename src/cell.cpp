#include "cell.h"
#include "tiff_utils.h"

#include <sstream>
#include <cassert>

CellHeader::CellHeader(const std::string& input) {

  std::stringstream ss(clean_string(input));
  std::string f;
  size_t i = 0;
  size_t marker_index = 0;

  while (std::getline(ss, f,',')) {
    /*    std::cerr << " f " << f << " " << f.length() << std::endl;

    switch (f) {
    case: "X_centroid":
      x = i;
      break;
    case: "Y_centroid":
      y = i;
      break;
    case: "Area":
      area = i;
      break;
    case: "Orientation":
      ori = i;
      break;
    case: "MinorAxisLength":
      min = i;
      break;
    case: "MajorAxisLength":
      maj = i;
      break;
    case: "Eccentricty":
      ecc = i;
      break;
    case: "Solidity":
      sol = i;
      break;
    case: "Extent":
      ecc = i;
      break;
      
      
  
    }
    */    
    header_map[f] = i;
    i++;
  }

  /*
  // x centroid and y centroid positions
  x = header_map.at("X_centroid");
  y = header_map.at("Y_centroid");      
  area = header_map.at("Area");
  ori = header_map.at("Orientation");
  min = header_map.at("MinorAxisLength");
  maj = header_map.at("MajorAxisLength");
  ecc = header_map.at("Eccentricity");
  sol = header_map.at("Solidity");
  ext = header_map.at("Extent");

  DEBUGP;
  
  assert(x > 0 && y > 0);
  assert(area > 0 && ori > 0 && min > 0 && maj > 0);
  assert(ecc > 0 && sol > 0 && ext > 0);
  */
}

// constructor from a string
Cell::Cell(const std::string& input, const CellHeader* h) {

  // hold a const pointer to the input header
  m_header = h;
  
  std::stringstream ss(clean_string(input));
  float f;
  size_t i = 0;

  while (ss >> f) {

    if (i == 0)
      cell_id = f;
    else if (i == h->x)
      x = f;
    else if (i == h->y)
      y = f;
    /*    else if (i == h->area || i == h->ori || i == h->maj || i == h->min ||
	     i == h->ecc || i == h->sol || i == h->ext)
      ; // right now we don't do anything with these, so ignore
    */

    // just add everything in the same order as the file
    m_elems.push_back(f);
    
    if (ss.peek() == ',')
      ss.ignore();

    ++i;
  }
  
}

std::set<std::string> CellHeader::Keys() const {
  
  std::set<std::string> keys;
  for (const auto& kv : header_map) {
    keys.insert(kv.first);
  }
  return keys;
  
}

void CellTable::__parse_markers() {

  // all the strings in the hader
  std::set<std::string> list_of_markers = m_header->Keys();

  // remove non-marker strings
  for (const auto& str : non_markers)
    list_of_markers.erase(str);

  // establish the vectors
  for (const auto& m : list_of_markers)
    m_intensity[m] = std::vector<float>();

  // Iterate over the cells
  for (const auto& c : cells) 
    for (const auto& m : list_of_markers) 
      m_intensity[m].push_back(c.getVal(m));

}

std::vector<double> CellTable::ColumnMajor() {

  // if markers haven't been col-ified, do that now
  if (m_intensity.empty())
    __parse_markers();

  // Determine the total size of the resulting array
  size_t num_rows = this->size();
  size_t num_cols = m_intensity.size();
  
  // allocate the column major array
  std::vector<double> result(num_rows * num_cols);
  
  int col = 0;
  for (const auto& kv : m_intensity) {
    const std::vector<float>& values = kv.second;
    for (int row = 0; row < num_rows; ++row)
      result[col*num_rows + row] = values[row];
    ++col;
  }

  return result;
}

size_t CellTable::NumMarkers() {

  // if markers haven't been col-ified, do that now
  if (m_intensity.empty())
    __parse_markers();
  
  return m_intensity.size();
}
