#pragma once
#include <unordered_set>
#include <queue>
#include <string>
#include <mutex>

extern std::unordered_set<std::string>blocklist;
extern std::queue<std::string> telemetryQueue;
extern std::mutex queueMutex;