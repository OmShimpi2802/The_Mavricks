#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
using namespace std;

inline DWORD FindProcessId(const wstring ProcessName)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    DWORD pid = 0;

    if (Process32First(snapshot, &entry)) {
        do {
            if (!_wcsicmp(entry.szExeFile, ProcessName.c_str())) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pid;

}

inline bool InjectDLL(DWORD pid, const string& dllpath)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        std::cerr << "Failed to open process.\n";
        return false;
    }
    else
        std::cout << " OpenProcess Success." << std::endl;
    HMODULE hModule = GetModuleHandleA("kernel32.dll");
    FARPROC lib = GetProcAddress(hModule, "LoadLibraryA");
    if (!lib) {
        std::cerr << "Failed to get process address.\n";
        return false;
    }
    else
        std::cout << " GetProcAddress Success." << std::endl;

    LPVOID allocMem = VirtualAllocEx(hProc, NULL, dllpath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocMem) {
        std::cerr << "Failed to allocate memory in target process.\n";
        CloseHandle(hProc);
        return false;
    }
    else
        std::cout << " Virtualallocex Success." << std::endl;

    BOOL wpm = WriteProcessMemory(hProc, allocMem, dllpath.c_str(), dllpath.size() + 1, NULL);
    if (!wpm) {
        std::cerr << "WriteProcessMemory failed.\n";
        VirtualFreeEx(hProc, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }
    else
        std::cout << " Writeprocessmemory Success." << std::endl;
    //(LPTHREAD_START_ROUTINE)lib
    HANDLE hThread = CreateRemoteThread(hProc, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(lib), allocMem, 0, 0);
    if (!hThread) {
        std::cerr << "CreateRemoteThread failed.\n";
        VirtualFreeEx(hProc, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }
    else
    {
        cout << "CreateRemotethread success." << endl;
        //MessageBoxA(NULL, "CreateProcess success", "Hurray", MB_OK);
    }

    std::cout << "Remote thread successfully created." << std::endl;
    std::cout << "process ID = " << pid << std::endl;

    WaitForSingleObject(hThread, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    if (exitCode != 0)
        std::cout << "DLL loaded successfully." << std::endl;
    else
        std::cout << "DLL load failed." << std::endl;

    VirtualFreeEx(hProc, allocMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProc);
    return true;
}


inline void Injectnow(wstring targetprocess)
{
    //wstring targetprocess = L"notepad.exe";
    string dllpath = "D:\\system_programming\\NewHackathon\\mavricksdll\\x64\\Debug\\mavricksdll.dll";//dll path...

    DWORD pid = FindProcessId(targetprocess);
    if (pid == 0)
    {
        cout << "The application is not running..." << endl;
        return;
    }
    else
    {
        cout << "The application is running and pid generated..." << endl;
    }
    cout << pid << endl;
    if (InjectDLL(pid, dllpath))
    {
        cout << "Dll injected successfully..." << endl;
    }
    else
    {
        cout << "DLL injection failed..." << endl;
    }
}


