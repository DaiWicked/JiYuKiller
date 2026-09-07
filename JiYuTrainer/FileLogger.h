#pragma once
#include "stdafx.h"
#include <string>
#include <mutex>
#include <cstdio>

class FileLogger
{
public:
    static FileLogger* Get(const std::wstring& logName);
    void Log(const wchar_t* format, ...);
    void LogInfo(const wchar_t* format, ...);
    void LogWarn(const wchar_t* format, ...);
    void LogError(const wchar_t* format, ...);
    void WriteRaw(const std::wstring& text);
    std::wstring GetLogPath() const { return logFilePath_; }

private:
    FileLogger(const std::wstring& logName);
    ~FileLogger();
    void WriteLine(const std::wstring& level, const std::wstring& text);
    static std::wstring GetExeDir();
    std::wstring logName_;
    std::wstring logFilePath_;
    FILE* file_;
    std::mutex mutex_;
};