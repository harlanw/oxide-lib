#pragma once

// clang-format off

#if defined(__COUNTER__)
    #define OXIDE_COUNTER __COUNTER__
#else
    #define OXIDE_COUNTER __LINE__
#endif

#if defined(__has_builtin)
    #define OXIDE_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define OXIDE_HAS_BUILTIN(x) 0
#endif

#if defined(__has_include)
    #define OXIDE_HAS_INCLUDE(x) __has_include(x)
#else
    #define OXIDE_HAS_INCLUDE(x) 0
#endif

#if defined(__has_attribute)
    #define OXIDE_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
    #define OXIDE_HAS_ATTRIBUTE(x) 0
#endif

#if defined(__has_cpp_attribute)
    #define OXIDE_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
    #define OXIDE_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if defined(__has_feature)
    #define OXIDE_HAS_FEATURE(x) __has_feature(x)
#else
    #define OXIDE_HAS_FEATURE(x) 0
#endif

// clang-format on
