#include <chrono>
#include <iostream>
#include <thread>

#include "aio.h"

int main(int argc, char *argv[]) {
  std::cout << "hello world" << std::endl;
  Aio aio;

  aio.sleep(2000, []() { std::cout << "callback" << std::endl; });

  std::this_thread::sleep_for(std::chrono::milliseconds(4000));

  std::cout << "goodbye world" << std::endl;
  return 0;
}