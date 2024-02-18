#ifndef CELL_HEADER2_H
#define CELL_HEADER2_H

#include <string>
#include <unordered_map>
#include <set>
#include <vector>

#include <iostream>
#include <fstream>
#include <json/json.h>

typedef std::set<std::string> Markers;
typedef std::set<std::string> Metas;

//static std::vector<std::string> meta_list = {"Solidity","Extent","Area","Eccentricity",
//					     "MinorAxisLength","MajorAxisLength","Orientation"};

class CellTable {
    
public:

  CellTable() {}

  CellTable(const char* file, const char* markers_file);
  
  std::unordered_map<std::string, std::vector<float>> table;

  size_t num_cells() const { return table.at(x).size(); }

  using const_iterator = std::unordered_map<std::string, std::vector<float>>::const_iterator;
  const_iterator begin() const { return table.begin(); }
  const_iterator end() const { return table.end(); }

  Metas meta;
  Markers markers;
  std::string x;
  std::string y;

  // methods
  std::vector<double> XY() const;
  

};

class JsonReader {
public:
    JsonReader(const std::string& filename) : filename_(filename) {}

    bool ReadData() {
        std::ifstream json_file(filename_);
        if (!json_file.is_open()) {
            std::cerr << "Error opening file " << filename_ << std::endl;
            return false;
        }

        Json::Value root;
        json_file >> root;

        x_ = root["X"].asString();
        y_ = root["Y"].asString();

        Json::Value markers = root["MARKERS"];
        for (unsigned int i = 0; i < markers.size(); ++i) {
            markers_.push_back(markers[i].asString());
        }

        return true;
    }

    std::string GetX() const { return x_; }
    std::string GetY() const { return y_; }
    const std::vector<std::string>& GetMarkers() const { return markers_; }

private:
    std::string filename_;
    std::string x_;
    std::string y_;
    std::vector<std::string> markers_;
};



#endif




