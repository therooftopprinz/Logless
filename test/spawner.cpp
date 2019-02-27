#include <string_view>
#include <iostream>
#include <vector>
#include <thread>
#include <cstdio>
#include <cstring>
#include <Logger.hpp>

class Spawner
{
    enum class State {Logpoint, Time, Thread, Tag, Param8, ParamU8, Param16, ParamU16, Param32, ParamU32, Param64, ParamU64, ParamFloat, ParamDouble, ParamVoidP};
public:
    Spawner(std::vector<char>&& pRodata)
        : mRodata(std::move(pRodata))
        , mRefPos(std::string_view((char*)mRodata.data(), mRodata.size()).find("LoggerRefXD"))
    {
        if (std::string_view::npos == mRefPos)
            throw std::runtime_error("LoggerRefXD not found in rodata");
    }
    void in(uint8_t pData)
    {
        mReadBuff[mReadSz++] = pData;
        switch (mState)
        {
            case State::Logpoint:
            {
                if (sizeof(int) == mReadSz)
                {
                    int i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    mLogPoint = mRodata.data() + mRefPos + i;
                    mState = State::Time;
                    mReadSz = 0;
                }
                break;
            }
            case State::Time:
            {
                if (sizeof(uint64_t) == mReadSz)
                {
                    std::memcpy(&mLogTime, mReadBuff, sizeof(mLogTime));
                    mState = State::Thread;
                    mReadSz = 0;
                }
                break;
            }
            case State::Thread:
            {
                if (sizeof(uint64_t) == mReadSz)
                {
                    std::memcpy(&mLogThread, mReadBuff, sizeof(mLogThread));
                    mReadSz = 0;
                    std::cout << mLogTime << "us ";
                    std::cout << mLogThread << "t ";
                    mState = State::Tag;
                }
                break;
            }
            case State::Tag:
            {
                auto ntok = findNextToken('_', mLogPoint);
                size_t sglen = uintptr_t(ntok)-uintptr_t(mLogPoint);
                std::string_view logSeg(mLogPoint, sglen);
                std::cout << logSeg;
                if (*ntok) mLogPoint = ntok + 1;
                if (TypeTraits<uint8_t>::type_id == pData)         mState = State::Param8;
                else if (TypeTraits<int8_t>::type_id == pData)     mState = State::ParamU8;
                else if (TypeTraits<uint16_t>::type_id == pData)   mState = State::Param16;
                else if (TypeTraits<int16_t>::type_id == pData)    mState = State::ParamU16;
                else if (TypeTraits<uint32_t>::type_id == pData)   mState = State::Param32;
                else if (TypeTraits<int32_t>::type_id == pData)    mState = State::ParamU32;
                else if (TypeTraits<uint64_t>::type_id == pData)   mState = State::Param64;
                else if (TypeTraits<int64_t>::type_id == pData)    mState = State::ParamU64;
                else if (TypeTraits<float>::type_id == pData)      mState = State::ParamFloat;
                else if (TypeTraits<double>::type_id == pData)     mState = State::ParamDouble;
                else if (TypeTraits<void*>::type_id == pData)      mState = State::ParamVoidP;
                else                                               mState = State::Logpoint;
                mReadSz = 0;
                break;
            }
            case State::Param8:
            {
                int8_t i;
                std::memcpy(&i, mReadBuff, sizeof(i));
                std::cout << i;
                mState = State::Tag;
                mReadSz = 0;
            }
            case State::ParamU8:
            {
                uint8_t i;
                std::memcpy(&i, mReadBuff, sizeof(i));
                std::cout << i;
                mState = State::Tag;
                mReadSz = 0;
            }
            case State::Param16:
            {
                if (sizeof(int16_t) == mReadSz)
                {
                    int16_t i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::ParamU16:
            {
                if (sizeof(uint16_t) == mReadSz)
                {
                    uint16_t i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::Param32:
            {
                if (sizeof(int32_t) == mReadSz)
                {
                    int32_t i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::ParamU32:
            {
                if (sizeof(uint32_t) == mReadSz)
                {
                    uint32_t i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::Param64:
            {
                if (sizeof(int64_t) == mReadSz)
                {
                    int64_t i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::ParamU64:
            {
                if (sizeof(uint64_t) == mReadSz)
                {
                    uint64_t i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::ParamFloat:
            {
                if (sizeof(float) == mReadSz)
                {
                    float i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::ParamDouble:
            {
                if (sizeof(double) == mReadSz)
                {
                    float i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::ParamVoidP:
            {
                if (sizeof(void*) == mReadSz)
                {
                    void* i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << i;
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
        }
    }
private:
    const char* findNextToken(char pTok, const char* pStr)
    {
        while (*pStr!=0&&*pStr!=pTok)
        {
            pStr++;
        }
        return pStr;
    }

    std::vector<char> mRodata;
    State mState = State::Logpoint;
    char mReadBuff[128];
    size_t mReadSz = 0;
    int mRefPos;
    // logline
    const char* mLogPoint = nullptr;
    uint64_t mLogTime;
    uint64_t mLogThread;
};

int main(int argc, const char* argv[])
{
    std::vector<char> rodata;
    auto rodatefile  = fopen(argv[1], "r");
    auto loglessfile = fopen(argv[2], "r");
    char c;
    int rg = 0;

    while (std::fread(&c, 1, 1, loglessfile)>0)
        rodata.push_back(c);

    Spawner spawner(std::move(rodata));

    while (1)
    {
        int rd = std::fread(&c, 1, 1, loglessfile);
        if (rd)
        {
            spawner.in(c);
            rg++;
        }
        else
        {
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(5ms); // why is this needed?
        }
    }
}