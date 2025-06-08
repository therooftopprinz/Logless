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

#define LOGFULDEBUG  if constexpr(false) printf
#define LOGLESSDEBUG if constexpr(false) printf

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
    void log(const char *id, uint64_t p_time, uint64_t p_thread, const ts&... ts_)
    {
        {
            uint8_t buffer[2048];
            uint8_t* used = buffer;
            auto log_point = intptr_t(id)-intptr_t(g_ref);
            LOGLESSDEBUG("log_point=%zd format=\"%s\"\n", log_point, id);

            memcpy(used, &log_point, sizeof(log_point));

            used += sizeof(header_t);
            size_t sz = logless(used, p_time, p_thread, ts_...) + sizeof(header_t);
            std::fwrite((char*)buffer, 1, sz, m_output_file);
        }
        if (m_logful)
        {
            logful_context_t ctx;
            uint8_t logbuff[4096*2];
            int flen = std::sprintf((char*)logbuff, "%" PRIu64 "us 0x%" PRIx64 " ", p_time, p_thread);
            size_t sz = flen +
                logful(ctx, logbuff + flen, id, ts_...);

            LOGFULDEBUG("rem[%zu]=\"%s\"\n", strlen(id), id);

            auto rem_id = strlen(id);
            if (rem_id && sizeof(logbuff) > rem_id+sz)
            {
                strncpy((char*) logbuff+sz, id, rem_id);
                sz += rem_id;
            }

            logbuff[sz++] = '\n';

            [[maybe_unused]] auto rv = ::write(1, logbuff, sz);
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

    static const char* find_next_token(char open_tok, char close_tok, std::string& token_fmt, const char* str)
    {
        LOGFULDEBUG("find_next_token(\"%c\", \"%c\", ..., [%p]=\"%s\")\n", open_tok, close_tok, str, str);

        bool has_token = false;
        token_fmt.clear();
        token_fmt.reserve(16);
        const char* last = str;
        while (*str)
        {
            if (has_token && close_tok==*str)
            {
                break;
            }

            if (!has_token && open_tok==*str)
            {
                last = str;
                has_token = true;
            }

            if (has_token)
            {
                token_fmt.push_back(*str);
            }

            str++;
        }

        if (!*str)
        {
            last = str;
        }

        LOGFULDEBUG("find_next_token(..., ..., \"%s\", [%p]=\"%s\") => %s\n", token_fmt.c_str(), str, str, last);

        return last;
    }

    size_t logful(logful_context_t& ctx, uint8_t* out, const char* msg)
    {
        return 0;
    }

    template<typename T>
    size_t logful(logful_context_t& ctx, uint8_t*& out, const char*& msg, T t)
    {
        const char *n_tok = find_next_token('%', ';', ctx.token_fmt, msg);
        size_t seg_len = uintptr_t(n_tok)-uintptr_t(msg);

        std::memcpy(out, msg, seg_len);

        LOGFULDEBUG("logful<T>(): [%p]=\"%s\"\n", msg, std::string(msg, seg_len).c_str());

        msg += seg_len + ctx.token_fmt.size() + 1;

        out += seg_len;
        int flen = 0;

        if (*n_tok)
        {
            if constexpr (!std::is_same_v<T, buffer_log_t>)
            {
                flen = std::sprintf((char*) out, ctx.token_fmt.c_str(), t);
                LOGFULDEBUG("token: fmt_spec=\"%s\" value=\"%s\"\n", ctx.token_fmt.c_str(), out);
            }
            else
            {
                auto s = to_hex_str((uint8_t*) t.second, t.first);
                std::memcpy(out, s.data(), s.size());
                flen += s.size();
            }
        }

        if (flen>0) out += flen;

        return seg_len + flen;
    }

    template<typename T, typename... Ts>
    size_t logful(logful_context_t& ctx, uint8_t* out, const char*& msg, T t, Ts... ts)
    {
        return logful(ctx, out, msg, t) + logful(ctx, out, msg, ts...);
    }

    size_t logless(uint8_t* p_used)
    {
        LOGLESSDEBUG("tag=0x0\n");
        tail_t t(0);
        memcpy(p_used, &t, sizeof(t));
        return sizeof(tail_t);
    }

    template<typename T, typename... Ts>
    size_t logless(uint8_t* p_used, T t, Ts... ts)
    {
        auto tag = type_traits<T>::type_id;
        memcpy(p_used, &tag, sizeof(tag));
        p_used += sizeof(tag_t);
        memcpy(p_used, &t, sizeof(t));

        LOGLESSDEBUG("tag=0x%x xsvalue=%s\n", tag, to_hex_str(p_used, sizeof(T)).c_str());

        p_used += sizeof(T);
        return logless(p_used, ts...) + sizeof(tag_t) + sizeof(T);
    }

    template<typename... Ts>
    size_t logless(uint8_t* p_used, buffer_log_t t, Ts... ts)
    {
        auto tag = type_traits<buffer_log_t>::type_id;
        memcpy(p_used, &tag, sizeof(tag));
        p_used += sizeof(tag_t);

        memcpy(p_used, &t.first, sizeof(t.first));
        p_used += sizeof(buffer_log_t::first_type);

        LOGLESSDEBUG("tag=0x%x xsvalue[%zu]=%s\n", tag, t.first, to_hex_str(t.second, t.first).c_str());
        std::memcpy(p_used, t.second, t.first);
        p_used += t.first;

        return logless(p_used, ts...) + sizeof(tag_t) + sizeof(buffer_log_t::first_type) + t.first;
    }

    template<typename... Ts>
    size_t logless(uint8_t* p_used, const char* t, Ts... ts)
    {
        auto tag = type_traits<const char*>::type_id;
        memcpy(p_used, &tag, sizeof(tag));
        p_used += sizeof(tag_t);

        size_t tlen = strlen(t);

        LOGLESSDEBUG("tag=0x%x svalue[%zu]=%s\n", tag, tlen, t);

        memcpy(p_used, &tlen, sizeof(tlen));
        p_used += sizeof(buffer_log_t::first_type);

        std::memcpy(p_used, t, tlen);
        p_used += tlen;

        return logless(p_used, ts...) + sizeof(tag_t) + sizeof(buffer_log_t::first_type) + tlen;
    }

    template<typename... Ts>
    size_t logless(uint8_t* p_used, char* t, Ts... ts)
    {
        return logless(p_used, (const char*) t, ts...);
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

#undef LOGFULDEBUG
#define FUNCTION_TRACE(logger) logless::scope __trace(logger, __PRETTY_FUNCTION__)

#endif // __LOGGER_HPP__
