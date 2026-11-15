#include <gtest/gtest.h>

#include <atomic>
#include <cerrno>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include <oxide/posix/shared_memory.hpp>

namespace oxide::posix {
namespace {

// class SharedMemoryTest : public ::testing::Test {
// protected:
//     std::string name;

//     void SetUp() override {
//         static std::atomic<unsigned> counter{0};
//         name = "/ox_test_"
//             + std::to_string(::getpid())
//             + "_"
//             + std::to_string(counter.fetch_add(1));
//     }

//     void TearDown() override { discard(posix::shm_unlink(CStr{name})); }

//     auto builder() { return shared_memory(String::from(name)); }

//     Option<SharedMemory> create(usize size) {
//         return builder().creation_policy(CreationPolicy::create_new()).size(size).create();
//     }

//     void expect_missing() {
//         auto fd = posix::shm_open(Str{name.data(), name.size()}, OFlag{O_RDONLY}, Mode::none());
//         EXPECT_TRUE(fd.is_invalid());
//         EXPECT_EQ(errno, ENOENT);
//     }
// };

// TEST_F(SharedMemoryTest, AnotherProcessCanOpenAndReadPublishedBytes) {
//     auto owner = create(SystemInfo::page_size());
//     ASSERT_TRUE(owner.is_some());
//     owner->as_mut_slice()[0] = 42;

//     const pid_t child = ::fork();
//     ASSERT_NE(child, -1);
//     if (child == 0) {
//         auto reader = builder().open_existing(AccessMode::read());
//         const bool matches = reader.is_some() && reader->as_slice()[0] == 42;
//         // Avoid destroying the inherited owner's handle in the child.
//         ::_exit(matches ? 0 : 1);
//     }

//     int status = 0;
//     pid_t waited;
//     do {
//         waited = ::waitpid(child, &status, 0);
//     } while (waited == -1 && errno == EINTR);
//     ASSERT_EQ(waited, child);
//     ASSERT_TRUE(WIFEXITED(status));
//     EXPECT_EQ(WEXITSTATUS(status), 0);
// }

// TEST_F(SharedMemoryTest, OffsetMappingStartsAtItsFirstMappedByte) {
//     const usize page = SystemInfo::page_size();
//     auto owner = create(2 * page);
//     ASSERT_TRUE(owner.is_some());
//     owner->as_mut_slice()[page] = 73;

//     auto reader = builder().mapping_offset(page).open_existing(AccessMode::read_write());
//     ASSERT_TRUE(reader.is_some());
//     EXPECT_EQ(reader->size(), page);
//     EXPECT_EQ(reader->as_slice(), reader->base_address());
//     EXPECT_EQ(reader->as_slice()[0], 73);
//     reader->as_mut_slice()[0] = 74;
//     EXPECT_EQ(owner->as_slice()[page], 74);
// }

// TEST_F(SharedMemoryTest, OpenOrCreatePreservesExistingDataAndOwnership) {
//     const usize page = SystemInfo::page_size();
//     auto owner = create(page);
//     ASSERT_TRUE(owner.is_some());
//     owner->as_mut_slice()[0] = 91;
//     {
//         auto opener = builder()
//                           .creation_policy(CreationPolicy::open_or_create())
//                           .size(page)
//                           .create();
//         ASSERT_TRUE(opener.is_some());
//         EXPECT_FALSE(opener->has_ownership());
//         EXPECT_EQ(opener->as_slice()[0], 91);
//     }
//     EXPECT_TRUE(builder().open_existing(AccessMode::read()).is_some());
//     owner = NONE;
//     expect_missing();
// }

// TEST_F(SharedMemoryTest, MappingFailureRollsBackCreationEvenWithoutOwnership) {
//     Errno::set(Errno::EXISTS);
//     auto failed = builder()
//                       .address_hint(reinterpret_cast<void*>(1))
//                       .creation_policy(CreationPolicy::create_new())
//                       .has_ownership(false)
//                       .size(SystemInfo::page_size())
//                       .create();
//     // A mapping cannot start at this unaligned address. Platforms can reject
//     // mmap itself or choose an address that fails the enforced-hint check.
//     ASSERT_TRUE(failed.is_none());
//     EXPECT_TRUE(errno == EINVAL || errno == EADDRNOTAVAIL) << errno;
//     expect_missing();
// }

// TEST_F(SharedMemoryTest, MoveAssignmentRemovesTheDestinationsPreviousObject) {
//     auto destination = create(SystemInfo::page_size());
//     auto source = shared_memory(String::from(name + "_next"))
//                       .creation_policy(CreationPolicy::create_new())
//                       .size(SystemInfo::page_size())
//                       .create();
//     ASSERT_TRUE(destination.is_some());
//     ASSERT_TRUE(source.is_some());
//     source->as_mut_slice()[0] = 19;

//     *destination = std::move(*source);
//     EXPECT_FALSE(source->has_ownership());
//     EXPECT_EQ(destination->as_slice()[0], 19);
//     expect_missing();
// }

// TEST_F(SharedMemoryTest, ExplicitUnlinkDoesNotRemoveALaterReplacement) {
//     auto original = create(SystemInfo::page_size());
//     ASSERT_TRUE(original.is_some());
//     ASSERT_EQ(original->unlink(), 0);
//     EXPECT_FALSE(original->has_ownership());

//     auto replacement = create(SystemInfo::page_size());
//     ASSERT_TRUE(replacement.is_some());
//     original = NONE;
//     EXPECT_TRUE(builder().open_existing(AccessMode::read()).is_some());
// }

// TEST_F(SharedMemoryTest, MoveAssignmentUnlocksBeforeUnmapping) {
//     auto destination = builder()
//                            .memory_locked(true)
//                            .creation_policy(CreationPolicy::create_new())
//                            .size(SystemInfo::page_size())
//                            .create();
//     if (destination.is_none()) {
//         if (errno == EPERM || errno == ENOMEM || errno == EAGAIN) {
//             GTEST_SKIP() << "Memory locking is unavailable under the current limits";
//         }
//         FAIL() << "Shared-memory creation failed: " << errno;
//     }
//     auto source = shared_memory(String::from(name + "_next"))
//                       .creation_policy(CreationPolicy::create_new())
//                       .size(SystemInfo::page_size())
//                       .create();
//     ASSERT_TRUE(source.is_some());
//     *destination = std::move(*source);
//     EXPECT_FALSE(source->has_ownership());
//     expect_missing();
// }

// TEST_F(SharedMemoryTest, CleanupPreservesErrno) {
//     auto owner = create(SystemInfo::page_size());
//     ASSERT_TRUE(owner.is_some());
//     errno = EDOM;
//     owner = NONE;
//     EXPECT_EQ(errno, EDOM);
//     expect_missing();
// }

// TEST_F(SharedMemoryTest, WriteOnlyAccessIsRejectedBeforeOpening) {
//     EXPECT_TRUE(AccessMode::read().has_read());
//     EXPECT_TRUE(AccessMode::read_write().has_read());
//     EXPECT_FALSE(AccessMode::write().has_read());
//     auto result = builder().open_existing(AccessMode::write());
//     EXPECT_TRUE(result.is_none());
//     EXPECT_EQ(errno, EACCES);
// }

} // namespace
} // namespace oxide::posix
