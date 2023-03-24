#include "aio.h"

extern "C" {
void *aio__construct();
void aio__destruct(void *aio);
}

Aio::Aio() { aio = aio__construct(); }

Aio::~Aio() { aio__destruct(aio); }