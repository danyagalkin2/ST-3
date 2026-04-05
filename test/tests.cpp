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

void closeDoor(Door* door) {
  door->lock();
}

void openDoor(Door* door) {
  door->unlock();
}

bool getDoorState(Door* door) {
  return door->isDoorOpened();
}

class TimedDoorTest : public ::testing::Test {
 protected:
  TimedDoor* instantDoor{};
  TimedDoor* delayedDoor{};

  void SetUp() override {
    instantDoor = new TimedDoor(0);
    delayedDoor = new TimedDoor(40);
  }

  void TearDown() override {
    delete instantDoor;
    delete delayedDoor;
  }
};

}  // namespace

TEST_F(TimedDoorTest, ConstructorStoresTimeoutValue) {
  EXPECT_EQ(delayedDoor->getTimeOut(), 40);
}

TEST_F(TimedDoorTest, DoorIsClosedAfterConstruction) {
  EXPECT_FALSE(instantDoor->isDoorOpened());
}

TEST_F(TimedDoorTest, LockClosesDoor) {
  delayedDoor->lock();

  EXPECT_FALSE(delayedDoor->isDoorOpened());
}

TEST_F(TimedDoorTest, ThrowStateDoesNotThrowForClosedDoor) {
  instantDoor->lock();

  EXPECT_NO_THROW(instantDoor->throwState());
}

TEST_F(TimedDoorTest, ThrowStateThrowsForOpenedDoor) {
  auto unlockFuture = std::async(std::launch::async, [this] {
    delayedDoor->unlock();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_THROW(delayedDoor->throwState(), std::runtime_error);

  delayedDoor->lock();
  EXPECT_NO_THROW(unlockFuture.get());
}

TEST_F(TimedDoorTest, AdapterTimeoutDoesNotThrowWhenDoorIsClosed) {
  DoorTimerAdapter adapter(*instantDoor);
  instantDoor->lock();

  EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(TimedDoorTest, AdapterTimeoutThrowsWhenDoorIsOpened) {
  DoorTimerAdapter adapter(*delayedDoor);
  auto unlockFuture = std::async(std::launch::async, [this] {
    delayedDoor->unlock();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);

  delayedDoor->lock();
  EXPECT_NO_THROW(unlockFuture.get());
}

TEST_F(TimedDoorTest, UnlockThrowsWhenDoorRemainsOpenedUntilTimeout) {
  EXPECT_THROW(instantDoor->unlock(), std::runtime_error);
}

TEST_F(TimedDoorTest, UnlockDoesNotThrowIfDoorIsClosedBeforeTimeout) {
  auto unlockFuture = std::async(std::launch::async, [this] {
    delayedDoor->unlock();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  delayedDoor->lock();

  EXPECT_NO_THROW(unlockFuture.get());
  EXPECT_FALSE(delayedDoor->isDoorOpened());
}

TEST(TimerTest, TimerCallsTimeoutForRegisteredClient) {
  Timer timer;
  MockTimerClient client;

  EXPECT_CALL(client, Timeout()).Times(Exactly(1));

  timer.tregister(0, &client);
}

TEST(DoorInterfaceTest, HelperCloseDoorUsesLockMethod) {
  MockDoor door;

  EXPECT_CALL(door, lock()).Times(Exactly(1));

  closeDoor(&door);
}

TEST(DoorInterfaceTest, HelperOpenDoorUsesUnlockMethod) {
  MockDoor door;

  EXPECT_CALL(door, unlock()).Times(Exactly(1));

  openDoor(&door);
}

TEST(DoorInterfaceTest, HelperGetDoorStateUsesIsDoorOpenedMethod) {
  MockDoor door;

  EXPECT_CALL(door, isDoorOpened()).Times(Exactly(1))
      .WillOnce(Return(true));

  EXPECT_TRUE(getDoorState(&door));
}
