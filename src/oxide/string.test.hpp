#pragma once

#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <oxide/c_str.hpp>
#include <oxide/string.hpp>

namespace oxide::tests::string {

template <template <typename> class Expression, typename T, typename = void>
struct IsValid : std::false_type {};

template <template <typename> class Expression, typename T>
struct IsValid<Expression, T, std::void_t<Expression<T>>> : std::true_type {};

template <typename T>
using Borrow = decltype(AsStr(std::declval<T>()));

template <typename T>
using BorrowMut = decltype(AsStrMut(std::declval<T>()));

template <typename T>
using Data = decltype(std::declval<T>().data());

template <typename T>
using View = decltype(std::declval<T>().as_str());

template <typename T>
using MutView = decltype(std::declval<T>().as_mut_str());

template <typename T>
using Iter = decltype(std::declval<T>().iter());

template <typename T>
using Index = decltype(std::declval<T>()[0]);

static_assert(!std::is_copy_constructible_v<String> && !std::is_copy_assignable_v<String>);
static_assert(std::is_nothrow_move_constructible_v<String>);
static_assert(std::is_nothrow_move_assignable_v<String>);
static_assert(IsValid<Borrow, String&>{} && IsValid<Borrow, const String&>{});
static_assert(!IsValid<Borrow, String&&>{} && !IsValid<Borrow, const String&&>{});
static_assert(IsValid<BorrowMut, String&>{} && !IsValid<BorrowMut, const String&>{});
static_assert(!IsValid<BorrowMut, String&&>{} && !IsValid<BorrowMut, const String&&>{});
static_assert(IsValid<Data, String&>{} && IsValid<Data, const String&>{});
static_assert(!IsValid<Data, String&&>{} && !IsValid<Data, const String&&>{});
static_assert(IsValid<View, const String&>{} && !IsValid<View, String&&>{});
static_assert(!IsValid<View, const String&&>{});
static_assert(IsValid<MutView, String&>{} && !IsValid<MutView, const String&>{});
static_assert(!IsValid<MutView, String&&>{});
static_assert(IsValid<Iter, const String&>{} && !IsValid<Iter, String&&>{});
static_assert(!IsValid<Iter, const String&&>{});
static_assert(IsValid<Index, const String&>{} && !IsValid<Index, String&&>{});
static_assert(!IsValid<Index, const String&&>{});
static_assert(std::is_same_v<Index<String&>, char&>);
static_assert(std::is_same_v<Index<const String&>, const char&>);
static_assert(std::is_same_v<decltype(Str::from_ref(std::declval<const char&>(), 0)), Str>);
static_assert(std::is_same_v<decltype(StrMut::from_ref(std::declval<char&>(), 0)), StrMut>);
static_assert(noexcept(Str::from_ref(std::declval<const char&>(), 0)));
static_assert(noexcept(StrMut::from_ref(std::declval<char&>(), 0)));

inline void check(const String& actual, std::string_view expected) {
    OXIDE_ASSERT(actual.len() == expected.size());
    OXIDE_ASSERT(actual.capacity() >= actual.len());
    OXIDE_ASSERT(actual.data() != nullptr);
    OXIDE_ASSERT(actual.is_empty() == expected.empty());
    if (!expected.empty()) {
        OXIDE_ASSERT(std::memcmp(actual.data(), expected.data(), expected.size()) == 0);
    }

    auto view = AsStr(actual);
    OXIDE_ASSERT(view.data() == actual.data() && view.len() == actual.len());
    usize index = 0;
    for (char byte : actual.iter()) {
        OXIDE_ASSERT(byte == expected[index++]);
    }
    OXIDE_ASSERT(index == expected.size());
}

inline void construction_and_views() {
    String empty;
    check(empty, {});
    OXIDE_ASSERT(empty.capacity() == 0 && empty.pop().is_none());
    OXIDE_ASSERT(empty.as_mut_str().iter().next().is_none());
    check(String(std::string_view{}), {});
    check(String::with_capacity(0), {});

    char source[] = "abc";
    String value(Str::from_ref(source[0], 3));
    source[0] = 'X';
    check(value, "abc");
    OXIDE_ASSERT(value.data() != source);
    OXIDE_ASSERT(value.as_mut_str().data() == value.data());
    AsStrMut(value)[1] = 'B';
    check(value, "aBc");
    check(String(std::string("temporary")), "temporary");
    check(String(AsStr(CStr::from_ptr("cstr").unwrap())), "cstr");

    auto reserved = String::with_capacity(16);
    check(reserved, {});
    OXIDE_ASSERT(reserved.as_mut_str().iter().next().is_none());
    auto pointer = reserved.data();
    reserved.push_str("abc");
    reserved.clear();
    check(reserved, {});
    OXIDE_ASSERT(reserved.data() == pointer);
    OXIDE_ASSERT(reserved.as_mut_str().iter().next().is_none());
}

inline void ownership() {
    String source("abc");
    auto copy = source.clone();
    OXIDE_ASSERT(source == copy && source.data() != copy.data());
    copy[0] = 'A';
    check(source, "abc");
    check(copy, "Abc");
    OXIDE_ASSERT(source != copy);

    auto pointer = source.data();
    const auto capacity = source.capacity();
    String moved(std::move(source));
    check(moved, "abc");
    check(source, {});
    OXIDE_ASSERT(moved.data() == pointer && moved.capacity() == capacity);
    OXIDE_ASSERT(source.capacity() == 0);
    source.push('z');
    check(source, "z");

    String target("previous allocation");
    target = std::move(moved);
    check(target, "abc");
    check(moved, {});
    OXIDE_ASSERT(target.data() == pointer && moved.capacity() == 0);
    target = target.clone();
    check(target, "abc");

    auto reserved = String::with_capacity(32);
    String transferred(std::move(reserved));
    check(reserved, {});
    check(transferred, {});
    OXIDE_ASSERT(reserved.capacity() == 0 && transferred.capacity() == 32);

    std::vector<String> owners;
    owners.push_back(String("one"));
    owners.push_back(String("two"));
    check(owners[0], "one");
    check(owners[1], "two");
}

inline void capacity_and_aliasing() {
    auto value = String::with_capacity(16);
    auto pointer = value.data();
    value.push_str("abc");
    value.push_str(value.as_str());
    check(value, "abcabc");
    value.push_str(std::string_view(value.data() + 1, 3));
    check(value, "abcabcbca");
    value.push('!');
    check(value, "abcabcbca!");
    OXIDE_ASSERT(value.data() == pointer && value.capacity() == 16);

    value.truncate(3);
    value.truncate(100);
    check(value, "abc");
    value.clear();
    OXIDE_ASSERT(value.capacity() == 16 && value.data() == pointer);
    value.push_str("reuse");
    check(value, "reuse");
    value.shrink_to_fit();
    check(value, "reuse");
    OXIDE_ASSERT(value.capacity() == value.len());
    value.clear();
    value.shrink_to_fit();
    check(value, {});
    OXIDE_ASSERT(value.capacity() == 0);

    String growing("abcd");
    growing.push_str(growing.as_str());
    check(growing, "abcdabcd");
    growing.push_str(std::string_view(growing.data() + 2, 4));
    check(growing, "abcdabcdcdab");

    String exact("abc");
    exact.reserve_exact(2);
    OXIDE_ASSERT(exact.capacity() == 5);
    exact.reserve(3);
    OXIDE_ASSERT(exact.capacity() >= exact.len() + 3);
    check(exact, "abc");
}

inline void bytes_and_empty_subviews() {
    const char bytes[] = {'a', '\0', static_cast<char>(0xff), 'b'};
    String value(std::string_view(bytes, sizeof(bytes)));
    check(value, std::string_view(bytes, sizeof(bytes)));
    OXIDE_ASSERT(value.pop().unwrap() == 'b');
    OXIDE_ASSERT(static_cast<unsigned char>(value.pop().unwrap()) == 0xff);
    auto nul = value.pop();
    OXIDE_ASSERT(nul.is_some() && nul.unwrap() == '\0');
    OXIDE_ASSERT(value.pop().unwrap() == 'a');
    OXIDE_ASSERT(value.pop().is_none());

    auto full = String::with_capacity(1);
    full.push('x');
    check(full, "x");
    OXIDE_ASSERT(full.len() == full.capacity());
    // An empty subview can be one-past-end, so it must not bind a reference.
    full.push_str(std::string_view(full.data() + full.len(), 0));
    check(full, "x");
}

inline void run() {
    construction_and_views();
    ownership();
    capacity_and_aliasing();
    bytes_and_empty_subviews();
}

} // namespace oxide::tests::string
