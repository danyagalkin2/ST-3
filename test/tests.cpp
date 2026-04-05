// Copyright 2021 GHA Test Team

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

#include "TimedDoor.h"

using ::testing::Exactly;

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
  TimedDoor* shortDoor{};
  TimedDoor* longDoor{};

  void SetUp() override {
    shortDoor = new TimedDoor(0);
    longDoor = new TimedDoor(40);
  }

  void TearDown() override {
    delete shortDoor;
    delete longDoor;
  }
};

}  // namespace

TEST_F(TimedDoorTest, ConstructorStoresTimeoutValue) {
  EXPECT_EQ(longDoor->getTimeOut(), 40);
}

TEST_F(TimedDoorTest, DoorIsClosedAfterConstruction) {
  EXPECT_FALSE(shortDoor->isDoorOpened());
}

TEST_F(TimedDoorTest, LockMakesDoorClosed) {
  longDoor->lock();

  EXPECT_FALSE(longDoor->isDoorOpened());
}

TEST_F(TimedDoorTest, ThrowStateDoesNotThrowForClosedDoor) {
  shortDoor->lock();

  EXPECT_NO_THROW(shortDoor->throwState());
}

TEST_F(TimedDoorTest, ThrowStateThrowsForOpenedDoor) {
  auto unlockFuture = std::async(std::launch::async, [this] {
    longDoor->unlock();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_THROW(longDoor->throwState(), std::runtime_error);

  longDoor->lock();
  EXPECT_NO_THROW(unlockFuture.get());
}

TEST_F(TimedDoorTest, AdapterTimeoutDoesNotThrowWhenDoorIsClosed) {
  DoorTimerAdapter adapter(*shortDoor);
  shortDoor->lock();

  EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(TimedDoorTest, AdapterTimeoutThrowsWhenDoorIsOpened) {
  DoorTimerAdapter adapter(*longDoor);
  auto unlockFuture = std::async(std::launch::async, [this] {
    longDoor->unlock();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);

  longDoor->lock();
  EXPECT_NO_THROW(unlockFuture.get());
}

TEST_F(TimedDoorTest, UnlockThrowsWhenDoorRemainsOpenedUntilTimeout) {
  EXPECT_THROW(shortDoor->unlock(), std::runtime_error);
}

TEST_F(TimedDoorTest, UnlockDoesNotThrowIfDoorGetsClosedBeforeTimeout) {
  auto unlockFuture = std::async(std::launch::async, [this] {
    longDoor->unlock();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  longDoor->lock();

  EXPECT_NO_THROW(unlockFuture.get());
  EXPECT_FALSE(longDoor->isDoorOpened());
}

TEST(TimerTest, TimerCallsTimeoutForRegisteredClient) {
  Timer timer;
  MockTimerClient client;

  EXPECT_CALL(client, Timeout()).Times(Exactly(1));

  timer.tregister(0, &client);
}

TEST(DoorInterfaceTest, CloseDoorUsesLockMethod) {
  MockDoor door;

  EXPECT_CALL(door, lock()).Times(Exactly(1));

  closeDoor(&door);
}

TEST(DoorInterfaceTest, OpenDoorUsesUnlockMethod) {
  MockDoor door;

  EXPECT_CALL(door, unlock()).Times(Exactly(1));

  openDoor(&door);
}

TEST(DoorInterfaceTest, GetDoorStateUsesIsDoorOpenedMethod) {
  MockDoor door;

  EXPECT_CALL(door, isDoorOpened()).Times(Exactly(1)).WillOnce(testing::Return(true));

  EXPECT_TRUE(getDoorState(&door));
}
