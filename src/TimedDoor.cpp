// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor& timedDoor) : door(timedDoor) {}

void DoorTimerAdapter::Timeout() {
  door.throwState();
}

TimedDoor::TimedDoor(int timeout)
    : adapter(new DoorTimerAdapter(*this)),
      iTimeout(timeout),
      isOpened(false) {}

TimedDoor::~TimedDoor() {
  delete adapter;
}

bool TimedDoor::isDoorOpened() {
  return isOpened;
}

void TimedDoor::unlock() {
  isOpened = true;

  Timer timer;
  timer.tregister(iTimeout, adapter);
}

void TimedDoor::lock() {
  isOpened = false;
}

int TimedDoor::getTimeOut() const {
  return iTimeout;
}

void TimedDoor::throwState() {
  if (isOpened) {
    throw std::runtime_error("Door is open past allowed timeout");
  }
}

void Timer::sleep(int timeout) {
  auto ms = std::chrono::milliseconds(std::max(0, timeout));
  std::this_thread::sleep_for(ms);
}

void Timer::tregister(int timeout, TimerClient* timerClient) {
  client = timerClient;
  sleep(timeout);

  if (client != nullptr) {
    client->Timeout();
  }
}
