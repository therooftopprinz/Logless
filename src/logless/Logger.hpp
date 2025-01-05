#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <thread>
#include <chrono>

#include <unistd.h>

using BufferLog = std::pair<uint16_t, const void*>;

inline std::string toHexString(const uint8_t* pData, size_t size)
{
    std::stringstream ss;;
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData[i]);
    }

    return ss.str();
}

#define _ static constexpr
template<typename>   struct TypeTraits;
template<typename T> struct TypeTraits<T*>       {_ auto type_id = 0xad; _ size_t size =  sizeof(void*);              };
template<> struct TypeTraits<unsigned char>      {_ auto type_id = 0xa1; _ size_t size =  sizeof(unsigned char);      };
template<> struct TypeTraits<signed char>        {_ auto type_id = 0xa2; _ size_t size =  sizeof(signed char);        };
template<> struct TypeTraits<unsigned short>     {_ auto type_id = 0xa3; _ size_t size =  sizeof(unsigned short);     };
template<> struct TypeTraits<short>              {_ auto type_id = 0xa4; _ size_t size =  sizeof(short);              };
template<> struct TypeTraits<unsigned int>       {_ auto type_id = 0xa5; _ size_t size =  sizeof(unsigned int);       };
template<> struct TypeTraits<int>                {_ auto type_id = 0xa6; _ size_t size =  sizeof(int);                };
template<> struct TypeTraits<unsigned long>      {_ auto type_id = 0xa7; _ size_t size =  sizeof(unsigned long);      };
template<> struct TypeTraits<long>               {_ auto type_id = 0xa8; _ size_t size =  sizeof(long);               };
template<> struct TypeTraits<unsigned long long> {_ auto type_id = 0xa9; _ size_t size =  sizeof(unsigned long long); };
template<> struct TypeTraits<long long>          {_ auto type_id = 0xaa; _ size_t size =  sizeof(long long);          };
template<> struct TypeTraits<float>              {_ auto type_id = 0xab; _ size_t size =  sizeof(float);              };
template<> struct TypeTraits<double>             {_ auto type_id = 0xac; _ size_t size =  sizeof(double);             };
template<> struct TypeTraits<BufferLog>          {_ auto type_id = 0xae; _ size_t size = 0;                           };
template<> struct TypeTraits<const char*>        {_ auto type_id = 0xaf; _ size_t size = 0;                           };
template<> struct TypeTraits<char*>              {_ auto type_id = 0xaf; _ size_t size = 0;                           };
#undef _

template <typename... Ts>
struct TotalSize
{
    static constexpr size_t value = (TypeTraits<Ts>::size+...);
};

template<>
struct TotalSize<>
{
    static constexpr size_t value = 0;
};

class Logger
{
public:
    struct logful_context_t
    {
        std::string tokenFormat;
    };

