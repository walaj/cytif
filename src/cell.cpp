#include "cell.h"
#include "tiff_utils.h"

#include <sstream>
#include <cassert>
#include <fstream>

CellHeader::CellHeader(const std::string& input) {

  std::ifstream  file(input);
  std::string    line;

  // parse the header from the quant table
  if (!std::getline(file, line)) {
    std::cerr << "Error reading CSV header from " << input << std::endl;
    assert(false);
  }

  file.close();

  // search for which pattern we have
  std::string xstring, ystring;
  std::string cleans = clean_string(line);
  if (cleans.find("X_centroid") != std::string::npos) {
    xstring = "X_centroid";
    ystring = "Y_centroid";    
  } else if (cleans.find("Xt") != std::string::npos) {
    xstring = "Xt";
    ystring = "Yt";      
  } else {
    std::cerr << "Error: Could not find X coordinates in quant file" << std::endl;
    assert(false);
  }
  
  // read this line
  std::stringstream ss(cleans);
  std::string f;
  size_t i = 0;
  size_t marker_index = 0;
  
  while (std::getline(ss, f,',')) {
    header_map[f] = i;
    i++;
  }

  //
  //for (auto& c : header_map)
  //  std::cerr << c.first << " " << c.second << std::endl;
  
  // x centroid and y centroid positions
  x = header_map.at(xstring);
  y = header_map.at(ystring);      
  area = header_map.at("Area");
  ori = header_map.at("Orientation");
  min = header_map.at("MinorAxisLength");
  maj = header_map.at("MajorAxisLength");
  ecc = header_map.at("Eccentricity");
  sol = header_map.at("Solidity");
  ext = header_map.at("Extent");

  // must have spatial information
  assert(header_map.find(xstring) != header_map.end());
  assert(header_map.find(ystring) != header_map.end());  
  
  
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
  size_t num_rows = this->size(); // number of markers
  size_t num_cols = m_intensity.size(); // number of cells
  
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

CellTable CellTable::Head(size_t n) {

  CellTable ct;

  n = std::max(n, this->cells.size());
  
  ct.m_header = this->m_header;
  ct.m_intensity = this->m_intensity;
  ct.cells = std::vector<Cell>(this->cells.begin(), this->cells.begin() + n);

  ct.__parse_markers();

  return ct;
  
}

CellTable::CellTable(const std::string& input, const CellHeader* h, int limit) {
  m_header = h;

  std::ifstream       file(input);
  std::string         line;

  // loop the table and build the CellTable
  size_t count = 0;
  size_t cellid = 0;
  while (std::getline(file, line)) {

    // skip first line
    if (count == 0) {
      count++;
      continue;
    }
    count++;
    
    ++cellid;

    if (limit > 0 && cellid > limit)
      break;

    addCell(Cell(line, h));

    bool verbose= true;
    if (cellid % 100000 == 0 && verbose)
      std::cerr << "...reading cell " << AddCommas(static_cast<uint32_t>(cellid)) << std::endl;
  }

  file.close();

  // create the matrix
  
}

void CellTable::__create_matrix() {

  m_xy = Eigen::MatrixXd(this->size(), 2);

  std::set<std::string> meta = this->m_header->meta();
  
  // fill the matricies
  for (size_t i = 0; i < this->size(); i++) {
    m_xy(i, 0) = cells[i].x;
    m_xy(i, 1) = cells[i].y;

    
  }

  //   
}




void CellTable::scale(double factor) {

  for (auto& c : cells) {
    c.x *= factor;
    c.y *= factor;
  }
  
}

void CellTable::GetDims(uint32_t &width, uint32_t &height) {

  width = 0;
  height = 0;
  
  for (auto& c : cells) {
    width = std::max(width, static_cast<uint32_t>(std::ceil(c.x)));
    height = std::max(height, static_cast<uint32_t>(std::ceil(c.y)));
  }
    
}

/*Cell Cell::subset(const std::set<std::string>& cols) const {

  Cell out(m_header);
  out.m_elems = std::vector<float>(out.size());
  
  for (const auto& c : *m_header) {
    if (cols.find(c.first) != cols.end()) {
      out[c.second] = 
    }
  }
  
}

CellTable CellTable::subset(const std::vector<std::string>& cols) const { 

  CellTable out(m_header);

  for (const auto& c: cells) {
  }
  
}
*/
