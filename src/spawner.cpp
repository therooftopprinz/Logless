#include <logless/logger.hpp>

#include <string_view>
#include <functional>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <utility>
#include <sstream>
#include <cstring>
#include <vector>
#include <thread>
#include <cstdio>
#include <regex>

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SPAWNERDEBUG if constexpr(false) std::cout

namespace logless
{
enum class State {
    Logpoint, Time, Thread, Tag,
    Param8, ParamU8,
    Param16, ParamU16,
    Param32, ParamU32,
    ParamU32or34, Param32or34,
    Param64, ParamU64,
    ParamFloat, ParamDouble,
    ParamVoidP,
    ParamBufferLogSz, ParamBufferLogData,
    ParamStrSz, ParamStrData,
    N};

struct logpoint{};
struct time{};
struct thread{};
struct tag
{
    uint8_t data;
};
struct buffer{};
struct buffer2{};
struct string{};
struct string2{};

template <State> struct StateTraits;
template<> struct StateTraits<State::Param8>            {using type = unsigned char;};
template<> struct StateTraits<State::ParamU8>           {using type = signed char;};
template<> struct StateTraits<State::Param16>           {using type = unsigned short;};
template<> struct StateTraits<State::ParamU16>          {using type = short;};
template<> struct StateTraits<State::Param32>           {using type = unsigned int;};
template<> struct StateTraits<State::ParamU32>          {using type = int;};
template<> struct StateTraits<State::ParamU32or34>      {using type = unsigned long;};
template<> struct StateTraits<State::Param32or34>       {using type = long;};
template<> struct StateTraits<State::Param64>           {using type = unsigned long long;};
template<> struct StateTraits<State::ParamU64>          {using type = long long;};
template<> struct StateTraits<State::ParamFloat>        {using type = float;};
template<> struct StateTraits<State::ParamDouble>       {using type = double;};
template<> struct StateTraits<State::ParamVoidP>        {using type = void*;};
template<> struct StateTraits<State::ParamBufferLogSz>  {using type = buffer;};
template<> struct StateTraits<State::ParamStrSz>        {using type = string;};
template<> struct StateTraits<State::Logpoint>          {using type = logpoint;};
template<> struct StateTraits<State::Time>              {using type = time;};
template<> struct StateTraits<State::Thread>            {using type = thread;};
template<> struct StateTraits<State::Tag>               {using type = tag;};
template<> struct StateTraits<State::ParamBufferLogData>{using type = buffer2;};
template<> struct StateTraits<State::ParamStrData>      {using type = string2;};

class Spawner
{
public:
    Spawner(std::vector<char>&& pRodata)
        : Spawner(std::move(pRodata) ,ALL_STATES())
    {}

    Spawner() = delete;

