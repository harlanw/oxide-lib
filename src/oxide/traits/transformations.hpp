#pragma once

#include <type_traits>

namespace oxide {

template <typename T>
using RemoveConst = std::remove_const_t<T>;

template <typename T>
using RemoveVolatile = std::remove_volatile_t<T>;

template <typename T>
using RemoveCv = std::remove_cv_t<T>;

template <typename T>
using AddConst = std::add_const_t<T>;

template <typename T>
using AddVolatile = std::add_volatile_t<T>;

template <typename T>
using AddCv = std::add_cv_t<T>;

template <typename T>
using RemoveRef = std::remove_reference_t<T>;

template <typename T>
using RemoveCvRef = RemoveCv<RemoveRef<T>>;

template <typename T>
using AddLvalueRef = std::add_lvalue_reference_t<T>;

template <typename T>
using AddRvalueRef = std::add_rvalue_reference_t<T>;

template <typename T>
using RemovePtr = std::remove_pointer_t<T>;

template <typename T>
using AddPtr = std::add_pointer_t<T>;

template <typename T>
using RemoveExtent = std::remove_extent_t<T>;

template <typename T>
using RemoveAllExtents = std::remove_all_extents_t<T>;

template <typename T>
using MakeSigned = std::make_signed_t<T>;

template <typename T>
using MakeUnsigned = std::make_unsigned_t<T>;

template <bool Condition, typename T = void>
using EnableIf = std::enable_if_t<Condition, T>;

template <bool Condition, typename T, typename U>
using Conditional = std::conditional_t<Condition, T, U>;

template <typename... Ts>
using CommonType = std::common_type_t<Ts...>;

template <typename T>
using Decay = std::decay_t<T>;

template <typename T>
using UnderlyingType = std::underlying_type_t<T>;

template <typename F, typename... Args>
using InvokeResult = std::invoke_result_t<F, Args...>;

template <typename... Ts>
using Void = std::void_t<Ts...>;

template <typename T>
struct TypeIdentity {
    using Type = T;
};

template <typename T>
using TypeIdentityT = typename TypeIdentity<T>::Type;

template <typename T>
using UnderlyingType = std::underlying_type_t<T>;

} // namespace oxide
