#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <inttypes.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <thread>
#include <chrono>
#include <atomic>


namespace logless
{

using buffer_log_t = std::pair<uint16_t, const void*>;

inline std::string to_hex_str(const uint8_t* pData, size_t size)
{
    std::stringstream ss;;
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData[i]);
    }

    return ss.str();
}

#define _ static constexpr
template<typename>   struct type_traits;
template<typename T> struct type_traits<T*>       {_ auto type_id = 0xad; _ size_t size =  sizeof(void*);              };
template<> struct type_traits<unsigned char>      {_ auto type_id = 0xa1; _ size_t size =  sizeof(unsigned char);      };
template<> struct type_traits<signed char>        {_ auto type_id = 0xa2; _ size_t size =  sizeof(signed char);        };
template<> struct type_traits<unsigned short>     {_ auto type_id = 0xa3; _ size_t size =  sizeof(unsigned short);     };
template<> struct type_traits<short>              {_ auto type_id = 0xa4; _ size_t size =  sizeof(short);              };
template<> struct type_traits<unsigned int>       {_ auto type_id = 0xa5; _ size_t size =  sizeof(unsigned int);       };
template<> struct type_traits<int>                {_ auto type_id = 0xa6; _ size_t size =  sizeof(int);                };
template<> struct type_traits<unsigned long>      {_ auto type_id = 0xa7; _ size_t size =  sizeof(unsigned long);      };
template<> struct type_traits<long>               {_ auto type_id = 0xa8; _ size_t size =  sizeof(long);               };
template<> struct type_traits<unsigned long long> {_ auto type_id = 0xa9; _ size_t size =  sizeof(unsigned long long); };
template<> struct type_traits<long long>          {_ auto type_id = 0xaa; _ size_t size =  sizeof(long long);          };
template<> struct type_traits<float>              {_ auto type_id = 0xab; _ size_t size =  sizeof(float);              };
template<> struct type_traits<double>             {_ auto type_id = 0xac; _ size_t size =  sizeof(double);             };
template<> struct type_traits<buffer_log_t>       {_ auto type_id = 0xae; _ size_t size = 0;                           };
template<> struct type_traits<const char*>        {_ auto type_id = 0xaf; _ size_t size = 0;                           };
template<> struct type_traits<char*>              {_ auto type_id = 0xaf; _ size_t size = 0;                           };
#undef _

constexpr uint64_t LOGALL  = 0;
constexpr unsigned FATAL   = 0;
constexpr unsigned ERROR   = 1;
constexpr unsigned WARNING = 2;
constexpr unsigned DEBUG   = 3;
constexpr unsigned INFO    = 4;
constexpr unsigned TRACE   = 4;

template <typename... Ts>
struct total_sizer
{
    static constexpr size_t value = (type_traits<Ts>::size+...);
};

template<>
struct total_sizer<>
{
    static constexpr size_t value = 0;
};

class logger
{
public:
    struct logful_context_t
    {
        std::string token_fmt;
    };

    using header_t = int64_t;
    using tag_t    = uint8_t;
    using tail_t   = uint8_t;

    template<typename... ts>
    void log(const char * id, uint64_t p_time, uint64_t p_thread, const ts&... ts_)
    {
        if (m_logful)
        {
            logful_context_t ctx;
            uint8_t logbuff[4096*2];
            int flen = std::sprintf((char*)logbuff, "%" PRIu64 " %" PRIu64 " ", p_time, p_thread);
            size_t sz = flen + 
            logful(ctx, logbuff + flen, id, ts_...);
            logbuff[sz++] = '\n';
            [[maybe_unused]] auto rv = ::write(1, logbuff, sz);
        }
        {
            uint8_t buffer[2048];
            uint8_t* used = buffer;
            new (used) header_t(intptr_t(id)-intptr_t(g_ref));
            used += sizeof(header_t);
            size_t sz = logless(used, p_time, p_thread, ts_...) + sizeof(header_t);
            std::fwrite((char*)buffer, 1, sz, m_output_file);
        }
    }
    void logful()
    {
        m_logful = true;
    }
    void logless()
    {
        m_logful = false;
    }
    void flush()
    {
        std::fflush(m_output_file);
    }

    logger(const char* p_filename)
        : m_output_file(std::fopen(p_filename, "wb"))
    {
    }

    ~logger()
    {
        std::fclose(m_output_file);
    }

    void set_level(unsigned level)
    {
        m_level = level;
    }

    unsigned get_level()
    {
        return m_level;
    }

    void set_logbit(uint64_t logbit)
    {
        m_logbit = logbit;
    }

    uint64_t get_logbit()
    {
        return m_logbit;
    }

private:

