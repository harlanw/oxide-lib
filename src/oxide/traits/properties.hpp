#pragma once

#include <type_traits>

#include <oxide/num/types.hpp>
#include <oxide/traits/transformations.hpp>

namespace oxide {

template <typename... Traits>
using Conjunction = std::conjunction<Traits...>;

template <typename... Traits>
using Disjunction = std::disjunction<Traits...>;

template <typename Trait>
using Negation = std::negation<Trait>;

template <bool T>
using BoolConst = std::bool_constant<T>;

// Relational traits compare the first operand with every remaining operand.
template <typename T, typename... Us>
using IsSame = BoolConst<(std::is_same_v<T, Us> && ...)>;

template <typename T, typename... Us>
using IsDifferent = BoolConst<(!std::is_same_v<T, Us> && ...)>;

template <typename Base, typename... Derived>
using IsBaseOf = BoolConst<(std::is_base_of_v<Base, Derived> && ...)>;

template <typename From, typename... To>
using IsConvertible = BoolConst<(std::is_convertible_v<From, To> && ...)>;

template <typename T, typename... Ts>
using IsOneOf = BoolConst<(std::is_same_v<RemoveCv<T>, Ts> || ...)>;

// Property packs use conjunction; an empty pack satisfies the property.
template <typename... Ts>
using IsVoid = BoolConst<(std::is_void_v<Ts> && ...)>;

template <typename... Ts>
using IsNullPointer = BoolConst<(std::is_null_pointer_v<Ts> && ...)>;

template <typename... Ts>
using IsIntegral = BoolConst<
    (IsOneOf<Ts, u8, u16, u32, u64, i8, i16, i32, i64, usize, isize, uptr, iptr>::value && ...)>;

template <typename... Ts>
using IsFloatingPoint = BoolConst<(IsOneOf<Ts, f32, f64>::value && ...)>;

template <typename... Ts>
using IsArray = BoolConst<(std::is_array_v<Ts> && ...)>;

template <typename... Ts>
using IsEnum = BoolConst<(std::is_enum_v<Ts> && ...)>;

template <typename... Ts>
using IsUnion = BoolConst<(std::is_union_v<Ts> && ...)>;

template <typename... Ts>
using IsClass = BoolConst<(std::is_class_v<Ts> && ...)>;

template <typename... Ts>
using IsFunction = BoolConst<(std::is_function_v<Ts> && ...)>;

template <typename... Ts>
using IsPointer = BoolConst<(std::is_pointer_v<Ts> && ...)>;

template <typename... Ts>
using IsLvalueRef = BoolConst<(std::is_lvalue_reference_v<Ts> && ...)>;

template <typename... Ts>
using IsRvalueRef = BoolConst<(std::is_rvalue_reference_v<Ts> && ...)>;

template <typename... Ts>
using IsMemberObjectPointer = BoolConst<(std::is_member_object_pointer_v<Ts> && ...)>;

template <typename... Ts>
using IsMemberFunctionPointer = BoolConst<(std::is_member_function_pointer_v<Ts> && ...)>;

template <typename... Ts>
using IsFundamental = BoolConst<(std::is_fundamental_v<Ts> && ...)>;

template <typename... Ts>
using IsArithmetic = BoolConst<(std::is_arithmetic_v<Ts> && ...)>;

template <typename... Ts>
using IsScalar = BoolConst<(std::is_scalar_v<Ts> && ...)>;

template <typename... Ts>
using IsObject = BoolConst<(std::is_object_v<Ts> && ...)>;

template <typename... Ts>
using IsCompound = BoolConst<(std::is_compound_v<Ts> && ...)>;

template <typename... Ts>
using IsRef = BoolConst<(std::is_reference_v<Ts> && ...)>;

template <typename... Ts>
using IsMemberPointer = BoolConst<(std::is_member_pointer_v<Ts> && ...)>;

template <typename... Ts>
using IsConst = BoolConst<(std::is_const_v<Ts> && ...)>;

template <typename... Ts>
using IsVolatile = BoolConst<(std::is_volatile_v<Ts> && ...)>;

template <typename... Ts>
using IsSigned = BoolConst<(IsOneOf<Ts, i8, i16, i32, i64, isize, iptr>::value && ...)>;

template <typename... Ts>
using IsUnsigned = BoolConst<(IsOneOf<Ts, u8, u16, u32, u64, usize, uptr>::value && ...)>;

template <typename... Ts>
using IsTrivial = BoolConst<(std::is_trivial_v<Ts> && ...)>;

template <typename... Ts>
using IsStandardLayout = BoolConst<(std::is_standard_layout_v<Ts> && ...)>;

template <typename... Ts>
using HasUniqueObjectRepr = BoolConst<(std::has_unique_object_representations_v<Ts> && ...)>;

template <typename... Ts>
using IsEmpty = BoolConst<(std::is_empty_v<Ts> && ...)>;

template <typename... Ts>
using IsPolymorphic = BoolConst<(std::is_polymorphic_v<Ts> && ...)>;

template <typename... Ts>
using IsAbstract = BoolConst<(std::is_abstract_v<Ts> && ...)>;

template <typename... Ts>
using IsFinal = BoolConst<(std::is_final_v<Ts> && ...)>;

template <typename... Ts>
using IsAggregate = BoolConst<(std::is_aggregate_v<Ts> && ...)>;

template <typename... Ts>
using HasVirtualDestructor = BoolConst<(std::has_virtual_destructor_v<Ts> && ...)>;

template <typename T, typename... Args>
using IsConstructible = std::is_constructible<T, Args...>;

template <typename... Ts>
using IsDefaultConstructible = BoolConst<(std::is_default_constructible_v<Ts> && ...)>;

template <typename... Ts>
using IsCopyConstructible = BoolConst<(std::is_copy_constructible_v<Ts> && ...)>;

template <typename... Ts>
using IsMoveConstructible = BoolConst<(std::is_move_constructible_v<Ts> && ...)>;

template <typename T, typename... Us>
using IsAssignable = BoolConst<(std::is_assignable_v<T, Us> && ...)>;

template <typename... Ts>
using IsCopyAssignable = BoolConst<(std::is_copy_assignable_v<Ts> && ...)>;

template <typename... Ts>
using IsMoveAssignable = BoolConst<(std::is_move_assignable_v<Ts> && ...)>;

template <typename... Ts>
using IsCopyable = Conjunction<IsCopyConstructible<Ts...>, IsCopyAssignable<Ts...>>;

template <typename... Ts>
using IsMovable = Conjunction<IsMoveConstructible<Ts...>, IsMoveAssignable<Ts...>>;

template <typename... Ts>
using IsDestructible = BoolConst<(std::is_destructible_v<Ts> && ...)>;

template <typename T, typename... Args>
using IsNothrowConstructible = std::is_nothrow_constructible<T, Args...>;

template <typename... Ts>
using IsNothrowDefaultConstructible = BoolConst<
    (std::is_nothrow_default_constructible_v<Ts> && ...)>;

template <typename... Ts>
using IsNothrowCopyConstructible = BoolConst<(std::is_nothrow_copy_constructible_v<Ts> && ...)>;

template <typename... Ts>
using IsNothrowMoveConstructible = BoolConst<(std::is_nothrow_move_constructible_v<Ts> && ...)>;

template <typename T, typename... Us>
using IsNothrowAssignable = BoolConst<(std::is_nothrow_assignable_v<T, Us> && ...)>;

template <typename... Ts>
using IsNothrowCopyAssignable = BoolConst<(std::is_nothrow_copy_assignable_v<Ts> && ...)>;

template <typename... Ts>
using IsNothrowMoveAssignable = BoolConst<(std::is_nothrow_move_assignable_v<Ts> && ...)>;

template <typename... Ts>
using IsNothrowCopyable =
    Conjunction<IsNothrowCopyConstructible<Ts...>, IsNothrowCopyAssignable<Ts...>>;

template <typename... Ts>
using IsNothrowMovable =
    Conjunction<IsNothrowMoveConstructible<Ts...>, IsNothrowMoveAssignable<Ts...>>;

template <typename... Ts>
using IsNothrowDestructible = BoolConst<(std::is_nothrow_destructible_v<Ts> && ...)>;

template <typename T, typename... Args>
using IsTriviallyConstructible = std::is_trivially_constructible<T, Args...>;

template <typename... Ts>
using IsTriviallyDefaultConstructible = BoolConst<
    (std::is_trivially_default_constructible_v<Ts> && ...)>;

template <typename... Ts>
using IsTriviallyCopyConstructible = BoolConst<(std::is_trivially_copy_constructible_v<Ts> && ...)>;

template <typename... Ts>
using IsTriviallyMoveConstructible = BoolConst<(std::is_trivially_move_constructible_v<Ts> && ...)>;

template <typename T, typename... Us>
using IsTriviallyAssignable = BoolConst<(std::is_trivially_assignable_v<T, Us> && ...)>;

template <typename... Ts>
using IsTriviallyCopyAssignable = BoolConst<(std::is_trivially_copy_assignable_v<Ts> && ...)>;

template <typename... Ts>
using IsTriviallyMoveAssignable = BoolConst<(std::is_trivially_move_assignable_v<Ts> && ...)>;

template <typename... Ts>
using IsTriviallyCopyable =
    Conjunction<IsTriviallyCopyConstructible<Ts...>, IsTriviallyCopyAssignable<Ts...>>;

template <typename... Ts>
using IsTriviallyMovable =
    Conjunction<IsTriviallyMoveConstructible<Ts...>, IsTriviallyMoveAssignable<Ts...>>;

template <typename... Ts>
using IsTriviallyDestructible = BoolConst<(std::is_trivially_destructible_v<Ts> && ...)>;

template <typename F, typename... Args>
using IsInvocable = std::is_invocable<F, Args...>;

template <typename R, typename F, typename... Args>
using IsInvocableR = std::is_invocable_r<R, F, Args...>;

template <typename F, typename... Args>
using IsNothrowInvocable = std::is_nothrow_invocable<F, Args...>;

template <typename R, typename F, typename... Args>
using IsNothrowInvocableR = std::is_nothrow_invocable_r<R, F, Args...>;

template <typename... Ts>
using IsSwappable = BoolConst<(std::is_swappable_v<Ts> && ...)>;

template <typename T, typename... Us>
using IsSwappableWith = BoolConst<(std::is_swappable_with_v<T, Us> && ...)>;

template <typename... Ts>
using IsNothrowSwappable = BoolConst<(std::is_nothrow_swappable_v<Ts> && ...)>;

template <typename T, typename... Us>
using IsNothrowSwappableWith = BoolConst<(std::is_nothrow_swappable_with_v<T, Us> && ...)>;

template <typename T>
using AlignmentOf = std::alignment_of<T>;

template <typename T>
using Rank = std::rank<T>;

template <typename T, unsigned N = 0>
using Extent = std::extent<T, N>;

template <typename From, typename... To>
using IfConvertible = EnableIf<IsConvertible<From, To...>::value, int>;

template <typename... Ts>
using IfIntegral = EnableIf<IsIntegral<Ts...>::value, int>;

template <typename... Ts>
using IfUnsigned = EnableIf<IsUnsigned<Ts...>::value, int>;

template <typename... Ts>
using IfFloatingPoint = EnableIf<IsFloatingPoint<Ts...>::value, int>;

} // namespace oxide