    template <typename T>
    void decodeParam(T i)
    {
        if (sizeof(T) == mReadSz)
        {
            std::memcpy(&i, mReadBuff, sizeof(i));
            snprintf(fmtbuf, sizeof(fmtbuf), tokenFormat.c_str(), i);
            SPAWNERDEBUG << "state: " << std::dec << unsigned(mState) << "format: " << tokenFormat.c_str() <<  " value: " << fmtbuf << "\n";
            mSs << fmtbuf;
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void decodeParam(logpoint)
    {
        if (sizeof(int64_t) == mReadSz)
        {
            int64_t i;
            std::memcpy(&i, mReadBuff, sizeof(i));
            mLogPoint = mRodata.data() + mRefPos + i;
            mState = State::Time;
            mReadSz = 0;
            SPAWNERDEBUG << "state: Logpoint@(rel=" << std::dec << i << " abs=" << (void*)(mRefPos+i) << "): " << mLogPoint << "\n";
        }
    }

    void decodeParam(time)
    {
        if (sizeof(logger::tag_t) + sizeof(uint64_t) == mReadSz)
        {
            std::memcpy(&mLogTime, mReadBuff + sizeof(logger::tag_t), sizeof(mLogTime));
            mState = State::Thread;
            mReadSz = 0;
            SPAWNERDEBUG << "state: Time: " << std::dec << mLogTime << "\n";
        }
    }

    void decodeParam(thread)
    {
        if (sizeof(logger::tag_t) + sizeof(uint64_t) == mReadSz)
        {
            std::memcpy(&mLogThread, mReadBuff + sizeof(logger::tag_t), sizeof(mLogThread));
            SPAWNERDEBUG << "state: Thread: " << std::hex << "0x" << mLogThread << "\n";
            mSs << std::dec << mLogTime << "us ";
            mSs << std::hex << "0x" << mLogThread << " ";
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void decodeParam(tag pTag)
    {
        auto pData = pTag.data;
        auto ntok = findNextToken('%', ';', tokenFormat, mLogPoint);
        size_t seg_len = uintptr_t(ntok)-uintptr_t(mLogPoint);
        std::string logSeg(mLogPoint, seg_len);

        SPAWNERDEBUG << "log_segment: st[" << std::dec << seg_len << "]:" << logSeg  << "\n";
        SPAWNERDEBUG << "state: Tag: 0x" << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData) << "\n";

        mSs << logSeg;
        if (*ntok) mLogPoint = ntok + tokenFormat.size() + 1;
        if      (pData == type_traits<unsigned char>::type_id)      mState = State::ParamU8;
        else if (pData == type_traits<signed char>::type_id)        mState = State::Param8;
        else if (pData == type_traits<unsigned short>::type_id)     mState = State::ParamU16;
        else if (pData == type_traits<short>::type_id)              mState = State::Param16;
        else if (pData == type_traits<unsigned int>::type_id)       mState = State::ParamU32;
        else if (pData == type_traits<int>::type_id)                mState = State::Param32;
        else if (pData == type_traits<unsigned long>::type_id)      mState = State::ParamU32or34;
        else if (pData == type_traits<long>::type_id)               mState = State::Param32or34;
        else if (pData == type_traits<unsigned long long>::type_id) mState = State::ParamU64;
        else if (pData == type_traits<long long>::type_id)          mState = State::Param64;
        else if (pData == type_traits<float>::type_id)              mState = State::ParamFloat;
        else if (pData == type_traits<double>::type_id)             mState = State::ParamDouble;
        else if (pData == type_traits<void*>::type_id)              mState = State::ParamVoidP;
        else if (pData == type_traits<buffer_log_t>::type_id)       mState = State::ParamBufferLogSz;
        else if (pData == type_traits<const char*>::type_id)        mState = State::ParamStrSz;
        else
        {
            std::cout << mSs.str() << "\n";
            mSs = {};
            mState = State::Logpoint;
        }
        mReadSz = 0;
    }

    void decodeParam(buffer)
    {
        if (sizeof(buffer_log_t::first_type) == mReadSz)
        {
            buffer_log_t::first_type i;
            std::memcpy(&i, mReadBuff, sizeof(i));
            SPAWNERDEBUG << "state: ParamBufferLogSz: " << i << "\n";
            mBufferLogSz = i;
            mState = State::ParamBufferLogData;
            mReadSz = 0;
        }
    }

    void decodeParam(buffer2)
    {
        if (mBufferLogSz == mReadSz)
        {
            auto i = to_hex_str((uint8_t*)mReadBuff, mReadSz);
            SPAWNERDEBUG << "state: ParamBufferLogData: " << i << "\n";
            mSs << i;
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void decodeParam(string)
    {
        if (sizeof(buffer_log_t::first_type) == mReadSz)
        {
            buffer_log_t::first_type i;
            std::memcpy(&i, mReadBuff, sizeof(i));
            SPAWNERDEBUG << "state: ParamStrSz: " << i << "\n";
            mBufferLogSz = i;
            mState = State::ParamStrData;
            mReadSz = 0;
        }
    }

    void decodeParam(string2)
    {
        if (mBufferLogSz == mReadSz)
        {
            std::string_view i(mReadBuff, mReadSz);
            SPAWNERDEBUG << "state: ParamStrData: " << i << "\n";
            mSs << i;
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void in(uint8_t pData)
    {
        SPAWNERDEBUG << "in[" << std::dec << std::setw(2) << std::setfill('0') << mReadSz << "]= " << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData) << " " << pData << "\n";
        mReadBuff[mReadSz++] = pData;
        if (mState == State::Tag)
            return decodeParam((tag){pData});
        fn[size_t(mState)]();
    }
private:
    using ALL_STATES = std::make_integer_sequence<size_t, size_t(State::N)>;

    template<size_t... I>
    Spawner(std::vector<char>&& pRodata, std::integer_sequence<size_t, I...>)
        : fn({[this](){decodeParam(typename StateTraits<State(I)>::type());}...})
        , mRodata(std::move(pRodata))
        , mRefPos(std::string_view((char*)mRodata.data(), mRodata.size()).find("mV@it2+UNd$j*rHr=4&S6etGWQN1ub-Zaf2e,.n[mf@KQ3+aJMF?+4f).TwVPU2qKkB3nG/hZJ[D2)4L0?7ST!j{T]9{wF5[ePJg+_@5]Q$5SX2jX1,.P6hb[cTYMb]+"))
    {
        if (std::string_view::npos == mRefPos)
            throw std::runtime_error("logger::g_ref not found in rodata");
        SPAWNERDEBUG << "ref_pos=" << mRefPos << "(" << (void*) mRefPos << ")\n";
    }

    static const char* findNextToken(char openTok, char closeTok, std::string& tokenFormat, const char* pStr)
    {
        SPAWNERDEBUG << "findNextToken('" << openTok << "', '" << closeTok << "' , [" << (void*) pStr << "] = \"" << pStr << "\")\n";
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

        if (!*pStr)
        {
            last = pStr;
        }

        SPAWNERDEBUG << "findNextToken: [" << (void*)pStr << "]=\""<< pStr << "\" format=\"" << tokenFormat << "\" => \"" << last << "\"\n" ;
        return last;
    }

    std::array<std::function<void()>, size_t(State::N)> fn;
    std::vector<char> mRodata;
    State mState = State::Logpoint;
    char mReadBuff[1024*1024];
    size_t mReadSz = 0;
    size_t mRefPos;
    // logline
    const char* mLogPoint = nullptr;
    uint64_t mLogTime;
    uint64_t mLogThread;
    buffer_log_t::first_type mBufferLogSz;
    std::stringstream mSs;
    std::string tokenFormat;
    char fmtbuf[128*128];
};

} // namespace logless

int main(int argc, const char* argv[])
{
    std::vector<char> rodata;
    assert(argc == 4);
    auto rodatefile  = open(argv[1], O_RDONLY);
    bool exit_eof = std::string_view("exiteof")==argv[3];

    int c;
    while (read(rodatefile, &c, 1) >0)
        rodata.push_back(c);

    logless::Spawner spawner(std::move(rodata));

    auto loglessfile = open(argv[2], O_RDONLY);

    if (-1 == loglessfile)
    {
        throw std::runtime_error("Failed to open log file!");
    }
    
    char in[512];
    while (1)
    {
        int rd = read(loglessfile, &in, sizeof(in));
        if (0 < rd)
        {
            for (int i=0; i < rd; i++)
            {
                spawner.in(in[i]);
            }
        }
        else if (0 == rd)
        {
            if (exit_eof)
            {
                printf("--exiteof--\n");
                break;
            }
            continue;
        }
        else
        {
            using namespace std::string_literals;
            throw std::runtime_error("error reading: rd="s + std::to_string(rd) + " error=" + std::strerror(errno));
        }
    }
}
