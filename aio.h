#include <cstdint>

class Aio {
public:
  Aio();
  ~Aio();
  Aio(const Aio &) = delete;

private:
  void *aio;
};
