#ifndef TIFFO_CELL_H
#define TIFFO_CELL_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <set>
#include <Eigen/Core>

// Create a set of strings
static std::set<std::string> non_markers =
  {"Orientation", "CellID", "X_centroid", "Y_centroid", "Area", "MajorAxisLength",
   "MinorAxisLength","Eccentricity","Solidity","Extent"};

class CellHeader {
  
 public:

  friend class CellTable;
  friend class Cell;  
  
  CellHeader(const std::string& input);
  
  // null constructor
  CellHeader() {}

  // constructor from a file name. This will open and close the file
  CellHeader(const char* filename);
  
  int getIndex(const std::string& token) const { return header_map.at(token); }

  friend std::ostream& operator<<(std::ostream& out, const CellHeader& obj) {
    for (const auto& [key, value] : obj.header_map) {
      out << key << ":" << value << " ";
    }
    return out;
  }

  int KeyValue(const std::string& key) const {
    return (header_map.at(key));
  }

  // these the indicies in the header NOT actual values
  // e.g. in the header: CellID, Hoechst, AF1, X_centroid, ...
  // x would be 3
  size_t x = -1; // x_centroid
  size_t y = -1; // y_centroid
  size_t area = -1; // area
  size_t ori = -1; // orientation
  size_t maj = -1; // major axis length
  size_t min = -1; // minor axis length
  size_t ecc = -1; // eccentricity
  size_t sol = -1; // solidity
  size_t ext = -1; // extent

  std::set<std::string> Keys() const;

  using const_iterator = std::map<std::string, int>::const_iterator;
  const_iterator begin() const { return header_map.begin(); }
  const_iterator end() const { return header_map.end(); }
  
 private:
  std::map<std::string, int> header_map;

  std::set<std::string> m_markers;

  std::set<std::string> m_meta;
  
};


class Cell {

 public:

  // constructor from a string
  Cell(const std::string& input, const CellHeader* h);
  
  Cell(const CellHeader* h) : m_header(h) {}

  int getCellId() const { return cell_id; }
  int getX() const { return x; }
  int getY() const { return y; }
  float getVal(const std::string& key) const {
    return (m_elems.at(m_header->KeyValue(key)));
  }
  size_t size() const { return m_elems.size(); } 
  
  /*  friend std::ostream& operator<<(std::ostream& out, const Cell& obj) {
    out << "cell_id: " << obj.cell_id << ", x: " << obj.x << ", y: " << obj.y;
    return out;
    } */ 

  friend std::ostream& operator<<(std::ostream& out, const Cell& obj) {
    for (size_t i = 0; i < obj.m_elems.size(); i++)
      out << obj.cell_id << ": (" << obj.x << "," << obj.y <<
	") - " << obj.m_elems.at(i) << ",";
    return out;

  }  

  using const_iterator = std::vector<float>::const_iterator;
  const_iterator begin() const { return m_elems.begin(); }
  const_iterator end() const { return m_elems.end(); }
  
  int cell_id = 0;
  float x = 0;
  float y = 0;

  Cell subset(const std::set<std::string>& cols) const;
  
private:
  const CellHeader* m_header;

  std::vector<float> m_elems;
  
};

class CellTable {
    
public:

  CellTable() {}
  
  CellTable(const CellHeader* h) : m_header(h) {}

  CellTable(const std::string& input, const CellHeader* h, int limit = 0);

  CellTable subset(const std::vector<std::string>& cols) const;
  
  void addCell(const Cell& cell) { cells.push_back(cell); }
  
  std::vector<Cell> getCells() const { return cells; }

  void scale(double factor);
  
  using const_iterator = std::vector<Cell>::const_iterator;
  const_iterator begin() const { return cells.begin(); }
  const_iterator end() const { return cells.end(); }

  // total number of cells
  size_t size() const { return cells.size(); }

  // return the marker intensities as a single column-major vector
  std::vector<double> ColumnMajor();

  size_t NumMarkers();

  // return a copy of this cell table with first n elements
  CellTable Head(size_t n);

  void GetDims(uint32_t &width, uint32_t &height);
  
private:
  const CellHeader* m_header;

  Eigen::MatrixXd m_xy;
  Eigen::MatrixXd m_markers;
  Eigen::MatrixXd m_meta;  
  
  // store each of the cells
  std::vector<Cell> cells;  

  // same data but stored more logically as columns
  std::map<std::string, std::vector<float>> m_intensity;

  // setup vectors containing all of the values for each marker
  void __parse_markers();

  void __create_matrix();
};


#endif
