#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <unistd.h>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <thread>
#include <chrono>
#include <mutex>

using BufferLog = std::pair<uint16_t, void*>;

#define _ static constexpr
template<typename> struct TypeTraits;
template<> struct TypeTraits<uint8_t>     {_ auto type_id = 0xa1; _ size_t size =  sizeof(uint8_t);     _ char cfmt[] = "%c";};
template<> struct TypeTraits<int8_t>      {_ auto type_id = 0xa2; _ size_t size =  sizeof(int8_t);      _ char cfmt[] = "%c";};
template<> struct TypeTraits<uint16_t>    {_ auto type_id = 0xa3; _ size_t size =  sizeof(uint16_t);    _ char cfmt[] = "%u";};
template<> struct TypeTraits<int16_t>     {_ auto type_id = 0xa4; _ size_t size =  sizeof(int16_t);     _ char cfmt[] = "%d";};
template<> struct TypeTraits<uint32_t>    {_ auto type_id = 0xa5; _ size_t size =  sizeof(uint32_t);    _ char cfmt[] = "%u";};
template<> struct TypeTraits<int32_t>     {_ auto type_id = 0xa6; _ size_t size =  sizeof(int32_t);     _ char cfmt[] = "%d";};
template<> struct TypeTraits<uint64_t>    {_ auto type_id = 0xa7; _ size_t size =  sizeof(uint64_t);    _ char cfmt[] = "%ld";};
template<> struct TypeTraits<int64_t>     {_ auto type_id = 0xa8; _ size_t size =  sizeof(int64_t);     _ char cfmt[] = "%lu";};
template<> struct TypeTraits<float>       {_ auto type_id = 0xa9; _ size_t size =  sizeof(float);       _ char cfmt[] = "%f";};
template<> struct TypeTraits<double>      {_ auto type_id = 0xaa; _ size_t size =  sizeof(double);      _ char cfmt[] = "%f";};
template<> struct TypeTraits<void*>       {_ auto type_id = 0xab; _ size_t size =  sizeof(void*);       _ char cfmt[] = "%p";};
template<> struct TypeTraits<BufferLog>   {_ auto type_id = 0xac; _ size_t size =  sizeof(void*);       _ char cfmt[] = "%s";};
#undef _

template <typename... Ts>
struct TotalSize
{
    static constexpr size_t value = (TypeTraits<Ts>::size+...);
};

std::string toHexString(const uint8_t* pData, size_t size)
{
    std::stringstream ss;;
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData[i]);
    }

    return ss.str();
}

class Logger
{
public:
    using HeaderType = int32_t;
    using TagType    = uint8_t;
    using TailType   = uint8_t;
    template<typename... Ts>
    void log(const char * id, uint64_t pTime, uint64_t pThread, const Ts&... ts)
    {
        if (mLogful)
        {
            uint8_t logbuff[192];
            int flen = std::sprintf((char*)logbuff, "%luus %lut ", pTime, pThread);
            size_t sz = logful(logbuff + flen, id, ts...) + flen;
            logbuff[sz++] = '\n';
            ::write(1, logbuff, sz);
        }
        {       
            constexpr size_t payloadSize = sizeof(HeaderType) + sizeof(TagType)*2 + sizeof(pTime) + sizeof(pThread) +
                sizeof(TagType)*sizeof...(Ts) + TotalSize<Ts...>::value + sizeof(TailType);
            uint8_t usedBuffer[payloadSize];
            int usedIdx = 0;
            new (usedBuffer + usedIdx) HeaderType(intptr_t(id)-intptr_t(LoggerRef));
            usedIdx += sizeof(HeaderType);
            logless(usedBuffer, usedIdx, pTime, pThread, ts...);
            std::fwrite((char*)usedBuffer, 1, payloadSize, mOutputFile);
        }
    }
    void logful()
    {
        mLogful = true;
    }
    void logless()
    {
        mLogful = false;
    }
    static Logger& getInstance()
    {
        static Logger logger{};
        return logger;
    }

private:

    Logger()
        : mOutputFile(std::fopen("log.bin", "wb"))
    {
        std::cout << "Logger::Logger\n";
    }

    ~Logger()
    {
        std::fclose(mOutputFile);
    }

    const char* findNextToken(char pTok, const char* pStr)
    {
        while (*pStr!=0&&*pStr!=pTok)
        {
            pStr++;
        }
        return pStr;
    }

    size_t logful(uint8_t* pOut, const char* pMsg)
    {
        const char *nTok = findNextToken('_',pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        return sglen;
    }

    template<typename T, typename... Ts>
    size_t logful(uint8_t* pOut, const char* pMsg, T t, Ts... ts)
    {
        const char *nTok = findNextToken('_',pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        pMsg+=sglen;
        pOut+=sglen;
        int flen = 0;
        if (*nTok)
        {
            flen = std::sprintf((char*)pOut, TypeTraits<T>::cfmt, t);
            pMsg++;
        }
        if (flen>0) pOut += flen;
        return sglen + flen + logful(pOut, pMsg, ts...);
    }

    template<typename... Ts>
    size_t logful(uint8_t* pOut, const char* pMsg, BufferLog t, Ts... ts)
    {
        const char *nTok = findNextToken('_',pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        pMsg+=sglen;
        pOut+=sglen;
        int flen = 0;
        if (*nTok)
        {
            auto s = toHexString((uint8_t*)t.second, t.first);
            std::memcpy(pOut, s.data(), s.size());
            flen += s.size();
            pMsg++;
        }
        if (flen>0) pOut += flen;
        return sglen + flen + logful(pOut, pMsg, ts...);
    }


    void logless(uint8_t* pUsedBuffer, int& pUsedIndex)
    {
        new (pUsedBuffer+pUsedIndex) TailType(0);
    }

    template<typename T, typename... Ts>
    void logless(uint8_t* pUsedBuffer, int& pUsedIndex, T t, Ts... ts)
    {
        new (pUsedBuffer + pUsedIndex) TagType(TypeTraits<T>::type_id);
        pUsedIndex += sizeof(TagType);
        new (pUsedBuffer + pUsedIndex) T(t);
        pUsedIndex += sizeof(T);
        logless(pUsedBuffer, pUsedIndex, ts...);
    }

    template<typename... Ts>
    void logless(uint8_t* pUsedBuffer, int& pUsedIndex, BufferLog t, Ts... ts)
    {
        new (pUsedBuffer + pUsedIndex) TagType(TypeTraits<BufferLog>::type_id);
        pUsedIndex += sizeof(TagType);
        new (pUsedBuffer + pUsedIndex) BufferLog::first_type(t.first);
        pUsedIndex += sizeof(BufferLog::first_type);
        std::memcpy(pUsedBuffer + pUsedIndex, t.second, t.first);
        pUsedIndex += t.first;
        logless(pUsedBuffer, pUsedIndex, ts...);
    }

    std::FILE* mOutputFile;
    bool mLogful = false;
    static constexpr const char* LoggerRef = "LoggerRefXD";
};

template <typename... Ts>
void Logless(const char* id, Ts... ts)
{
    static uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t threadId = std::hash<std::thread::id>()(std::this_thread::get_id());
    Logger::getInstance().log(id, timeNow, threadId, ts...);
}

#endif // __LOGGER_HPP__