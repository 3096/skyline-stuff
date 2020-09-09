#pragma once

#include <type_traits>

#include "nn/ro.h"
#include "skyline/efl/service.hpp"
#include "skyline/inlinehook/And64InlineHook.hpp"
#include "skyline/nx/arm/counter.h"

namespace util {

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#if NOLOG
#    define LOG(...) ({})

#else

#    if USE_EFL_LOG
#        define EFL_LOG_BUFFER_SIZE 0x1000
static auto s_eflLogBuffer = std::array<char, EFL_LOG_BUFFER_SIZE>{0};

#        define LOG(fmt, ...)                                                            \
            snprintf(util::s_eflLogBuffer.data(), EFL_LOG_BUFFER_SIZE - 1, "[%s]: " fmt, \
                     __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__);                    \
            skyline::efl::Log("...", EFL_LOG_LEVEL_INFO, util::s_eflLogBuffer.data());

#    else
#        define LOG(fmt, ...) \
            skyline::logger::s_Instance->LogFormat("[%s]: " fmt, __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__);

#    endif
#endif

#define GENERATE_SYM_HOOK(name, symbolStr, ReturnType, ...)                                  \
    ReturnType (*name##Bak)(__VA_OPT__(__VA_ARGS__));                                        \
    ReturnType name##Replace(__VA_OPT__(__VA_ARGS__));                                       \
    void name##Hook() {                                                                      \
        uintptr_t symbolAddress;                                                             \
        if (R_SUCCEEDED(nn::ro::LookupSymbol(&symbolAddress, symbolStr))) {                  \
            LOG("hooking %s...", STRINGIFY(name));                                           \
            A64HookFunction((void*)symbolAddress, (void*)name##Replace, (void**)&name##Bak); \
        } else {                                                                             \
            LOG("failed to look up %s, symbol is: %s", STRINGIFY(name), symbolStr);          \
        }                                                                                    \
    }                                                                                        \
    ReturnType name##Replace(__VA_OPT__(__VA_ARGS__))

#define CLASS_METHOD_RETURN_TYPE(ClassName, methodName, ...) \
    std::result_of<decltype (&ClassName::methodName)(ClassName __VA_OPT__(, __VA_ARGS__))>::type

#define GENERATE_CLASS_HOOK(ClassName, methodName, ...)                                                      \
    CLASS_METHOD_RETURN_TYPE(ClassName, methodName __VA_OPT__(, __VA_ARGS__))                                \
    (*methodName##Bak)(ClassName * __VA_OPT__(, __VA_ARGS__));                                               \
                                                                                                             \
    CLASS_METHOD_RETURN_TYPE(ClassName, methodName __VA_OPT__(, __VA_ARGS__))                                \
    methodName##Replace(ClassName* p_this __VA_OPT__(, __VA_ARGS__));                                        \
    void methodName##Hook() {                                                                                \
        LOG("hooking %s::%s...", STRINGIFY(ClassName), STRINGIFY(methodName));                               \
        auto methodName##Addr = &ClassName::methodName;                                                      \
        A64HookFunction(*(void**)(&methodName##Addr), (void*)methodName##Replace, (void**)&methodName##Bak); \
    }                                                                                                        \
                                                                                                             \
    CLASS_METHOD_RETURN_TYPE(ClassName, methodName __VA_OPT__(, __VA_ARGS__))                                \
    methodName##Replace(ClassName* p_this __VA_OPT__(, __VA_ARGS__))

#define GENERATE_CLASS_HOOK_NAMED(hookName, ClassName, methodName, ReturnType, ...)                       \
    ReturnType (*hookName##Bak)(ClassName * __VA_OPT__(, __VA_ARGS__));                                   \
    ReturnType hookName##Replace(ClassName* p_this __VA_OPT__(, __VA_ARGS__));                            \
    void hookName##Hook() {                                                                               \
        LOG("hooking %s::%s to %s...", STRINGIFY(ClassName), STRINGIFY(methodName), STRINGIFY(hookName)); \
        ReturnType (ClassName::*hookName##Addr)(__VA_ARGS__) = &ClassName::methodName;                    \
        A64HookFunction(*(void**)&hookName##Addr, (void*)hookName##Replace, (void**)&hookName##Bak);      \
    }                                                                                                     \
    ReturnType hookName##Replace(ClassName* p_this __VA_OPT__(, __VA_ARGS__))

class FpsLogger {
    uint m_frameCount;
    uint64_t m_lastSecondTick;

   public:
    FpsLogger() : m_lastSecondTick(armGetSystemTick()) {}

    void tick() {
        auto curTick = armGetSystemTick();
        if (curTick - m_lastSecondTick > armGetSystemTickFreq()) {
            LOG("%d fps", m_frameCount);
            m_frameCount = 0;
            m_lastSecondTick = armGetSystemTick();
        } else {
            m_frameCount++;
        }
    }
};
#define LOG_FPS                                  \
    static auto s_fpsLogger = util::FpsLogger{}; \
    s_fpsLogger.tick();

}  // namespace util
