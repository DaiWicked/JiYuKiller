#include "stdafx.h"
#include "FileLogger.h"
#include <map>
#include <stdarg.h>
#include <time.h>

static std::map<std::wstring, FileLogger*>& GetInstances()
{
    static std::map<std::wstring, FileLogger*> instances;
    return instances;
}

FileLogger* FileLogger::Get(const std::wstring& logName)
{
    auto& instances = GetInstances();
    auto it = instances.find(logName);
    if (it != instances.end())
        return it->second;
    FileLogger* logger = new FileLogger(logName);
    instances[logName] = logger;
    return logger;
}

std::wstring FileLogger::GetExeDir()
{
    WCHAR path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath(path);
    size_t pos = exePath.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        return exePath.substr(0, pos + 1);
    return L".\\";
}

FileLogger::FileLogger(const std::wstring& logName)
    : logName_(logName)
{
    logFilePath_ = GetExeDir() + logName + L"_log.txt";
    _wfopen_s(&file_, logFilePath_.c_str(), L"a+, ccs=UTF-8");
    if (file_)
    {
        fwprintf(file_, L"\n=== %s 日志启动 ===\n", logName_.c_str());
        fflush(file_);
    }
}

FileLogger::~FileLogger()
{
    if (file_)
    {
        fclose(file_);
        file_ = nullptr;
    }
}

void FileLogger::WriteRaw(const std::wstring& text)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_) return;
    fwprintf(file_, L"%s\n", text.c_str());
    fflush(file_);
}
void FileLogger::WriteLine(const std::wstring& level, const std::wstring& text)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_) return;

    // 时间戳
    time_t now = time(nullptr);
    struct tm t;
    localtime_s(&t, &now);
    WCHAR timeBuf[64];
    swprintf_s(timeBuf, L"%04d-%02d-%02d %02d:%02d:%02d",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec);

    fwprintf(file_, L"[%s] [%s] %s\n", timeBuf, level.c_str(), text.c_str());
    fflush(file_);
}

void FileLogger::Log(const wchar_t* format, ...)
{
    WCHAR buf[2048];
    va_list args;
    va_start(args, format);
    vswprintf_s(buf, format, args);
    va_end(args);
    WriteLine(L"信息", buf);
}

void FileLogger::LogInfo(const wchar_t* format, ...)
{
    WCHAR buf[2048];
    va_list args;
    va_start(args, format);
    vswprintf_s(buf, format, args);
    va_end(args);
    WriteLine(L"信息", buf);
}

void FileLogger::LogWarn(const wchar_t* format, ...)
{
    WCHAR buf[2048];
    va_list args;
    va_start(args, format);
    vswprintf_s(buf, format, args);
    va_end(args);
    WriteLine(L"警告", buf);
}

void FileLogger::LogError(const wchar_t* format, ...)
{
    WCHAR buf[2048];
    va_list args;
    va_start(args, format);
    vswprintf_s(buf, format, args);
    va_end(args);
    WriteLine(L"错误", buf);
}

