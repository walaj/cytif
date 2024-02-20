#include "channel.h"

#include <iostream>
#include <fstream>
#include <sstream>

Channel::Channel(int num, const std::string& name, RGBColor col, uint16_t lower, uint16_t upper) 
  : channelNumber(num), channelName(name), color(col), lowerBound(lower), upperBound(upper) {}

Channel::Channel(const std::string& csvLine) {
  
  std::istringstream iss(csvLine);
  std::string token;
  
  std::getline(iss, token, ',');
  channelNumber = std::stoi(token);
  
  std::getline(iss, channelName, ',');
  
  std::getline(iss, token, ',');
  color.r = static_cast<uint8_t>(std::stoi(token));
  
  std::getline(iss, token, ',');
  color.g = static_cast<uint8_t>(std::stoi(token));
  
  std::getline(iss, token, ',');
  color.b = static_cast<uint8_t>(std::stoi(token));
  
  std::getline(iss, token, ',');
  lowerBound = static_cast<uint16_t>(std::stoi(token));
  
  std::getline(iss, token);
  upperBound = static_cast<uint16_t>(std::stoi(token));
  
}


// Overload the << operator for RGBColor
std::ostream& operator<<(std::ostream& os, const RGBColor& color) {
    return os << "RGB(" << static_cast<int>(color.r) << ", " << static_cast<int>(color.g) << ", " << static_cast<int>(color.b) << ")";
}

// Overload the << operator for Channel
std::ostream& operator<<(std::ostream& os, const Channel& channel) {
  os << channel.channelNumber << ":" << channel.channelName << " " << channel.color
     << " [" << channel.lowerBound << "," << channel.upperBound << "]";
    return os;
}
