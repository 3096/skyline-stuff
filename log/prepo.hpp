#pragma once

#include "../utils/util.hpp"
#include "nn/fs.h"
#include "nn/prepo.h"
#include "skyline/utils/cpputils.hpp"

namespace log {

// prepo logging
void logPrepo(nn::prepo::PlayReport* p_reportObj) {
    nn::fs::CreateDirectory("sd:/prepo");
    nn::fs::CreateDirectory("sd:/prepo/xde");
    nn::time::PosixTime time;
    nn::time::StandardUserSystemClock::GetCurrentTime(&time);
    std::string reportOutputPath = "sd:/prepo/xde/";
    reportOutputPath += std::to_string(time.time) + "_";
    reportOutputPath += std::to_string(svcGetSystemTick()) + "_";
    reportOutputPath += std::string((char*)&p_reportObj->m_EventName) + ".bin";

    skyline::utils::writeFile(reportOutputPath.c_str(), 0, p_reportObj->m_Buff, p_reportObj->m_End);
    LOG("Logged report: %s", p_reportObj->m_EventName);
}

Result (*nnPrepoSaveBak)(nn::prepo::PlayReport*);
Result handleNnPrepoSave(nn::prepo::PlayReport* p_reportObj) {
    logPrepo(p_reportObj);
    return nnPrepoSaveBak(p_reportObj);
}

Result (*nnPrepoSaveWUidBak)(nn::prepo::PlayReport*, nn::account::Uid const&);
Result handleNnPrepoSaveWUid(nn::prepo::PlayReport* p_reportObj, nn::account::Uid const& acc) {
    logPrepo(p_reportObj);
    return nnPrepoSaveWUidBak(p_reportObj, acc);
}

void prepoSaveHook() {
    LOG("hooking prepoSave...");
    Result (nn::prepo::PlayReport::*saveAddr)() = &nn::prepo::PlayReport::Save;
    Result (nn::prepo::PlayReport::*saveWUidAddr)(nn::account::Uid const&) = &nn::prepo::PlayReport::Save;
    A64HookFunction(*(void**)&saveAddr, (void*)handleNnPrepoSaveWUid, (void**)&nnPrepoSaveBak);
    A64HookFunction(*(void**)&saveWUidAddr, (void*)handleNnPrepoSaveWUid, (void**)&nnPrepoSaveWUidBak);
}

}  // namespace log
