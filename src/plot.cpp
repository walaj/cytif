#include <cmath>
#include <matplot/matplot.h>

int myplot() {
  using namespace matplot;
  std::vector<double> x = matplot::linspace(0, 2 * pi);
  std::vector<double> y = matplot::transform(x, [](auto x) { return sin(x); });
  
  matplot::plot(x, y, "-o");
  matplot::hold(on);
  matplot::plot(x, transform(y, [](auto y) { return -y; }), "--xr");
  matplot::plot(x, transform(x, [](auto x) { return x / pi - 1.; }), "-:gs");
  matplot::plot({1.0, 0.7, 0.4, 0.0, -0.4, -0.7, -1}, "k");
  
  matplot::show();
  return 0;
}
