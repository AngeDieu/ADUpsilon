#include <ion/timing.h>
#include "main.h"
#include <chrono>
// #include <libndls.h>

#include "k_csdk.h"

// static auto start = std::chrono::steady_clock::now();

namespace Ion {
namespace Timing {

uint64_t millis() {
  return ::millis();
  // auto elapsed = std::chrono::steady_clock::now() - start;
  // return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

void usleep(uint32_t us) {
  os_wait_1ms(us/1000); // sleep_us(us);
}

void msleep(uint32_t ms) {
  os_wait_1ms(ms); // sleep_us(ms * 1000);
}

}
}
