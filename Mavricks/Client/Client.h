#include <winsock2.h>
#include <iphlpapi.h>
#include <Windows.h>
#include <string>
#include <queue>
#include <mutex>
#include <ctime>
#include <thread>
#include <iostream>
#include <sstream>
#include <wininet.h>
#include "json.hpp"
#include "blocklist.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")

std::queue<std::string> telemetryQueue;
std::mutex queueMutex;


std::string GetCurrentTimestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[64];
    localtime_s(&tstruct, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tstruct);
    return std::string(buf);
}

std::string GetUsername() {
    char username[256];
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size))
        return std::string(username);
    return "Unknown";
}

std::string GetLocalIP() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return "0.0.0.0";
    }

    // Get buffer size
    ULONG bufferSize = 0;
    GetAdaptersInfo(nullptr, &bufferSize); // First call to get required buffer size

    // Allocate memory
    IP_ADAPTER_INFO* adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    if (!adapterInfo) {
        std::cerr << "Memory allocation failed.\n";
        WSACleanup();
        return "0.0.0.0";
    }

    // Get adapter info
    if (GetAdaptersInfo(adapterInfo, &bufferSize) != NO_ERROR) {
        std::cerr << "GetAdaptersInfo failed.\n";
        free(adapterInfo);
        WSACleanup();
        return "0.0.0.0";
    }

    // Loop through all adapters
    IP_ADAPTER_INFO* currentAdapter = adapterInfo;
    while (currentAdapter) {
        std::string ip = currentAdapter->IpAddressList.IpAddress.String;
        if (ip != "0.0.0.0")
        {
            /*std::cout << "Adapter Name: " << currentAdapter->AdapterName << "\n";
            std::cout << "  Description: " << currentAdapter->Description << "\n";
            std::cout << "  IP Address: " << currentAdapter->IpAddressList.IpAddress.String << "\n";
            std::cout << "  Subnet Mask: " << currentAdapter->IpAddressList.IpMask.String << "\n\n";*/
            return currentAdapter->IpAddressList.IpAddress.String;
        }
        currentAdapter = currentAdapter->Next;
    }

    // Cleanup
    free(adapterInfo);
    WSACleanup();
    return"0.0.0.0";
}

std::string GetSafety(const std::string& opType)
{
    if (blocklist.count(opType) > 0)
    {
        return "false";
    }
    else
    {
        return "true";
    }
}

std::string CreateTelemetryPacket(const std::string& opType) {
    nlohmann::json j;
    j["operation_type"] = opType;
    j["timestamp"] = GetCurrentTimestamp();
    j["user"] = GetUsername();
    j["ip"] = GetLocalIP();
    j["safe"] = GetSafety(opType);

    return j.dump(); 
}

void SimulateProcessOpenEvent(const std::string& operation) {
    std::string jsonPayload = CreateTelemetryPacket(operation);

    std::lock_guard<std::mutex> lock(queueMutex);
    telemetryQueue.push(jsonPayload);

    std::cout << "Event captured and added to queue:\n" << jsonPayload << "\n";
}

bool SendJsonToServer(const std::string& jsonData) {
    HINTERNET hInternet = InternetOpenA("TelemetryClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        std::cerr << "[-] InternetOpenA failed\n";
        return false;
    }

    HINTERNET hConnect = InternetConnectA(hInternet, "localhost", 8080,
        NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        std::cerr << "[-] InternetConnectA failed\n";
        InternetCloseHandle(hInternet);
        return false;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/telemetry", NULL,
        NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest) {
        std::cerr << "[-] HttpOpenRequestA failed\n";
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    const char* headers = "Content-Type: application/json\r\n";
    BOOL result = HttpSendRequestA(hRequest, headers, -1L, (LPVOID)jsonData.c_str(), jsonData.length());

    if (!result) {
        std::cerr << "[-] HttpSendRequestA failed. Error: " << GetLastError() << "\n";
    }
    else {
        std::cout << "[+] Sent to server:\n" << jsonData << "\n";
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return result;
}

void BackgroundSender() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::lock_guard<std::mutex> lock(queueMutex);
        if (!telemetryQueue.empty()) {
            std::string jsonPayload = telemetryQueue.front();
            telemetryQueue.pop();

            bool success = SendJsonToServer(jsonPayload);
            if (!success) {
                std::cerr << "[-] Failed to send telemetry. Consider retry/requeue.\n";
                telemetryQueue.push(jsonPayload);
                // push it back to queue
            }
        }
    }
}


//old json format...
//std::string CreateTelemetryPacket(const std::string& opType) {
//    std::ostringstream oss;
//    oss << "{\n"
//        << "  \"operation_type\": \"" << opType << "\",\n"
//        << "  \"timestamp\": \"" << GetCurrentTimestamp() << "\",\n"
//        << "  \"user\": \"" << GetUsername() << "\",\n"
//        << "  \"ip\": \"" << GetLocalIP() << "\"\n"
//        << "}";
//    return oss.str();
//
//} //old json format
//old get ip
//std::string GetLocalIP() {
//    PIP_ADAPTER_INFO pAdapterInfo;
//    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
//    ULONG buflen = sizeof(IP_ADAPTER_INFO);
//
//    if (pAdapterInfo == NULL)
//    {
//        std::cout << "PAdapterInfo is null pointer ..." << GetLastError() << std::endl;
//        MessageBoxA(NULL, "Hello something failed", "error bro", MB_OK);
//    }
//
//    if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
//        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(buflen);
//    }
//
//    if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
//        std::string ip = pAdapterInfo->IpAddressList.IpAddress.String;
//        free(pAdapterInfo);
//        return ip;
//    }
//
//    return "0.0.0.0";
//}