#include "pch.h"
#include"blocklist.h"

std::unordered_set<std::string> blocklist{
"notepad.exe",
"chrome.exe",
"suspicious.exe",
"malicious.exe"
};
std::queue<std::string> telemetryQueue;
std::mutex queueMutex;