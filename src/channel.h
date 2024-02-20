#include <cstdint>
#include <string>
#include <vector>

// Define a struct to hold RGB color values
struct RGBColor {
    uint8_t r, g, b;
  
    RGBColor(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
      : r(red), g(green), b(blue) {}

  // print overload
  friend std::ostream& operator<<(std::ostream& os, const RGBColor& channel);
  
};

// Define a struct to represent a channel with its associated properties
struct Channel {
  int channelNumber; // Channel identifier
  std::string channelName;
  RGBColor color; // RGB color associated with this channel
  uint16_t lowerBound; // Lower bound for windowing
  uint16_t upperBound; // Upper bound for windowing
  
  // Constructor to initialize the Channel
  Channel(int num, const std::string& name, RGBColor col, uint16_t lower, uint16_t upper);
  
  // take a csv line
  Channel(const std::string& csvLine);

  // print overload
  friend std::ostream& operator<<(std::ostream& os, const Channel& channel);
};

typedef std::vector<Channel> ChannelVector;
