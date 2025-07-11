#include "pch.h"
#include "Minhook.h"
#include "ApiHooks.h"
#include <iostream>
#include <ostream>
#include <Windows.h>
#include <string>
#include "json.hpp"  
#include <psapi.h>
#include <winspool.h>
#include "Client.h"

#pragma comment(lib, "psapi.lib")

using json = nlohmann::json;
std::string GetCurrentProcessName() {
	char filename[MAX_PATH];
	if (GetModuleFileNameA(NULL, filename, MAX_PATH)) {
		// Extract only the executable name from the full path
		std::string fullPath(filename);
		size_t pos = fullPath.find_last_of("\\/");
		if (pos != std::string::npos)
			return fullPath.substr(pos + 1);
		else
			return fullPath; // In case there's no slash
	}
	return "Unknown";
}

HHOOK g_hKeyBoardHook = NULL;
typedef HANDLE(WINAPI* SetClipboardData_t)(UINT uFormat, HANDLE hMem);
typedef HANDLE(WINAPI* GetClipboardData_t)(UINT uFormat);
typedef HANDLE(WINAPI* EmptyClipboard_t)();
typedef BOOL(WINAPI* pStartDocPrinterW)(HANDLE, DWORD, LPBYTE);
typedef int(WINAPI* pStartDocW)(HDC, CONST DOCINFOW*);
typedef BOOL(WINAPI* pWriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);

SetClipboardData_t pSetClipboardData = nullptr;
GetClipboardData_t pGetClipboardData = nullptr;
EmptyClipboard_t pEmptyClipboard = nullptr;
pStartDocPrinterW fpStartDocPrinterW = nullptr;
pStartDocW fpStartDocW = nullptr;
pWriteFile fpWriteFile = nullptr;

std::string appname = GetCurrentProcessName();

HANDLE WINAPI mySetClipboardData(UINT uFormat, HANDLE hMem)
{
	//MessageBoxA(NULL, "Copy event", "heyy", MB_OK);
	std::string message = CreateTelemetryPacket(appname,"Copy");
	SendJsonToServer(message);
	//SendTelemetryToClient("copy");
	//MessageBoxA(NULL, "Copy event", "heyy", MB_OK);
	return pSetClipboardData(uFormat, hMem); //alow normally
}

HANDLE WINAPI myGetClipboardData(UINT uFormat)
{
	//MessageBoxA(NULL, "Paste event", "heyy", MB_OK);
	//SendTelemetryToClient("paste");
	std::string message = CreateTelemetryPacket(appname, "Paste");
	SendJsonToServer(message);
	//cout << message << endl;
	//SendJsonToServer(message);
	return pGetClipboardData(uFormat); //alow normally
}

BOOL WINAPI myStartDocPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo) {
	std::string message = CreateTelemetryPacket(appname, "Print");
	SendJsonToServer(message);
	return fpStartDocPrinterW(hPrinter, Level, pDocInfo);
}

// Hooked StartDocW
int WINAPI myStartDocW(HDC hdc, CONST DOCINFOW* lpdi) {
	std::string message = CreateTelemetryPacket(appname, "Print");
	SendJsonToServer(message);
	return fpStartDocW(hdc, lpdi);
}


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
	{
		KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

		if (p->vkCode == VK_SNAPSHOT)
		{
			//MessageBoxA(NULL, "Snapshot event", "heyy", MB_OK);
			std::string message = CreateTelemetryPacket(appname, "Snapshot");
			SendJsonToServer(message);
			//SendTelemetryToClient("screenshot");
		}
	}
	return CallNextHookEx(g_hKeyBoardHook, nCode, wParam, lParam);
}

// Thread to run the message loop, allowing the hook to work
DWORD WINAPI KeyboardHookThread(LPVOID) {
	g_hKeyBoardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void InstallClipboardHook()
{
	HMODULE hUser32 = GetModuleHandleA("user32.dll");
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (hUser32 && hKernel32)
	{
		void* target1 = GetProcAddress(hUser32, "SetClipboardData");
		void* target2 = GetProcAddress(hUser32, "GetClipboardData");
		if (target1 && target2 )
		{
			MH_CreateHook(target1, &mySetClipboardData, reinterpret_cast<void**>(&pSetClipboardData));
			MH_EnableHook(target1);
			MH_CreateHook(target2, &myGetClipboardData, reinterpret_cast<void**>(&pGetClipboardData));
			MH_EnableHook(target2);
			// Start the hook in a new thread to keep the message loop running
			CreateThread(NULL, 0, KeyboardHookThread, NULL, 0, NULL);
		}
		else
		{
			std::cout << "Error occured on InstallClipboard Hook error no :" << GetLastError() << std::endl;
			MessageBoxA(NULL, "something failed!", "Ozone Alert!...", MB_ICONWARNING);
		}
	}
	else
	{
		MessageBoxA(NULL, "Kernel32 open  failed!", "Ozone Alert!...", MB_ICONWARNING);
	}
}



void RemoveClipboardHook()
{
	HMODULE hUser32 = GetModuleHandleA("user32.dll");
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (hUser32 && hKernel32)
	{
		void* target1 = GetProcAddress(hUser32, "SetClipboardData");
		void* target2 = GetProcAddress(hUser32, "GetClipboardData");
		void* target3 = GetProcAddress(hUser32, "EmptyClipboard");
		void* target4 = GetProcAddress(hKernel32, "CreateFileW");
		if (target1 && target2 && target3 && target4)
		{
			MH_DisableHook(target1);
			MH_RemoveHook(target1);
			MH_DisableHook(target2);
			MH_RemoveHook(target2);
			MH_DisableHook(target3);
			MH_RemoveHook(target3);
			MH_DisableHook(target4);
			MH_RemoveHook(target4);
			// Unhook PrintScreen blocker
			if (g_hKeyBoardHook) {
				UnhookWindowsHookEx(g_hKeyBoardHook);
				g_hKeyBoardHook = NULL;
			}

		}
		else
		{
			std::cout << "Error occured on RemoveClipboard Hook error no :" << GetLastError() << std::endl;
		}
	}
}