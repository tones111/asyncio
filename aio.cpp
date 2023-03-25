#include "aio.h"

extern "C" {
void *aio__construct();
void aio__destruct(void *aio);

void aio__sleep(void *aio, uint32_t ms, void *ctx, void (*cb)(void *));
void aio__interval(void *aio, uint32_t ms, void *ctx, uint8_t (*cb)(void *));

void *aio__udp_bind(void *aio, uint32_t ip, uint16_t port);
void aio__udp_destruct(void *udp);
uint8_t aio__udp_set_broadcast(void *udp, uint8_t on);

void aio__udp_recv_from(void *ctx, void *udp, uint32_t max_size,
                        void (*cb)(void *ctx, uint8_t *buf, uint32_t len,
                                   uint32_t ip, uint16_t port));
uint32_t aio__udp_send_to(void *udp, const uint8_t *buf, uint32_t len,
                          uint32_t ip, uint16_t port);
}

Aio::Aio() {
  // TODO: throw if null pointer returned
  aio = aio__construct();
}

Aio::~Aio() { aio__destruct(aio); }

void Aio::sleep(uint32_t ms, void *ctx, void (*cb)(void *)) {
  aio__sleep(aio, ms, ctx, cb);
}

void Aio::interval(uint32_t ms, void *ctx, uint8_t (*cb)(void *)) {
  aio__interval(aio, ms, ctx, cb);
}

UdpSocket Aio::udp_bind(uint32_t ip, uint16_t port) {
  UdpSocket socket;
  // TODO: throw if null pointer returned
  socket.sock = aio__udp_bind(aio, ip, port);
  return socket;
}

UdpSocket::~UdpSocket() { aio__udp_destruct(sock); }

bool UdpSocket::set_broadcast(bool on) {
  return aio__udp_set_broadcast(sock, on ? 1 : 0) != 0;
}

void UdpSocket::recv_from(uint32_t max_size, void *ctx,
                          void (*cb)(void *ctx, uint8_t *buf, uint32_t len,
                                     uint32_t ip, uint16_t port)) {
  aio__udp_recv_from(sock, ctx, max_size, cb);
}

uint32_t UdpSocket::send_to(const uint8_t *buf, uint32_t len, uint32_t ip,
                            uint16_t port) {
  return aio__udp_send_to(sock, buf, len, ip, port);
}