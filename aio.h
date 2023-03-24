#include <cstdint>

class Aio {
public:
  using callback = void (*)();

  Aio();
  ~Aio();
  Aio(const Aio &) = delete;

  void sleep(uint32_t ms, callback cb);

private:
  void *aio;
};
