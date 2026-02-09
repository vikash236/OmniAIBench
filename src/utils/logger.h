/*
 * Logger Utility - Simple logging system for OmniAIBench
 * License: MIT
 */

#pragma once

#include <fstream>
#include <mutex>
#include <string>


enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
public:
  static void Initialize(const std::string &filename);
  static void Shutdown();

  static void Debug(const std::string &message);
  static void Info(const std::string &message);
  static void Warning(const std::string &message);
  static void Error(const std::string &message);

private:
  static void Log(LogLevel level, const std::string &message);

  static std::ofstream logFile;
  static std::mutex logMutex;
  static bool initialized;
};
