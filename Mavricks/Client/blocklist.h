#pragma once
#include <unordered_set>
#include <string>

extern std::unordered_set<std::string> blocklist{
    "notepad.exe",
    "keylogger.exe",
    "suspicious.exe",
    "Acrobat.exe"
};