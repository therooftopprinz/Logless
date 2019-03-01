#include <string_view>
#include <iostream>
#include <iomanip>
#include <sstream>
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
        std::cout << "in[" << std::dec << std::setw(2) << std::setfill('0') << mReadSz << "]= "
                           << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData) << "\n";
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
                    std::cout << "state: Logpoint: " << mLogPoint << "\n";
                }
                break;
            }
            case State::Time:
            {
                if (sizeof(Logger::TagType) + sizeof(uint64_t) == mReadSz)
                {
                    std::memcpy(&mLogTime, mReadBuff + sizeof(Logger::TagType), sizeof(mLogTime));
                    mState = State::Thread;
                    mReadSz = 0;
                    std::cout << "state: Time: " << std::dec << mLogTime << "\n";
                }
                break;
            }
            case State::Thread:
            {
                if (sizeof(Logger::TagType) + sizeof(uint64_t) == mReadSz)
                {
                    std::memcpy(&mLogThread, mReadBuff + sizeof(Logger::TagType), sizeof(mLogThread));
                    std::cout << "state: Thread: " << std::dec << mLogThread << "\n";
                    mSs << std::hex << mLogTime << "us ";
                    mSs << std::hex << mLogThread << "t ";
                    mState = State::Tag;
                    mReadSz = 0;
                }
                break;
            }
            case State::Tag:
            {
                auto ntok = findNextToken('_', mLogPoint);
                size_t sglen = uintptr_t(ntok)-uintptr_t(mLogPoint);
                std::string_view logSeg(mLogPoint, sglen);
                std::cout << "state: Tag: " << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData) << "\n";
                std::cout << "seg:" << logSeg << "\n";
                mSs << logSeg;
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
                else
                {
                    std::cout << mSs.str() << "\n";
                    mSs.clear();
                    mState = State::Logpoint;
                }
                mReadSz = 0;
                break;
            }
            case State::Param8:
            {
                int8_t i;
                std::memcpy(&i, mReadBuff, sizeof(i));
                std::cout << "state: Param8: " << std::dec << i << "\n";
                mSs << std::dec << i;
                mState = State::Tag;
                mReadSz = 0;
            }
            case State::ParamU8:
            {
                uint8_t i;
                std::memcpy(&i, mReadBuff, sizeof(i));
                std::cout << "state: ParamU8: " << std::dec << i << "\n";
                mSs << std::dec << i;
                mState = State::Tag;
                mReadSz = 0;
            }
            case State::Param16:
            {
                if (sizeof(int16_t) == mReadSz)
                {
                    int16_t i;
                    std::memcpy(&i, mReadBuff, sizeof(i));
                    std::cout << "state: Param16: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: ParamU16: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: Param32: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: ParamU32: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: Param64: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: ParamU64: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: ParamFloat: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: ParamDouble: " << std::dec << i << "\n";
                    mSs << std::dec << i;
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
                    std::cout << "state: ParamVoidP: " << i << "\n";
                    mSs << i;
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
    std::stringstream mSs;
};

int main(int argc, const char* argv[])
{
    { 
        std::vector<uint8_t> roda = {0x01,0x00,0x02,0x00,0x02,0x00,0x00,0x00,0x77,0x62,0x00,0x6c,0x6f,0x67,0x2e,0x62,0x69,0x6e,0x00,0x4c,0x6f,0x67,0x67,0x65,0x72,0x3a,0x3a,0x4c,0x6f,0x67,0x67,0x65,0x72,0x0a,0x00,0x4c,0x6f,0x67,0x20,0x6d,0x65,0x20,0x70,0x6c,0x73,0x20,0x5f,0x20,0x5f,0x20,0x68,0x75,0x68,0x75,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,0x20,0x2c,0x20,0x5f,0x00,0x54,0x44,0x3a,0x20,0x00,0x0a,0x00,0x41,0x4c,0x4c,0x3a,0x20,0x00,0x49,0x4e,0x54,0x20,0x5f,0x20,0x46,0x4c,0x4f,0x41,0x54,0x20,0x5f,0x00,0x26,0x62,0x20,0x5f,0x20,0x74,0x68,0x69,0x73,0x00,0x25,0x6c,0x75,0x75,0x73,0x20,0x25,0x6c,0x75,0x74,0x20,0x00,0x4c,0x6f,0x67,0x67,0x65,0x72,0x52,0x65,0x66,0x58,0x44,0x00,0x00,0x00,0x66,0x66,0x86,0x40,0x25,0x64,0x00,0x25,0x66,0x00,0x25,0x70,0x00,0x00,0x00,0x00,0x4e,0x53,0x74,0x36,0x74,0x68,0x72,0x65,0x61,0x64,0x31,0x31,0x5f,0x53,0x74,0x61,0x74,0x65,0x5f,0x69,0x6d,0x70,0x6c,0x49,0x4e,0x53,0x5f,0x38,0x5f,0x49,0x6e,0x76,0x6f,0x6b,0x65,0x72,0x49,0x53,0x74,0x35,0x74,0x75,0x70,0x6c,0x65,0x49,0x4a,0x50,0x46,0x76,0x76,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x00};
        auto rodao = fopen("test.rodata", "w");
        fwrite((char*)roda.data(), 1, roda.size(), rodao);
        fclose(rodao);
    }

    std::vector<char> rodata;
    auto rodatefile  = fopen(argv[1], "r");
    auto loglessfile = fopen(argv[2], "r");
    char c;
    int rg = 0;

    while (std::fread(&c, 1, 1, rodatefile)>0)
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