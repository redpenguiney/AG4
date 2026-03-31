#pragma once
#include <iostream>
#include <mutex>
#include <glm/gtx/io.hpp>
#include <string.h>
#include <sstream>

// TODO: / vs \\ will be a problem
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#if defined(_MSC_VER)
void MSVCDebugStringPrint(const char* c);
#endif

inline std::mutex LOGGING_MUTEX;
inline std::stringstream LOGGING_SSTREAM;
void TestPrint();

template<typename ... Args>
void _DebugLogInfo_(const char* file, int lineNumber, Args... args) {
    LOGGING_MUTEX.lock();
#if defined(_MSC_VER)
    LOGGING_SSTREAM << "INFO: (from " << file << ":" << lineNumber << ") ";
    ((LOGGING_SSTREAM << std::forward<Args>(args)), ...);
    LOGGING_SSTREAM << "\n";
    MSVCDebugStringPrint(LOGGING_SSTREAM.str().c_str());
    LOGGING_SSTREAM.str({});
#endif
    std::cout << "\x1B[33mINFO: (from " << file << ":" << lineNumber << ")\x1B[37m ";
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";

    std::cout.flush();
    LOGGING_MUTEX.unlock();

}

template<typename ... Args>
void _DebugLogError_(const char* file, int lineNumber, Args... args) {
    LOGGING_MUTEX.lock();

#if defined(_MSC_VER)
    LOGGING_SSTREAM << "ERROR: (from " << file << ":" << lineNumber << ") ";
    ((LOGGING_SSTREAM << std::forward<Args>(args)), ...);
    LOGGING_SSTREAM << "\n";
    MSVCDebugStringPrint(LOGGING_SSTREAM.str().c_str());
    LOGGING_SSTREAM.str({});
#endif
    std::cout << "\x1B[31mERROR: (from " << file << ":" << lineNumber << ")\x1B[37m ";
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";

    std::cout.flush();
    LOGGING_MUTEX.unlock();
}

#define DebugLogError(...) {_DebugLogError_(__FILENAME__, __LINE__, __VA_ARGS__);}
#define DebugLogInfo(...) {_DebugLogInfo_(__FILENAME__, __LINE__, __VA_ARGS__);}

