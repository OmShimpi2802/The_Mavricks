#include"blocklist.h"

std::unordered_set<std::string> blocklist{
"notepad.exe",
"keylogger.exe",
"suspicious.exe",
"Acrobat.exe"
};
std::queue<std::string> telemetryQueue;
std::mutex queueMutex;