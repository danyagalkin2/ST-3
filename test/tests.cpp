// Copyright 2021 GHA Test Team

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>

#include "TimedDoor.h"

using ::testing::Exactly;
using ::testing::Return;

namespace {

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class DoorFixture : public ::testing::Test {
 protected:
  TimedDoor* zeroDoor{};
  TimedDoor* slowDoor{};

  void SetUp() override {
    zeroDoor = new TimedDoor(0);
    slowDoor  = new TimedDoor(50);
  }

  void TearDown() override {
    delete zeroDoor;
    delete slowDoor;
  }
};

}  // namespace

TEST_F(DoorFixture, TimeoutValueStoredOnCreate) {
  EXPECT_EQ(slowDoor->getTimeOut(), 50);
}

TEST_F(DoorFixture, DoorStartsInClosedState) {
  EXPECT_FALSE(zeroDoor->isDoorOpened());
}

TEST_F(DoorFixture, LockSetsClosedState) {
  slowDoor->lock();
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST_F(DoorFixture, NoThrowWhenDoorLocked) {
  zeroDoor->lock();
  EXPECT_NO_THROW(zeroDoor->throwState());
}

TEST_F(DoorFixture, ThrowWhenDoorUnlocked) {
  auto fut = std::async(std::launch::async, [this] {
    slowDoor->unlock();
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_THROW(slowDoor->throwState(), std::runtime_error);
  slowDoor->lock();
  EXPECT_NO_THROW(fut.get());
}

TEST_F(DoorFixture, AdapterNoThrowOnLockedDoor) {
  DoorTimerAdapter adapter(*zeroDoor);
  zeroDoor->lock();
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(DoorFixture, AdapterThrowsOnUnlockedDoor) {
  DoorTimerAdapter adapter(*slowDoor);
  auto fut = std::async(std::launch::async, [this] {
    slowDoor->unlock();
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
  slowDoor->lock();
  EXPECT_NO_THROW(fut.get());
}

TEST_F(DoorFixture, UnlockThrowsAfterTimeout) {
  EXPECT_THROW(zeroDoor->unlock(), std::runtime_error);
}

TEST_F(DoorFixture, NoThrowIfLockedBeforeTimeout) {
  auto fut = std::async(std::launch::async, [this] {
    slowDoor->unlock();
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  slowDoor->lock();
  EXPECT_NO_THROW(fut.get());
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST(TimerTest, ClientTimeoutCalledOnce) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(Exactly(1));
  timer.tregister(0, &client);
}

TEST(MockDoorTest, LockMethodInvoked) {
  MockDoor door;
  EXPECT_CALL(door, lock()).Times(Exactly(1));
  door.lock();
}

TEST(MockDoorTest, UnlockMethodInvoked) {
  MockDoor door;
  EXPECT_CALL(door, unlock()).Times(Exactly(1));
  door.unlock();
}

TEST(MockDoorTest, IsDoorOpenedReturnsValue) {
  MockDoor door;
  EXPECT_CALL(door, isDoorOpened()).Times(Exactly(1))
      .WillOnce(Return(false));
  EXPECT_FALSE(door.isDoorOpened());
}
