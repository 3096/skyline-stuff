#pragma once

#include <list>
#include <unordered_map>

#include "nn/hid.hpp"
#include "skyline/logger/Logger.hpp"
#include "skyline/utils/cpputils.hpp"
#include "util.hpp"

namespace dbgutil {

constexpr size_t TEXT_OFFSET = 0x7100000000;

constexpr size_t MAX_TRACE_SIZE = 0x20;

struct StackFrame {
    StackFrame* p_nextFrame;
    void* lr;
};

auto getStackTrace(StackFrame* p_stackFrame) {
    std::list<void*> result;

    StackFrame* p_cur_stackFrame = p_stackFrame;
    for (size_t cur_idx = 0; cur_idx < MAX_TRACE_SIZE; cur_idx++) {
        /* Validate the current frame. */
        if (p_cur_stackFrame == nullptr || ((size_t)p_cur_stackFrame % sizeof(void*) != 0)) {
            break;
        }

        result.push_back(p_cur_stackFrame->lr);

        /* Advance to the next frame. */
        p_cur_stackFrame = p_cur_stackFrame->p_nextFrame;
    }

    return result;
}

void logStackTrace() {
    StackFrame* fp;
    asm("mov %[result], FP" : [ result ] "=r"(fp));

    void* lr;
    asm("mov %[result], LR" : [ result ] "=r"(lr));
    LOG("LR is %lx", (size_t)lr - skyline::utils::g_MainTextAddr + TEXT_OFFSET);

    for (auto address : getStackTrace(fp)) {
        if (address) {
            if ((size_t)address > skyline::utils::g_MainTextAddr) {
                LOG("%lx", (size_t)address - skyline::utils::g_MainTextAddr + TEXT_OFFSET);
            } else {
                LOG("main-%lx", skyline::utils::g_MainTextAddr - (size_t)address);
            }
        }
    }
}

auto getFirstReturn() {
    StackFrame* fp;
    asm("mov %[result], FP" : [ result ] "=r"(fp));

    return getStackTrace(fp).front();
}

void logRegistersX(InlineCtx* ctx) {
    constexpr auto REGISTER_COUNT = sizeof(ctx->registers) / sizeof(ctx->registers[0]);

    for (auto i = 0u; i < REGISTER_COUNT; i++) {
        LOG("X%d: %lx", i, ctx->registers[i].x);
    }
}

void poorPersonsBreakpoint(std::string msg) {
#ifdef NOLOG
    return;
#endif

    constexpr auto CONTINUE_ONCE_KEY = nn::hid::KEY_ZR;
    constexpr auto CONTINUE_HOLD_KEY = nn::hid::KEY_ZL;

    static auto justContinued = false;
    static auto npadFullKeyState = nn::hid::NpadFullKeyState{};

    LOG("Breakpoint reached: %s", msg.c_str());

    while (true) {
        nn::hid::GetNpadState(&npadFullKeyState, nn::hid::CONTROLLER_PLAYER_1);

        if (npadFullKeyState.Buttons & CONTINUE_HOLD_KEY) {
            LOG("Breakpoint ignored");
            break;
        }

        if (npadFullKeyState.Buttons & CONTINUE_ONCE_KEY) {
            if (not justContinued) {
                LOG("Breakpoint continued");
                justContinued = true;
                break;
            }
        } else if (justContinued) {
            justContinued = false;
        }

        svcSleepThread(1000000000);
    }
}

// file watch
static std::unordered_map<void*, std::string> s_fileWatchMap = {};

void addFileHandleToWatch(nn::fs::FileHandle fileHandle, const char* path) {
    s_fileWatchMap[fileHandle.handle] = path;
}

void removeFileHandleFromWatch(nn::fs::FileHandle fileHandle) { s_fileWatchMap.erase(fileHandle.handle); }

auto handleIsWatched(nn::fs::FileHandle fileHandle) {
    return s_fileWatchMap.find(fileHandle.handle) != end(s_fileWatchMap);
}

auto getHandlePath(nn::fs::FileHandle fileHandle) { return s_fileWatchMap[fileHandle.handle]; }

}  // namespace dbgutil
