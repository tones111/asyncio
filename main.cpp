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

  // UDP broadcast
  UdpSocket s_orig = aio.udp_bind(0x7F000001, 1234);
  UdpSocket s = std::move(s_orig); // Note: sockets can be moved
  s.set_broadcast(true);
  const char *mesg = "hello from c++";
  s.send_to(reinterpret_cast<const uint8_t *>(mesg), strlen(mesg) + 1,
            0x7FFFFFFF, 10'000);
  s.send_to(reinterpret_cast<const uint8_t *>(mesg), strlen(mesg) + 1,
            0x7FFFFFFF, 10'000);

  // Asynchronous Interval - The callback is invoked at the specified
  // interval while the callback returns "true".
  // Note: context only accessed by the callback (IO thread) doesn't need to
  // be thread-safe.
  struct CTX {
    uint32_t i;
  };
  CTX ictx{0};

  std::thread([&aio, &ictx]() {
    aio.interval(100, &ictx, [](void *ictxp) -> uint8_t {
      try {
        CTX &ctx = *static_cast<CTX *>(ictxp);
        std::cout << "interval " << ++ctx.i << std::endl;
        // Note: can't send_to here because it would block the IO runtime
        return ctx.i < 5 ? 1 : 0;
      } catch (...) {
        // Don't unwind through the FFI boundary
      }
      return 0;
    });
  }).join();

  UdpSocket s1 = aio.udp_bind(0x7F000001, 10'001);
  s1.recv_from(1024, &s1,
               [](void *context, uint8_t *buf, uint32_t len, uint32_t ip,
                  uint16_t port) {
                 std::cout << "s1 received " << len << " bytes from " << ip
                           << ":" << port << std::endl;
                 // Note: can't send_to here because it would block the IO
                 // runtime
               });

  UdpSocket s2 = aio.udp_bind(0x7F000001, 10'002);
  s2.recv_from(1024, &s2,
               [](void *context, uint8_t *buf, uint32_t len, uint32_t ip,
                  uint16_t port) {
                 UdpSocket &sock = *static_cast<UdpSocket *>(context);
                 std::cout << "s2 received " << len << " bytes from " << ip
                           << ":" << port << std::endl;
                 // Note: can't send_to here because it would block the IO
                 // runtime
               });

  const char *s1_mesg = "ping";
  s1.send_to(reinterpret_cast<const uint8_t *>(s1_mesg), strlen(s1_mesg) + 1,
             0x7F000001, 10'002);

  // This sleep is required to keep the application running long enough for the
  // async operations to complete.
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  return 0;
}