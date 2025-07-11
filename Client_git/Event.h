#pragma once
#include "Client.h"
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <codecvt>
#include <thread>
#include "blocklist.h"

std::vector<std::wstring> previousProcesses;
bool isFirstCycle = true;

inline std::string WStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

inline std::vector<std::wstring> GetProcessList() {
    std::vector<std::wstring> processList;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return processList;

    if (Process32First(snapshot, &entry)) {
        do {
            processList.push_back(entry.szExeFile);
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return processList;
}

inline void CheckForNewProcesses() {
    auto currentProcesses = GetProcessList();
    // if want to initialize from start turn this off(comment it)
    if (isFirstCycle) {
        previousProcesses = currentProcesses;
        isFirstCycle = false;
        return;
    }
    for (const auto& proc : currentProcesses) {
        if (std::find(previousProcesses.begin(), previousProcesses.end(), proc) == previousProcesses.end()) {
            // New process detected!
            std::wcout << L"[+] New Process: " << proc << std::endl;
            std::string procName = WStringToString(proc);
            SimulateProcessOpenEvent(procName);
        }
    }
    previousProcesses = currentProcesses;
}

