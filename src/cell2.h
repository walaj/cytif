#ifndef CELL_HEADER2_H
#define CELL_HEADER2_H

#include <string>
#include <unordered_map>
#include <set>
#include <vector>

typedef std::set<std::string> Markers;
typedef std::set<std::string> Metas;

//static std::vector<std::string> meta_list = {"Solidity","Extent","Area","Eccentricity",
//					     "MinorAxisLength","MajorAxisLength","Orientation"};

class CellTable {
    
public:

  CellTable() {}

  CellTable(const char* file, const char* markers_file);
  
  std::unordered_map<std::string, std::vector<float>> table;

  size_t size() const { return table.size(); } 

  using const_iterator = std::unordered_map<std::string, std::vector<float>>::const_iterator;
  const_iterator begin() const { return table.begin(); }
  const_iterator end() const { return table.end(); }

  Metas meta;
  Markers markers;
  std::string x;
  std::string y;
  
};

#endif
