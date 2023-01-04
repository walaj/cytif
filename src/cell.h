#ifndef TIFFO_CELL_H
#define TIFFO_CELL_H

#include <string>
#include <map>
#include <vector>
#include <boost/tokenizer.hpp>

class CellHeader {
  
 public:
  CellHeader(const std::string& input)
    {
      
      // Tokenize the input string
      boost::tokenizer<boost::escaped_list_separator<char>> tokens(input);
      
      // Store each token in the map
      int i = 0;
      for (const std::string& token : tokens) {
	header_map[token] = i;
	i++;
      }

      // x centroid and y centroid positions
      m_x = header_map.at("X_centroid");
      m_y = header_map.at("Y_centroid");      
      
    }
  
  // null constructor
  CellHeader() {}
  
  int getIndex(const std::string& token) const { return header_map.at(token); }
  size_t getX() const { return m_x; }
  size_t getY() const { return m_y; }  

  friend std::ostream& operator<<(std::ostream& out, const CellHeader& obj) {
    for (const auto& [key, value] : obj.header_map) {
      out << key << ":" << value << " ";
    }
    return out;
  }
  
  
  
 private:
  std::map<std::string, int> header_map;

  size_t m_x=0;
  size_t m_y=0;  
  
};


class Cell {

 public:

  // constructor froma string
  Cell(const std::string& input) {
    
    // Tokenize the input string
    boost::tokenizer<boost::escaped_list_separator<char>> tokens(input);

    // Parse the cell id, x, and y positions
    int i = 0;
    for (const std::string& token : tokens) {
      switch (i) {
      case 0:
	cell_id = std::stoi(token);
	break;
      case 1:
	x = std::stoi(token);
	break;
      case 2:
	y = std::stoi(token);
	break;
      }
      i++;
    }

  }

  int getCellId() const { return cell_id; }
  int getX() const { return x; }
  int getY() const { return y; }

  friend std::ostream& operator<<(std::ostream& out, const Cell& obj) {
    out << "cell_id: " << obj.cell_id << ", x: " << obj.x << ", y: " << obj.y;
    return out;
  }  
  
private:
  int cell_id;
  int x;
  int y;

  CellHeader m_header;
  
};

class CellTable {
    
public:
  void addCell(const Cell& cell) { cells.push_back(cell); }
  
  std::vector<Cell> getCells() const { return cells; }
  
private:
  std::vector<Cell> cells;
};

#endif
