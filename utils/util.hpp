#pragma once

#include "nn/ro.h"
#include "skyline/inlinehook/And64InlineHook.hpp"

namespace util {

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#if NOLOG || __INTELLISENSE__
#    define LOG(...) ({})
#else
#    define LOG(fmt, ...) \
        skyline::logger::s_Instance->LogFormat("[%s]: " fmt, __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__);
#endif

#define GENERATE_SYM_HOOK(name, symbolStr, ReturnType, ...)                                  \
    ReturnType (*name##Bak)(__VA_ARGS__);                                                    \
    ReturnType name##Replace(__VA_ARGS__);                                                   \
    void name##Hook() {                                                                      \
        uintptr_t symbolAddress;                                                             \
        if (R_SUCCEEDED(nn::ro::LookupSymbol(&symbolAddress, symbolStr))) {                  \
            LOG("hooking %s...", STRINGIFY(name));                                           \
            A64HookFunction((void*)symbolAddress, (void*)name##Replace, (void**)&name##Bak); \
        } else {                                                                             \
            LOG("failed to look up %s, symbol is: %s", STRINGIFY(name), symbolStr);          \
        }                                                                                    \
    }                                                                                        \
    ReturnType name##Replace(__VA_ARGS__)

}  // namespace util