    using HeaderType = int64_t;
    using TagType    = uint8_t;
    using TailType   = uint8_t;
    template<typename... Ts>
    void log(const char * id, uint64_t pTime, uint64_t pThread, const Ts&... ts)
    {
        if (mLogful)
        {
            logful_context_t ctx;
            uint8_t logbuff[4096*2];
            int flen = std::sprintf((char*)logbuff, "%lluus %llut ", (unsigned long long)pTime, (unsigned long long)pThread);
            size_t sz = flen + 
            logful(ctx, logbuff + flen, id, ts...);
            logbuff[sz++] = '\n';
            [[maybe_unused]] auto rv = ::write(1, logbuff, sz);
        }
        {
            uint8_t buffer[2048];
            uint8_t* usedBuffer = buffer;
            new (usedBuffer) HeaderType(intptr_t(id)-intptr_t(LoggerRef));
            usedBuffer += sizeof(HeaderType);
            size_t sz = logless(usedBuffer, pTime, pThread, ts...) + sizeof(HeaderType);
            std::fwrite((char*)buffer, 1, sz, mOutputFile);
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
    void flush()
    {
        std::fflush(mOutputFile);
    }

    Logger(const char* pFilename)
        : mOutputFile(std::fopen(pFilename, "wb"))
    {
    }

    ~Logger()
    {
        std::fclose(mOutputFile);
    }

private:

    static const char* findNextToken(char openTok, char closeTok, std::string& tokenFormat, const char* pStr)
    {
        // std::cout << "findNextToken(" << pTok << " , [" << (uintptr_t) pStr << "] = " << *pStr << ")\n";
        bool hasToken = false;
        tokenFormat.clear();
        tokenFormat.reserve(16);
        const char* last = pStr;
        while (*pStr)
        {
            if (hasToken && closeTok==*pStr)
            {
                break;
            }

            if (!hasToken && openTok==*pStr)
            {
                last = pStr;
                hasToken = true;
            }

            if (hasToken)
            {
                tokenFormat.push_back(*pStr);
            }

            pStr++;
        }
        // std::cout << "findNextToken = " << (uintptr_t)pStr << " format=\"" << tokenFormat << "\"\n";
        return last;
    }

    size_t logful(logful_context_t& ctx, uint8_t* pOut, const char* pMsg)
    {
        size_t sglen = strlen(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        return sglen;
    }

    template<typename T>
    size_t logful(logful_context_t& ctx, uint8_t*& pOut, const char*& pMsg, T t)
    {
        const char *nTok = findNextToken('%', ';', ctx.tokenFormat,pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        pMsg += sglen + ctx.tokenFormat.size() + 1;
        pOut += sglen;
        int flen = 0;
        // std::cout << "logful(T) nTok=" << uintptr_t(nTok) << " sglen=" << sglen << "\n";

        if (nTok)
        {
            if constexpr (!std::is_same_v<T, BufferLog>)
            {
                flen = std::sprintf((char*)pOut, ctx.tokenFormat.c_str(), t);
                // std::cout << "token: " << "format: " << ctx.tokenFormat.c_str() <<  " value: " << t <<  " formatted: \"" << pOut << "\"\n";
            }
            else
            {
                auto s = toHexString((uint8_t*)t.second, t.first);
                std::memcpy(pOut, s.data(), s.size());
                flen += s.size();
            }
        }
        if (flen>0) pOut += flen;
        return sglen + flen;
    }

    template<typename T, typename... Ts>
    size_t logful(logful_context_t& ctx, uint8_t* pOut, const char* pMsg, T t, Ts... ts)
    {
        return logful(ctx, pOut, pMsg, t) + logful(ctx, pOut, pMsg, ts...);
    }

    size_t logless(uint8_t* pUsedBuffer)
    {
        new (pUsedBuffer) TailType(0);
        return sizeof(TailType);
    }


    template<typename T, typename... Ts>
    size_t logless(uint8_t* pUsedBuffer, T t, Ts... ts)
    {
        new (pUsedBuffer) TagType(TypeTraits<T>::type_id);
        pUsedBuffer += sizeof(TagType);
        new (pUsedBuffer) T(t);
        pUsedBuffer += sizeof(T);
        return logless(pUsedBuffer, ts...) + sizeof(TagType) + sizeof(T);
    }

    template<typename... Ts>
    size_t logless(uint8_t* pUsedBuffer, BufferLog t, Ts... ts)
    {
        new (pUsedBuffer) TagType(TypeTraits<BufferLog>::type_id);
        pUsedBuffer += sizeof(TagType);
        new (pUsedBuffer) BufferLog::first_type(t.first);
        pUsedBuffer += sizeof(BufferLog::first_type);
        std::memcpy(pUsedBuffer, t.second, t.first);
        pUsedBuffer += t.first;
        return logless(pUsedBuffer, ts...) + sizeof(TagType) + sizeof(BufferLog::first_type) + t.first;
    }

    template<typename... Ts>
    size_t logless(uint8_t* pUsedBuffer, const char* t, Ts... ts)
    {
        new (pUsedBuffer) TagType(TypeTraits<const char*>::type_id);
        pUsedBuffer += sizeof(TagType);
        size_t tlen = strlen(t);
        new (pUsedBuffer) BufferLog::first_type(tlen);
        pUsedBuffer += sizeof(BufferLog::first_type);
        std::memcpy(pUsedBuffer, t, tlen);
        pUsedBuffer += tlen;
        return logless(pUsedBuffer, ts...) + sizeof(TagType) + sizeof(BufferLog::first_type) + tlen;
    }

    std::FILE* mOutputFile;
    bool mLogful = false;
    static const char* LoggerRef;
};

template <typename... Ts>
void Logless(Logger& logger, const char* id, Ts... ts)
{
    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t threadId = std::hash<std::thread::id>()(std::this_thread::get_id());
    logger.log(id, timeNow, threadId, ts...);
}

struct LoglessTrace
{
    LoglessTrace(Logger& pLogger, const char* pName)
        : mName(pName)
        , mStart(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())
        , mLogger(pLogger)
    {
        Logless(mLogger, "TRACE ENTER _", mName);
    }
    ~LoglessTrace()
    {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto diff = now-mStart;

        Logless(mLogger, "TRACE LEAVE _ TIME _", mName, diff);
        mLogger.flush();
    }
    const char* mName;
    uint64_t mStart;
    Logger& mLogger;
};

#define LOGLESS_TRACE(logger) LoglessTrace __trace(logger, __PRETTY_FUNCTION__)

#endif // __LOGGER_HPP__
