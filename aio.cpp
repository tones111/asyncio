#include "aio.h"

extern "C" {
void *aio__construct();
void aio__destruct(void *aio);

void aio__sleep(void *aio, uint32_t ms, void *ctx, void (*cb)(void *));
void aio__interval(void *aio, uint32_t ms, void *ctx, uint8_t (*cb)(void *));
}

Aio::Aio() { aio = aio__construct(); }

Aio::~Aio() { aio__destruct(aio); }

void Aio::sleep(uint32_t ms, void *ctx, void (*cb)(void *)) {
  aio__sleep(aio, ms, ctx, cb);
}

void Aio::interval(uint32_t ms, void *ctx, uint8_t (*cb)(void *)) {
  aio__interval(aio, ms, ctx, cb);
}
