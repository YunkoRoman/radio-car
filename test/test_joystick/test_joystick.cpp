// Host test: g++ -std=c++17 -Wall -Wextra -Werror -Iinclude
//   test/test_joystick/test_joystick.cpp -o /tmp/test_joystick && /tmp/test_joystick
#include <cassert>
#include <cstdio>

#include "joystick.h"

int main() {
  constexpr AxisCalibration cal{50, 2300, 4045, 60, false};
  constexpr AxisCalibration inv{50, 2300, 4045, 60, true};

  // Centre and dead zone.
  assert(normalizeAxis(2300, cal) == 0);
  assert(normalizeAxis(2360, cal) == 0);
  assert(normalizeAxis(2240, cal) == 0);
  assert(normalizeAxis(2370, cal) > 0);
  assert(normalizeAxis(2230, cal) < 0);

  // Endpoints and clamping.
  assert(normalizeAxis(4045, cal) == 1000);
  assert(normalizeAxis(4095, cal) == 1000);
  assert(normalizeAxis(50, cal) == -1000);
  assert(normalizeAxis(0, cal) == -1000);

  // Inversion.
  assert(normalizeAxis(6, inv) == 1000);
  assert(normalizeAxis(4095, inv) == -1000);
  assert(normalizeAxis(2300, inv) == 0);

  std::puts("test_joystick: ok");
  return 0;
}
