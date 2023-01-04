#ifndef TIFF_UTILS_H
#define TIFF_UTILS_H

/** Format an integer to include commas
 * @param data Number to format
 * @return String with formatted number containing commas
 */

template <typename T> inline
std::string AddCommas(T data) {
  std::stringstream ss; 
  ss << std::fixed << data; 
  std::string s = ss.str();
  if (s.length() > 3)
    for (int i = s.length()-3; i > 0; i -= 3)
      s.insert(i,",");
  return s;
}

#endif
