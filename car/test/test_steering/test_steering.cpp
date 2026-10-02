// Host test: g++ -std=c++17 -Wall -Wextra -Werror -Iinclude
//   test/test_steering/test_steering.cpp -o /tmp/test_steering && /tmp/test_steering
#include <cassert>
#include <cstdio>

#include "steering.h"

int main() {
  constexpr ServoCalibration cal{1500, 300, false};
  constexpr ServoCalibration inv{1480, 250, true};

  assert(steeringToPulseUs(0, cal) == 1500);
  assert(steeringToPulseUs(1000, cal) == 1800);
  assert(steeringToPulseUs(-1000, cal) == 1200);
  assert(steeringToPulseUs(500, cal) == 1650);

  // Out-of-range input is clamped to full lock.
  assert(steeringToPulseUs(1500, cal) == 1800);
  assert(steeringToPulseUs(-1500, cal) == 1200);

  // Inversion and off-centre trim.
  assert(steeringToPulseUs(0, inv) == 1480);
  assert(steeringToPulseUs(1000, inv) == 1230);
  assert(steeringToPulseUs(-1000, inv) == 1730);

  std::puts("test_steering: ok");
  return 0;
}
