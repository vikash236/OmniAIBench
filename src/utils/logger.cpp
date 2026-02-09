/*
 * Logger Utility Implementation
 * License: MIT
 */

#include "logger.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>


std::ofstream Logger::logFile;
std::mutex Logger::logMutex;
bool Logger::initialized = false;

void Logger::Initialize(const std::string &filename) {
  std::lock_guard<std::mutex> lock(logMutex);

  if (!initialized) {
    logFile.open(filename, std::ios::out | std::ios::app);
    initialized = true;
    Info("Logger initialized");
  }
}

void Logger::Shutdown() {
  std::lock_guard<std::mutex> lock(logMutex);

  if (initialized) {
    Info("Logger shutting down");
    logFile.close();
    initialized = false;
  }
}

void Logger::Debug(const std::string &message) {
  Log(LogLevel::DEBUG, message);
}

void Logger::Info(const std::string &message) { Log(LogLevel::INFO, message); }

void Logger::Warning(const std::string &message) {
  Log(LogLevel::WARNING, message);
}

void Logger::Error(const std::string &message) {
  Log(LogLevel::ERROR, message);
}

void Logger::Log(LogLevel level, const std::string &message) {
  std::lock_guard<std::mutex> lock(logMutex);

  if (!initialized)
    return;

  // Get current time
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

  // Level string
  std::string levelStr;
  switch (level) {
  case LogLevel::DEBUG:
    levelStr = "DEBUG";
    break;
  case LogLevel::INFO:
    levelStr = "INFO ";
    break;
  case LogLevel::WARNING:
    levelStr = "WARN ";
    break;
  case LogLevel::ERROR:
    levelStr = "ERROR";
    break;
  }

  // Write to file
  logFile << "[" << ss.str() << "] [" << levelStr << "] " << message
          << std::endl;
  logFile.flush();

  // Also write to console
  std::cout << "[" << levelStr << "] " << message << std::endl;
}
