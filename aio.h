#include <cstdint>

class Aio {
public:
  Aio();
  ~Aio();
  Aio(const Aio &) = delete;

  void sleep(uint32_t ms, void *ctx, void (*cb)(void *));
  void interval(uint32_t ms, void *ctx, uint8_t (*cb)(void *));

private:
  void *aio;
};
