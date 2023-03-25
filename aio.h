#include <cstdint>

class Aio {
public:
  using callback = void (*)(void *);

  Aio();
  ~Aio();
  Aio(const Aio &) = delete;

  void sleep(uint32_t ms, void *ctx, callback cb);

private:
  void *aio;
};
