#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "aio.h"

// Note: The async operations are wrapped in threads and run under tsan to
// test/demonstrate thread safety requirements.

int main(int argc, char *argv[]) {
  Aio aio;

  // Asynchronous Sleep - The callback is invoked once after the specified
  // duration
  // Note: any captured context must be thread-safe.
  std::atomic<uint32_t> scontext{123};
  std::thread([&aio, &scontext]() {
    aio.sleep(100, &scontext, [](void *ctxp) {
      try {
        std::atomic<uint32_t> &ctx =
            *static_cast<std::atomic<uint32_t> *>(ctxp);
        std::cout << "sleep callback: " << ctx << std::endl;
      } catch (...) {
        // Don't unwind through the FFI boundary
      }
    });
  }).join();
  // demonstrate modification by the main thread is seen in the callback
  // (requires atomic)
  scontext = 456;

  // Asynchronous Interval - The callback is invoked at the specified
  // interval while the callback returns "true".
  // Note: context only accessed by the callback (IO thread) doesn't need to
  // be thread-safe.
  uint32_t icontext{0};
  std::thread([&aio, &icontext]() {
    aio.interval(100, &icontext, [](void *ctxp) -> uint8_t {
      try {
        uint32_t &i = *static_cast<uint32_t *>(ctxp);
        std::cout << "interval " << ++i << std::endl;
        return i < 5 ? 1 : 0;
      } catch (...) {
        // Don't unwind through the FFI boundary
      }
      return 0;
    });
  }).join();

  UdpSocket s_orig = aio.udp_bind(0x7F000001, 1234);
  UdpSocket s = std::move(s_orig); // Note: sockets can be moved
  s.set_broadcast(true);
  const char *mesg = "hello from c++";
  s.send_to(reinterpret_cast<const uint8_t *>(mesg), strlen(mesg) + 1,
            0x7FFFFFFF, 10'000);

  // This sleep is required to keep the application running long enough for the
  // async operations to complete.
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  return 0;
}