    static const char* find_next_token(char openTok, char close_tok, std::string& token_fmt, const char* pStr)
    {
        // std::cout << "find_next_token(" << pTok << " , [" << (uintptr_t) pStr << "] = " << *pStr << ")\n";
        bool has_token = false;
        token_fmt.clear();
        token_fmt.reserve(16);
        const char* last = pStr;
        while (*pStr)
        {
            if (has_token && close_tok==*pStr)
            {
                break;
            }

            if (!has_token && openTok==*pStr)
            {
                last = pStr;
                has_token = true;
            }

            if (has_token)
            {
                token_fmt.push_back(*pStr);
            }

            pStr++;
        }
        // std::cout << "find_next_token = " << (uintptr_t)pStr << " format=\"" << token_fmt << "\"\n";
        return last;
    }

    size_t logful(logful_context_t& ctx, uint8_t* out, const char* msg)
    {
        size_t sglen = strlen(msg);
        std::memcpy(out, msg, sglen);
        return sglen;
    }

    template<typename T>
    size_t logful(logful_context_t& ctx, uint8_t*& out, const char*& msg, T t)
    {
        const char *nTok = find_next_token('%', ';', ctx.token_fmt, msg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(msg);
        std::memcpy(out, msg, sglen);
        msg += sglen + ctx.token_fmt.size() + 1;
        out += sglen;
        int flen = 0;
        // std::cout << "logful(T) nTok=" << uintptr_t(nTok) << " sglen=" << sglen << "\n";

        if (nTok)
        {
            if constexpr (!std::is_same_v<T, buffer_log_t>)
            {
                flen = std::sprintf((char*) out, ctx.token_fmt.c_str(), t);
                // std::cout << "token: " << "format: " << ctx.token_fmt.c_str() <<  " value: " << t <<  " formatted: \"" << out << "\"\n";
            }
            else
            {
                auto s = to_hex_str((uint8_t*) t.second, t.first);
                std::memcpy(out, s.data(), s.size());
                flen += s.size();
            }
        }
        if (flen>0) out += flen;
        return sglen + flen;
    }

    template<typename T, typename... Ts>
    size_t logful(logful_context_t& ctx, uint8_t* out, const char* msg, T t, Ts... ts)
    {
        return logful(ctx, out, msg, t) + logful(ctx, out, msg, ts...);
    }

    size_t logless(uint8_t* p_used)
    {
        new (p_used) tail_t(0);
        return sizeof(tail_t);
    }


    template<typename T, typename... Ts>
    size_t logless(uint8_t* p_used, T t, Ts... ts)
    {
        new (p_used) tag_t(type_traits<T>::type_id);
        p_used += sizeof(tag_t);
        new (p_used) T(t);
        p_used += sizeof(T);
        return logless(p_used, ts...) + sizeof(tag_t) + sizeof(T);
    }

    template<typename... Ts>
    size_t logless(uint8_t* p_used, buffer_log_t t, Ts... ts)
    {
        new (p_used) tag_t(type_traits<buffer_log_t>::type_id);
        p_used += sizeof(tag_t);

        new (p_used) buffer_log_t::first_type(t.first);
        p_used += sizeof(buffer_log_t::first_type);

        std::memcpy(p_used, t.second, t.first);
        p_used += t.first;

        return logless(p_used, ts...) + sizeof(tag_t) + sizeof(buffer_log_t::first_type) + t.first;
    }

    template<typename... Ts>
    size_t logless(uint8_t* p_used, const char* t, Ts... ts)
    {
        new (p_used) tag_t(type_traits<const char*>::type_id);
        p_used += sizeof(tag_t);

        size_t tlen = strlen(t);
        new (p_used) buffer_log_t::first_type(tlen);
        p_used += sizeof(buffer_log_t::first_type);

        std::memcpy(p_used, t, tlen);
        p_used += tlen;

        return logless(p_used, ts...) + sizeof(tag_t) + sizeof(buffer_log_t::first_type) + tlen;
    }

    std::FILE* m_output_file;
    std::atomic<bool> m_logful = false;
    static const char* g_ref;
    std::atomic<unsigned> m_level = std::numeric_limits<unsigned>::max();
    std::atomic<uint64_t> m_logbit = 0;
};

template <typename... Ts>
void log(logger& logger, unsigned level, uint64_t bit, const char* id, Ts... ts)
{
    if (level > logger.get_level())
    {
        return;
    }

    if (bit && !(bit & logger.get_logbit()))
    {
        return;
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t threadId = std::hash<std::thread::id>()(std::this_thread::get_id());
    logger.log(id, timeNow, threadId, ts...);
}

struct scope
{
    scope(logger& p_logger, const char* p_name)
        : m_name(p_name)
        , m_start(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())
        , m_logger(p_logger)
    {
        log(m_logger, TRACE, LOGALL, "SCOPE ENTER %s;", m_name);
    }

    ~scope()
    {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto diff = now-m_start;

        log(m_logger, TRACE, LOGALL, "SCOPE LEAVE %s; TIME %llu;", m_name, diff);
        m_logger.flush();
    }

    const char* m_name;
    uint64_t m_start;
    logger& m_logger;
};

} // namespace loggless

#define FUNCTION_TRACE(logger) logless::scope __trace(logger, __PRETTY_FUNCTION__)

#endif // __LOGGER_HPP__
