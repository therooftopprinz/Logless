#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

template<typename> struct TypeTraits;
template<> struct TypeTraits<uint8_t>  {static constexpr auto type_id = 0xf1;};
template<> struct TypeTraits<int8_t>   {static constexpr auto type_id = 0xf2;};
template<> struct TypeTraits<uint16_t> {static constexpr auto type_id = 0xf3;};
template<> struct TypeTraits<int16_t>  {static constexpr auto type_id = 0xf4;};
template<> struct TypeTraits<uint32_t> {static constexpr auto type_id = 0xf5;};
template<> struct TypeTraits<int32_t>  {static constexpr auto type_id = 0xf6;};
template<> struct TypeTraits<uint64_t> {static constexpr auto type_id = 0xf7;};
template<> struct TypeTraits<int64_t>  {static constexpr auto type_id = 0xf8;};
template<> struct TypeTraits<float>    {static constexpr auto type_id = 0xf9;};
template<> struct TypeTraits<double>   {static constexpr auto type_id = 0xfa;};

template <typename... Ts>
struct TotalSize
{
    static constexpr size_t value = (sizeof(Ts)+...);
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
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t BUFFER_COUNT = 1024;

    template<typename... Ts>
    void log(uint16_t id, const Ts&... ts)
    {
        constexpr size_t payloadSize = sizeof(uint16_t) + sizeof(uint8_t)*sizeof...(Ts) + TotalSize<Ts...>::value + sizeof(uint8_t);
        uint8_t* usedBuffer;
        int usedIdx = 0;

        {
            std::unique_lock<std::mutex> lock(mLogBufferMutex);

            if (mCurrentIdx + payloadSize <= BUFFER_SIZE)
            {
                usedIdx = mCurrentIdx;
                mCurrentIdx += payloadSize;
                mUseSize[mCurrentBuffer] = mCurrentIdx;
            }
            else
            {
                mUseSize[mCurrentBuffer] = mCurrentIdx;
                mCurrentIdx = payloadSize;
                mCurrentBuffer = (mCurrentBuffer+1) % BUFFER_COUNT;
                mLogBufferFlushCv.notify_one();
            }
        }

        usedBuffer = mLogBuffer[mCurrentBuffer].data();
        // LOG HEADER
        new (usedBuffer + usedIdx) uint16_t(id);
        // std::cout << "tolog[" << payloadSize << "] @("<< (void*)usedBuffer << "+" << std::dec << usedIdx << "): "  << toHexString(usedBuffer, payloadSize) << "\n";
        usedIdx += sizeof(uint16_t);
        log(usedBuffer, usedIdx, ts...);
    }

    static Logger& getInstance()
    {
        static Logger logger{};
        return logger;
    }

private:

    Logger()
        : mOutputStream(std::fstream("log.bin", std::ios::binary))
    {
        std::cout << "Logger::Logger\n";
        mRun = std::thread(&Logger::run, this);
    }

    ~Logger()
    {
        std::cout << "Logger::~Logger\n";
        std::unique_lock<std::mutex> lock(mLogBufferMutex);
        mRunning = false;
        lock.unlock();
        mLogBufferFlushCv.notify_one();
        mRun.join();
        mOutputStream.close();
        std::cout << "Total bytes logged: " << mLoggedBytes << "\n";
    }

    void log(uint8_t* pUsedBuffer, int& pUsedIndex)
    {
        // LOG TAIL
        new (pUsedBuffer+pUsedIndex) uint8_t (0);
    }

    template<typename T, typename... Ts>
    void log(uint8_t* pUsedBuffer, int& pUsedIndex, T t, Ts... ts)
    {
        new (pUsedBuffer + pUsedIndex) uint8_t(TypeTraits<T>::type_id);
        pUsedIndex++;
        new (pUsedBuffer + pUsedIndex) T(t);
        pUsedIndex += sizeof(T);
        log(pUsedBuffer, pUsedIndex, ts...);
    }

    void run()
    {
        mRunning = true;
        while(mRunning)
        {
            std::unique_lock<std::mutex> lock(mLogBufferMutex);
            mLogBufferFlushCv.wait(lock);
            lock.unlock();

            while (mCurrentLogBuffer != mCurrentBuffer)
            {
                mOutputStream.write((char*)mLogBuffer[mCurrentLogBuffer].data(), mUseSize[mCurrentLogBuffer]);
                std::cout << "logging[" << mUseSize[mCurrentLogBuffer] << "]: @" << (void*)(mLogBuffer[mCurrentLogBuffer].data()) << " "  << toHexString(mLogBuffer[mCurrentLogBuffer].data(), mUseSize[mCurrentLogBuffer]) << "\n";
                mLoggedBytes += mUseSize[mCurrentLogBuffer];
                mCurrentLogBuffer = (mCurrentLogBuffer+1)%BUFFER_COUNT;
            }
        }

        mOutputStream.write((char*)mLogBuffer[mCurrentLogBuffer].data(), mUseSize[mCurrentLogBuffer]);
        std::cout << "logging[" << mUseSize[mCurrentLogBuffer] << "]: @" << (void*)(mLogBuffer[mCurrentLogBuffer].data()) << " "  << toHexString(mLogBuffer[mCurrentLogBuffer].data(), mUseSize[mCurrentLogBuffer]) << "\n";
        mLoggedBytes += mUseSize[mCurrentLogBuffer];
    }

    std::array<std::array<uint8_t, BUFFER_SIZE>, BUFFER_COUNT> mLogBuffer;
    std::array<size_t, BUFFER_SIZE> mUseSize;
    std::mutex mLogBufferMutex;
    std::condition_variable mLogBufferFlushCv;
    int mCurrentBuffer = 0;
    int mCurrentLogBuffer = 0;
    int mCurrentIdx = 0;
    bool mRunning = false;
    size_t mLoggedBytes = 0;
    std::thread mRun;
    std::fstream mOutputStream;
};

template <typename... Ts>
void Logless(uint16_t id, Ts... ts)
{
    static uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t threadId = std::hash<std::thread::id>()(std::this_thread::get_id());
    Logger::getInstance().log(id, timeNow, threadId, ts...);
}

#endif // __LOGGER_HPP__