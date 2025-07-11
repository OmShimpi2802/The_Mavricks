#include "Event.h"


int main() {
    std::wcout << L"Process monitor started...\n";
    std::cout << "[Telemetry Client] Starting...\n";
    std::thread senderThread(BackgroundSender); // start background sender
    std::thread sysThread(CheckForSysInfo);
    while (true) {
        CheckForNewProcesses();
        Sleep(2000); // Wait 2 seconds
    }

    senderThread.join(); 
    sysThread.join();
    // Wait for thread (or you can make it detach in real app)
    return 0;
}
