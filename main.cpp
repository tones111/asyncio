#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "aio.h"

int main(int argc, char *argv[]) {
  Aio aio;

  {
    // Asynchronous Sleep - The callback is invoked once after the specified
    // duration
    std::atomic<uint32_t> context{123};
    // Note: any captured context must be thread-safe.
    std::thread([&aio, &context]() {
      aio.sleep(100, &context, [](void *ctxp) {
        try {
          std::atomic<uint32_t> &ctx =
              *static_cast<std::atomic<uint32_t> *>(ctxp);
          std::cout << "sleep callback: " << ctx << std::endl;
        } catch (...) {
          // Don't unwind through the FFI boundary
        }
      });
    }).join();
    context = 456;
    // This sleep is required to keep the context alive for the callback
    // invocation.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  return 0;
}