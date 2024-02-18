#include "cell2.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "tiff_utils.h"

#define MAX_LINE_SIZE 2048

std::vector<double> CellTable::XY() const {

  std::vector<double> out;
  out.reserve(2*num_cells());
  assert(!x.empty());
  assert(!y.empty());
  assert(table.at(x).size());
  assert(table.at(y).size());  
  out.insert(out.end(), table.at(x).begin(), table.at(x).end());
  out.insert(out.end(), table.at(y).begin(), table.at(y).end());  
  
  return out;
  
}

CellTable::CellTable(const char* file, const char* markers_file) {

  // read the markers first
  std::ifstream       mfile(markers_file);
  std::string         line;

  std::getline(mfile, line); 
  std::istringstream stream(line);
  std::string item;
  while (std::getline(stream, item)) {
    markers.insert(item);
  }
  
  // read in the header
  std::ifstream       rfile(file);  
  std::getline(rfile, line); 
  std::istringstream header_stream(line);
  std::string header_item;
  while (std::getline(header_stream, header_item, ',')) {
    table[header_item] = std::vector<float>();
  }

  // read in the JSON file
  std::string mm = std::string(markers_file);
  JsonReader json_reader(mm);
  json_reader.ReadData();

  //debug
  std::cerr << " JSON X " << json_reader.GetX() << " JSON Y " << json_reader.GetY() << std::endl;
  return;
  
  
  x = json_reader.GetX();
  y = json_reader.GetY();
  assert(!x.empty());
  assert(!y.empty());  
  
  size_t count = 0;

  // store the line
  char str[1024];

  std::vector<std::vector<float>> vec(table.size());
  
  // read the remaining lines
  while (std::getline(rfile, line)) {
    
    strcpy(str, line.c_str());
    char *value_str = strtok(str, ",");
    std::vector<float> values(table.size());
    str[0] = '\0';
 
    size_t n = 0;
    while (value_str) {
      //table[std::next(table.begin(), n++)->first].push_back(atoi(value_str));
      vec[n++].push_back(atoi(value_str));
      value_str = strtok(nullptr, ",");
    }

    if (count % 100000 == 0) {
      std::cerr << "line " << AddCommas(count) << std::endl;
      }
    count++;
    
  }

  // move the values to the unordered_map
  size_t n = 0;
  for (auto& k : table) {
    k.second = vec[n++];
  }
  
}
