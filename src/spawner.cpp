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

namespace spawner
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
            // std::cout << "state: " << std::dec << unsigned(mState) << "format: " << tokenFormat.c_str() <<  " value: " << fmtbuf << "\n";
            mSs << fmtbuf;
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void decodeParam(logpoint)
    {
        if (sizeof(uint64_t) == mReadSz)
        {
            uint64_t i;
            std::memcpy(&i, mReadBuff, sizeof(i));
            mLogPoint = mRodata.data() + mRefPos + i;
            mState = State::Time;
            mReadSz = 0;
            // std::cout << "state: Logpoint@" << std::dec << i << ": " << mLogPoint << "\n";
        }
    }

    void decodeParam(time)
    {
        if (sizeof(Logger::TagType) + sizeof(uint64_t) == mReadSz)
        {
            std::memcpy(&mLogTime, mReadBuff + sizeof(Logger::TagType), sizeof(mLogTime));
            mState = State::Thread;
            mReadSz = 0;
            // std::cout << "state: Time: " << std::dec << mLogTime << "\n";
        }
    }

    void decodeParam(thread)
    {
        if (sizeof(Logger::TagType) + sizeof(uint64_t) == mReadSz)
        {
            std::memcpy(&mLogThread, mReadBuff + sizeof(Logger::TagType), sizeof(mLogThread));
            // std::cout << "state: Thread: " << std::dec << mLogThread << "\n";
            mSs << std::dec << mLogTime << "us ";
            mSs << std::hex << mLogThread << "t ";
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void decodeParam(tag pTag)
    {
        auto pData = pTag.data;
        auto ntok = findNextToken('%', ';', tokenFormat, mLogPoint);
        size_t sglen = uintptr_t(ntok)-uintptr_t(mLogPoint);
        std::string logSeg(mLogPoint, sglen);

        // std::cout << "state: Tag: " << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData) << "\n";
        // std::cout << "seg str:" << logSeg  << "\n";

        mSs << logSeg;
        if (*ntok) mLogPoint = ntok + tokenFormat.size() + 1;
        if      (pData == TypeTraits<unsigned char>::type_id)      mState = State::ParamU8;
        else if (pData == TypeTraits<signed char>::type_id)        mState = State::Param8;
        else if (pData == TypeTraits<unsigned short>::type_id)     mState = State::ParamU16;
        else if (pData == TypeTraits<short>::type_id)              mState = State::Param16;
        else if (pData == TypeTraits<unsigned int>::type_id)       mState = State::ParamU32;
        else if (pData == TypeTraits<int>::type_id)                mState = State::Param32;
        else if (pData == TypeTraits<unsigned long>::type_id)      mState = State::ParamU32or34;
        else if (pData == TypeTraits<long>::type_id)               mState = State::Param32or34;
        else if (pData == TypeTraits<unsigned long long>::type_id) mState = State::ParamU64;
        else if (pData == TypeTraits<long long>::type_id)          mState = State::Param64;
        else if (pData == TypeTraits<float>::type_id)              mState = State::ParamFloat;
        else if (pData == TypeTraits<double>::type_id)             mState = State::ParamDouble;
        else if (pData == TypeTraits<void*>::type_id)              mState = State::ParamVoidP;
        else if (pData == TypeTraits<BufferLog>::type_id)          mState = State::ParamBufferLogSz;
        else if (pData == TypeTraits<const char*>::type_id)        mState = State::ParamStrSz;
        else
        {
            std::cout << mSs.str() << "\n";
            // RTP TODO: Buggy in ARM gcc
            // mSs = std::stringstream();
            mSs.~basic_stringstream();
            new (&mSs) std::stringstream();
            // mSs.str("");
            mState = State::Logpoint;
        }
        mReadSz = 0;
    }

    void decodeParam(buffer)
    {
        if (sizeof(BufferLog::first_type) == mReadSz)
        {
            BufferLog::first_type i;
            std::memcpy(&i, mReadBuff, sizeof(i));
            // std::cout << "state: ParamBufferLogSz: " << i << "\n";
            mBufferLogSz = i;
            mState = State::ParamBufferLogData;
            mReadSz = 0;
        }
    }

    void decodeParam(buffer2)
    {
        if (mBufferLogSz == mReadSz)
        {
            auto i = toHexString((uint8_t*)mReadBuff, mReadSz);
            // std::cout << "state: ParamBufferLogData: " << i << "\n";
            mSs << i;
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void decodeParam(string)
    {
        if (sizeof(BufferLog::first_type) == mReadSz)
        {
            BufferLog::first_type i;
            std::memcpy(&i, mReadBuff, sizeof(i));
            // std::cout << "state: ParamStrSz: " << i << "\n";
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
            // std::cout << "state: ParamStrData: " << i << "\n";
            mSs << i;
            mState = State::Tag;
            mReadSz = 0;
        }
    }

    void in(uint8_t pData)
    {
        // std::cout << "in[" << std::dec << std::setw(2) << std::setfill('0') << mReadSz << "]= " << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData) << " " << pData << "\n";
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
        , mRefPos(std::string_view((char*)mRodata.data(), mRodata.size()).find("LoggerRefXD"))
    {
        if (std::string_view::npos == mRefPos)
            throw std::runtime_error("LoggerRefXD not found in rodata");
    }

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
    BufferLog::first_type mBufferLogSz;
    std::stringstream mSs;
    std::string tokenFormat;
    char fmtbuf[128*128];
};

} //  spawner

int main(int argc, const char* argv[])
{
    std::vector<char> rodata;
    assert(argc == 4);
    auto rodatefile  = fopen(argv[1], "r");
    bool exitEof = std::string_view("exiteof")==argv[3];
    char c;

    while (std::fread(&c, 1, 1, rodatefile)>0)
        rodata.push_back(c);

    spawner::Spawner spawner(std::move(rodata));

    auto loglessfile = open(argv[2], O_RDONLY);

    if (-1 == loglessfile)
    {
        throw std::runtime_error("Failed to open log file!");
    }
    

    while (1)
    {
        // int rd = std::fread(&c, 1, 1, loglessfile);
        int rd = read(loglessfile, &c, 1);
        if (0 < rd)
        {
            spawner.in(c);
        }
        else if (0 == rd)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        else
        {
            using namespace std::string_literals;
            throw std::runtime_error("error reading: rd="s + std::to_string(rd) + " error=" + std::strerror(errno));
        }
    }
}