// Copyright 2021 GHA Test Team

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
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

TEST_F(DoorFixture, GetTimeoutOnFastDoor) {
  EXPECT_EQ(0, fastDoor->getTimeOut());
}

TEST_F(DoorFixture, ExceptionMessageContainsExpectedText) {
  try {
    fastDoor->unlock();
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("timeout"), std::string::npos);
  }
}

TEST_F(DoorFixture, IsDoorOpenedAfterUnlock) {
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  EXPECT_TRUE(slowDoor->isDoorOpened());
  slowDoor->lock();
  t.join();
}

TEST_F(DoorFixture, LockAfterUnlockRestoresClosed) {
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  slowDoor->lock();
  t.join();
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST(TimedDoorTest, ThrowStateDoesNotThrowOnFreshDoor) {
  TimedDoor door(5);
  EXPECT_NO_THROW(door.throwState());
}

TEST_F(DoorFixture, MultipleLockUnlockCyclesNoThrow) {
  slowDoor->lock();
  EXPECT_FALSE(slowDoor->isDoorOpened());
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  slowDoor->lock();
  t.join();
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST_F(DoorFixture, AdapterTimeoutCalledTwiceOnClosedDoor) {
  DoorTimerAdapter adapter(*fastDoor);
  fastDoor->lock();
  EXPECT_NO_THROW(adapter.Timeout());
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(TimedDoorTest, NewDoorWithLongTimeoutIsClosedInitially) {
  TimedDoor door(100);
  EXPECT_EQ(100, door.getTimeOut());
  EXPECT_FALSE(door.isDoorOpened());
}

TEST_F(DoorFixture, UnlockThrowsStdRuntimeErrorType) {
  ASSERT_THROW(fastDoor->unlock(), std::runtime_error);
}

TEST(TimerTest, TimerRegisterCallsTimeoutWithNonZeroTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(Exactly(1));
  timer.tregister(1, &client);
}

TEST(MockDoorTest, IsDoorOpenedReturnsTrue) {
  NiceMock<MockDoor> door;
  ON_CALL(door, isDoorOpened()).WillByDefault(Return(true));
  ASSERT_TRUE(door.isDoorOpened());
}

TEST(TimerTest, TimerDoesNotCallNullClient) {
  Timer timer;
  EXPECT_NO_THROW(timer.tregister(0, nullptr));
}

TEST_F(DoorFixture, AdapterReferenceTracksLockState) {
  DoorTimerAdapter adapter(*slowDoor);
  EXPECT_NO_THROW(adapter.Timeout());
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
  slowDoor->lock();
  t.join();
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST_F(DoorFixture, LockIdempotentOnOpenDoor) {
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  slowDoor->lock();
  slowDoor->lock();
  t.join();
  EXPECT_FALSE(slowDoor->isDoorOpened());
}

TEST_F(DoorFixture, GetTimeoutUnchangedAfterUnlockLock) {
  std::thread t([this] { slowDoor->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  slowDoor->lock();
  t.join();
  EXPECT_EQ(60, slowDoor->getTimeOut());
}

// --- Parameterized tests ---

class TimedDoorParamTest : public ::testing::TestWithParam<int> {};

TEST_P(TimedDoorParamTest, TimeoutStoredForAnyValue) {
  int timeout = GetParam();
  TimedDoor door(timeout);
  EXPECT_EQ(timeout, door.getTimeOut());
  EXPECT_FALSE(door.isDoorOpened());
}

INSTANTIATE_TEST_SUITE_P(
    TimeoutValues,
    TimedDoorParamTest,
    ::testing::Values(0, 1, 5, 10, 50, 100, 500, 1000));

// --- MiddleDoorFixture (30ms timeout) ---

class MiddleDoorFixture : public ::testing::Test {
 protected:
  TimedDoor* door{};
  void SetUp() override { door = new TimedDoor(30); }
  void TearDown() override { delete door; }
};

TEST_F(MiddleDoorFixture, MiddleTimeoutStoredCorrectly) {
  EXPECT_EQ(30, door->getTimeOut());
}

TEST_F(MiddleDoorFixture, MiddleDoorThrowsIfLeftOpen) {
  std::thread t([this] { door->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_THROW(door->throwState(), std::runtime_error);
  door->lock();
  t.join();
}

TEST_F(MiddleDoorFixture, MiddleDoorNoThrowIfLockedEarly) {
  std::thread t([this] { door->unlock(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  door->lock();
  t.join();
  EXPECT_FALSE(door->isDoorOpened());
}

// --- Destructor test ---

TEST(TimedDoorTest, DestructorReleasesAdapter) {
  { TimedDoor door(5); }
  SUCCEED();
}
