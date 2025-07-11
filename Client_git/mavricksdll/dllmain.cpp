// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include "HookManager.h"

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    Hookmanager::InitializeHook();
    //MessageBoxA(NULL, "DLL Injected!", "Status", MB_OK);
    // Keep the thread alive — hook stays active
    while (true) {
        Sleep(1000);
    }

    return 0;
}

// NamedPipeClient.cpp
#include <windows.h>
#include <string>

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);//
        CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        Hookmanager::RemoveHook();
        break;
    }
    return TRUE;
}

