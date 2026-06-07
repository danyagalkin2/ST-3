// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <exception>
#include <iostream>

int main() {
  TimedDoor tDoor(5);
  tDoor.lock();
  try {
    tDoor.unlock();
  } catch (const std::exception& error) {
    std::cout << error.what() << std::endl;
  }

  return 0;
}
