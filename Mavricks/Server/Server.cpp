#include "httplib.h"
#include <fstream>
#include <iostream>

void SaveToLogFile(const std::string& json) {
    std::ofstream file("telemetry_log.txt", std::ios::app); // append mode
    if (file.is_open()) {
        std::cout << "Writting into log :" << std::endl;
        file << json << "\n";
        file.close();
    }
    else {
        std::cerr << "Error: Could not open telemetry_log.txt for writing.\n";
    }
}
int main() {
    httplib::Server svr;

    svr.Post("/telemetry", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "Received Telemetry:\n" << req.body << std::endl;
        std::ofstream log("telemetry_log.jsonl", std::ios::app);
        log << req.body << std::endl;
        log.close();
        res.set_content("Telemetry received\n", "text/plain");
        });
        
    // Serve the HTML dashboard
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file("index.html");
        std::stringstream buffer;
        buffer << file.rdbuf();
        res.set_content(buffer.str(), "text/html");
        });

    // Serve the log file content
    svr.Get("/data", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream log("telemetry_log.jsonl");
        std::stringstream buffer;
        buffer << log.rdbuf();
        res.set_content(buffer.str(), "application/json");
        });

    std::cout << "Server listening on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
}



//save to text file version
//svr.Post("/api/telemetry", [](const httplib::Request& req, httplib::Response& res) {
//    std::cout << "Received Telemetry:\n" << req.body << std::endl;
//
//    // Save incoming JSON to log file
//    SaveToLogFile(req.body);
//
//    res.set_content("Telemetry received\n", "text/plain");
//
//    });