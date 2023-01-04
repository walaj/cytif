#ifndef TIFFO_CELL_H
#define TIFFO_CELL_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

class CellHeader {
  
 public:
  
  CellHeader(const std::string& input);
  
  // null constructor
  CellHeader() {}
  
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
  
  uint64_t x = static_cast<uint64_t>(-1);
  uint64_t y = static_cast<uint64_t>(-1);

 private:
  std::map<std::string, int> header_map;
  
};


class Cell {

 public:

  // constructor from a string
  Cell(const std::string& input, const CellHeader& h); 

  int getCellId() const { return cell_id; }
  int getX() const { return x; }
  int getY() const { return y; }
  float getVal(const std::string& key) const {
    return (m_elems.at(m_header.KeyValue(key)));
  }
  
  /*  friend std::ostream& operator<<(std::ostream& out, const Cell& obj) {
    out << "cell_id: " << obj.cell_id << ", x: " << obj.x << ", y: " << obj.y;
    return out;
    } */ 

  friend std::ostream& operator<<(std::ostream& out, const Cell& obj) {
    for (size_t i = 0; i < obj.m_elems.size(); i++)
      out << obj.m_elems.at(i) << ",";
    return out;

  }  

  int cell_id = 0;
  float x = 0;
  float y = 0;
  
private:
  CellHeader m_header;

  std::vector<float> m_elems;
  
};

class CellTable {
    
public:
  void addCell(const Cell& cell) { cells.push_back(cell); }
  
  std::vector<Cell> getCells() const { return cells; }
  
  using const_iterator = std::vector<Cell>::const_iterator;
  const_iterator begin() const { return cells.begin(); }
  const_iterator end() const { return cells.end(); }

  size_t size() const { return cells.size(); }
  
private:
  std::vector<Cell> cells;

  
  
};

#endif
