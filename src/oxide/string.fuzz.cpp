#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "string.test.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    constexpr std::size_t MaxLength = 4096;
    oxide::String actual;
    std::string model;
    std::size_t offset = 0;
    auto byte = [&]() -> std::size_t {
        return offset < size ? data[offset++] : 0;
    };

    while (offset < size) {
        switch (byte() % 11) {
            case 0:
                if (model.size() < MaxLength) {
                    const char value = static_cast<char>(byte());
                    actual.push(value);
                    model.push_back(value);
                }
                break;
            case 1: {
                const auto count = std::min({byte(), size - offset, MaxLength - model.size()});
                const auto* source = reinterpret_cast<const char*>(data + offset);
                actual.push_str(std::string_view(source, count));
                model.append(source, count);
                offset += count;
                break;
            }
            case 2: {
                auto popped = actual.pop();
                OXIDE_ASSERT(popped.is_some() == !model.empty());
                if (!model.empty()) {
                    OXIDE_ASSERT(popped.unwrap() == model.back());
                    model.pop_back();
                }
                break;
            }
            case 3: {
                const auto count = byte();
                actual.truncate(count);
                if (count < model.size()) {
                    model.resize(count);
                }
                break;
            }
            case 4: {
                const auto additional = byte();
                actual.reserve(additional);
                OXIDE_ASSERT(actual.capacity() >= model.size() + additional);
                break;
            }
            case 5: {
                const auto additional = byte();
                const auto old_capacity = actual.capacity();
                actual.reserve_exact(additional);
                OXIDE_ASSERT(
                    actual.capacity() == std::max(old_capacity, model.size() + additional)
                );
                break;
            }
            case 6:
                actual.shrink_to_fit();
                OXIDE_ASSERT(actual.capacity() == model.size());
                break;
            case 7: {
                const auto start = byte() % (model.size() + 1);
                const auto count = std::min(
                    {byte(), model.size() - start, MaxLength - model.size()}
                );
                const auto copy = model.substr(start, count);
                actual.push_str(std::string_view(actual.data() + start, count));
                model += copy;
                break;
            }
            case 8: {
                auto copy = actual.clone();
                oxide::tests::string::check(copy, model);
                oxide::String moved(std::move(copy));
                oxide::tests::string::check(copy, {});
                actual = std::move(moved);
                oxide::tests::string::check(moved, {});
                break;
            }
            case 9:
                actual.clear();
                model.clear();
                break;
            case 10:
                if (!model.empty()) {
                    const auto index = byte() % model.size();
                    const char value = static_cast<char>(byte());
                    oxide::AsStrMut(actual)[index] = value;
                    model[index] = value;
                }
                break;
        }
        oxide::tests::string::check(actual, model);
    }
    oxide::tests::string::check(actual, model);
    return 0;
}
