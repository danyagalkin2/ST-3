// Copyright 2021 GHA Test Team

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <thread>

#include "TimedDoor.h"

using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::NiceMock;
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
  TimedDoor* fastDoor{};
  TimedDoor* slowDoor{};

  void SetUp() override {
    fastDoor = new TimedDoor(0);
    slowDoor  = new TimedDoor(60);
  }

  void TearDown() override {
    delete fastDoor;
    delete slowDoor;
  }
};

}  // namespace

TEST_F(DoorFixture, InitialStateIsClosed) {
  EXPECT_FALSE(fastDoor->isDoorOpened());
}

TEST_F(DoorFixture, TimeoutStoredCorrectly) {
  EXPECT_EQ(60, slowDoor->getTimeOut());
}

TEST_F(DoorFixture, LockSetsDoorClosed) {
  slowDoor->lock();
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST_F(DoorFixture, MultipleLocksKeepDoorClosed) {
  slowDoor->lock();
  slowDoor->lock();
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST_F(DoorFixture, TimeoutUnchangedAfterLock) {
  slowDoor->lock();
  EXPECT_EQ(60, slowDoor->getTimeOut());
}

TEST_F(DoorFixture, ThrowStateOkWhenClosed) {
  fastDoor->lock();
  EXPECT_NO_THROW(fastDoor->throwState());
}

TEST_F(DoorFixture, ThrowStateThrowsWhenOpened) {
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  EXPECT_THROW(slowDoor->throwState(), std::runtime_error);
  slowDoor->lock();
  t.join();
}

TEST_F(DoorFixture, UnlockThrowsIfDoorStaysOpen) {
  EXPECT_THROW(fastDoor->unlock(), std::runtime_error);
}

TEST_F(DoorFixture, NoThrowIfClosedBeforeTimeout) {
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  slowDoor->lock();
  t.join();
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST_F(DoorFixture, AdapterNoThrowOnClosedDoor) {
  DoorTimerAdapter adapter(*fastDoor);
  fastDoor->lock();
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(DoorFixture, AdapterThrowsOnOpenDoor) {
  DoorTimerAdapter adapter(*slowDoor);
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
  slowDoor->lock();
  t.join();
}

TEST(TimerTest, RegisteredClientReceivesTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(Exactly(1));
  timer.tregister(0, &client);
}

TEST(MockDoorTest, LockIsCalledViaInterface) {
  MockDoor door;
  EXPECT_CALL(door, lock()).Times(AtLeast(1));
  auto doClose = [](Door& d) { d.lock(); };
  doClose(door);
}

TEST(MockDoorTest, UnlockIsCalledViaInterface) {
  MockDoor door;
  EXPECT_CALL(door, unlock()).Times(AtLeast(1));
  auto doOpen = [](Door& d) { d.unlock(); };
  doOpen(door);
}

TEST(MockDoorTest, IsDoorOpenedReturnsFalse) {
  NiceMock<MockDoor> door;
  ON_CALL(door, isDoorOpened()).WillByDefault(Return(false));
  ASSERT_FALSE(door.isDoorOpened());
}
