#include "aio.h"

extern "C" {
void *aio__construct();
void aio__destruct(void *aio);

void aio__sleep(void *aio, uint32_t ms, void *ctx, void (*cb)(void *));
}

Aio::Aio() { aio = aio__construct(); }

Aio::~Aio() { aio__destruct(aio); }

void Aio::sleep(uint32_t ms, void *ctx, callback cb) {
  aio__sleep(aio, ms, ctx, cb);
}
