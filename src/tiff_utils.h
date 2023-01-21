#ifndef TIFF_UTILS_H
#define TIFF_UTILS_H

#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <numeric>

#include "tiffio.h"

int MergeGrayToRGB(TIFF* in, TIFF* out);

static int cnt = 0; 
#define DEBUGP do { std::cerr << "DEBUGP: " << cnt++ << std::endl; } while(0)

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

// construct a string with only printable characters
// gets rid of weird carriage return etc issues
std::string inline clean_string(const std::string& str) {
    std::string result;
    for (char c : str)
      if (std::isprint(c))
	result += c;
    return result;
}

int inline check_tif(TIFF* tif) {

  if (tif == NULL || tif == 0) {
    fprintf(stderr, "Error: TIFF is NULL\n");
    return 1;
  }
  return 0;
}

double inline getMean(std::vector<int>& vec) {
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    int n = vec.size();
    return (double)sum / n;
}

int inline getMode(std::vector<int>& vec) {
    std::unordered_map<int, int> count;
    for (int i : vec) 
        count[i]++;
    int mode = vec[0];
    int maxCount = 0;
    for (auto i : count) {
        if (i.second > maxCount) {
            mode = i.first;
            maxCount = i.second;
        }
    }
    return mode;
}
#endif
