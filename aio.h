#include <cstdint>
#include <utility>

class UdpSocket;

// TODO: should Aio be a singleton?
class Aio {
public:
  Aio();
  ~Aio();
  Aio(const Aio &) = delete;

  void sleep(uint32_t ms, void *ctx, void (*cb)(void *));
  void interval(uint32_t ms, void *ctx, uint8_t (*cb)(void *));

  UdpSocket udp_bind(uint32_t ip, uint16_t port);

private:
  void *aio;
};

class UdpSocket {
  friend class Aio;

public:
  ~UdpSocket();

  // Prevent copies
  UdpSocket(const UdpSocket &) = delete;
  UdpSocket operator=(const UdpSocket &) = delete;

  // Allow moves
  UdpSocket(UdpSocket &&other) noexcept
      : sock{std::exchange(other.sock, nullptr)} {}
  UdpSocket &operator=(UdpSocket &&other) noexcept {
    if (this != &other) {
      sock = std::exchange(other.sock, nullptr);
    }
    return *this;
  }

  bool set_broadcast(bool on);

private:
  UdpSocket() : sock{nullptr} {}

  void *sock;
};