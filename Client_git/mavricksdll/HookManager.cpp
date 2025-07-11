#include "pch.h"
#include "HookManager.h"
#include "ApiHooks.h"
#include "MinHook.h"
#include <iostream>

void Hookmanager::InitializeHook()
{
	//const std::string Configpath = "D:\\DLP-Notepad-Protector\\HookingEngine\\config.json";
	//Policymanager::LoadPolicy(Configpath); // Set correct path
	//Logger::Init("D:\\DLP-Notepad-Protector\\dlp_log.txt");
	MH_Initialize();
	InstallClipboardHook();
}

void Hookmanager::RemoveHook()
{
	RemoveClipboardHook();
	MH_Uninitialize();
}