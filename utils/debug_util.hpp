#pragma once

#include <list>

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

    auto traceList = getStackTrace(fp);

    for (auto address : traceList) {
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

}  // namespace dbgutil
