#ifndef TIFF_UTILS_H
#define TIFF_UTILS_H

#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

/** Format an integer to include commas
 * @param data Number to format
 * @return String with formatted number containing commas
 */
template <typename T> inline std::string AddCommas(T data) {
  std::stringstream ss; 
  ss << std::fixed << data; 
  std::string s = ss.str();
  if (s.length() > 3)
    for (int i = s.length()-3; i > 0; i -= 3)
      s.insert(i,",");
  return s;
}

// generate x, y points to display a circle of radius
std::vector<std::pair<float, float>> inline get_circle_points(int radius)  {
    std::vector<std::pair<float, float>> points;
    for (int i = 0; i <= 360; i++) {
        double angle = i * M_PI / 180;
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        points.emplace_back(x, y);
    }
    return points;
}


#endif
