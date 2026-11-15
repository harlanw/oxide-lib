#include <oxide/mem/scope.hpp>
#include <oxide/posix/shared_memory.hpp>

namespace oxide::posix {

[[nodiscard]] Option<SharedMemory> SharedMemory::open(detail::SharedMemoryConfig config) {
    const bool creating = config.creation_policy.is_some()
        && *config.creation_policy != CreationPolicy::open_only();
    if (auto error = config.is_valid(creating); error.is_some()) {
        return WITH_ERRNO(error->to_errno(), NONE);
    }

    OwnedFd fd;
    bool file_created{false};

    if (config.creation_policy.is_none()) {
        fd = posix::shm_open(config.name.as_cstr(), config.access_mode.oflags(), config.mode);
    } else {
        switch (config.creation_policy->match()) {
            case CreationPolicy::CREATE_ONLY:
                fd = create_new(config);
                file_created = true;
                break;
            case CreationPolicy::OPEN_ONLY:
                fd = posix::
                    shm_open(config.name.as_cstr(), config.access_mode.oflags(), config.mode);
                break;
            case CreationPolicy::OPEN_OR_CREATE:
                fd = create_new(config);
                if (fd.is_valid()) {
                    file_created = true;
                } else if (Errno::last().exists()) {
                    fd = posix::
                        shm_open(config.name.as_cstr(), config.access_mode.oflags(), config.mode);
                }
                break;
        }
    }

    if (fd.is_invalid()) {
        return NONE;
    }

    // Roll back only objects created by this call, even when ownership on success
    // is disabled. Preserve the original error across cleanup.
    auto rollback = mem::defer([&]() {
        const auto previous_errno = Errno::last();
        if (file_created) {
            hints::discard(posix::shm_unlink(config.name.as_cstr()));
        }
        Errno::set(previous_errno);
    });

    if (file_created) {
        if (posix::ftruncate(fd, config.size) == -1) {
            return NONE;
        }
    }

    auto metadata = Metadata::from_fd(fd);
    if (metadata.is_none()) {
        return NONE;
    }

    if (metadata->size > as<u64>(std::numeric_limits<off_t>::max())
        || metadata->size > std::numeric_limits<usize>::max()) {
        return WITH_ERRNO(Errno::RANGE, NONE);
    }

    if (metadata->size == 0 || config.mapping_offset >= metadata->size) {
        return WITH_ERRNO(Errno::RANGE, NONE);
    }

    // If the file was not created, we need to check if the requested size
    // is greater than the file size.
    if (!file_created && config.size != 0 && config.size > metadata->size) {
        return WITH_ERRNO(Errno::RANGE, NONE);
    }

    const usize mapping_size = metadata->size - config.mapping_offset;
    auto mapping = MemoryMapping::from_fd(
        MemoryMappingSettings{
            MappingProtection::from(config.access_mode),
            MappingBehavior::SHARED,
            MappingFlags::none(),
            config.address_hint.unwrap_or(nullptr),
            mapping_size,
            config.mapping_offset,
            config.address_hint.is_some(),
        },
        std::move(fd)
    );

    if (mapping.is_err()) {
        return WITH_ERRNO(mapping.unwrap_err().to_errno(), NONE);
    }

    Option<MemoryLock> memory_lock;
    if (config.memory_locked) {
        auto result = MemoryLock::create(mapping.unwrap().base_address(), mapping_size);
        if (result.is_err()) {
            return WITH_ERRNO(result.unwrap_err(), NONE);
        }
        memory_lock = std::move(result).unwrap();
    }

    if (file_created && config.zero_memory) {
        std::memset(mapping.unwrap().base_address_mut(), 0, mapping_size);
    }

    SharedMemory result{
        std::move(config.name),
        file_created && config.has_ownership,
        std::move(mapping).unwrap(),
        std::move(memory_lock),
        config.mapping_offset
    };
    rollback.cancel();

    return result;
}

} // namespace oxide::posix
