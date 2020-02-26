#include <vector>
#include <Logger.hpp>
#include <sys/mman.h>

constexpr auto NLOGS = 250000;

void logtask(int div)
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint8_t buff[] = {0xde,0xad,0xbe,0xef,0xca,0xfe};
    int logcount = NLOGS/div;
    for (int i=0; i<logcount; i++)
    {
        // Logless("Hello logger: msg number _", i);
        Logless("Log me pls _ huhu _", i, BufferLog(6, buff));
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    double logduration = double(timeNow - timeBase)/1000000;

    std::stringstream ss;
    ss << "logtask logcount=" << NLOGS << " logduration=" << logduration << "s lograte=" <<  (logcount/logduration)/1000000 << " megalogs/second \n";
    std::cout << ss.str();
}

void runBM()
{
    using namespace std::literals::chrono_literals;

    Logger::getInstance().logless();

    std::cout << "1 thread " << NLOGS << " logs:\n";
    logtask(1);
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    constexpr int NTHREAD = 4;
    std::cout << NTHREAD << " threads " << NLOGS << " logs:\n";
    
    std::vector<std::thread> logThread;
    for (int i=0; i<NTHREAD; i++) logThread.emplace_back(logtask, 1);
    for (int i=0; i<NTHREAD; i++) logThread[i].join();

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    double logduration = double(timeNow - timeBase)/1000000;
    std::cout << "threaded logtask total logduration=" << logduration << "s lograte="<< (double(NLOGS*NTHREAD)/logduration)/1000000 << " megalogs/second\n";
}

int main()
{
    Logger::getInstance().logful();
    uint8_t buff[] = {0xde,0xad,0xbe,0xef,0xca,0xfe};
    uint8_t buff2[] = {0xde,0xad,0xbe,0xef,0xde,0xca,0xf8,0xed,0xca,0xfe,0xde,0xad,0xbe,0xef,0xde,0xca,0xf8,0xed,0xca,0xfe};

    Logless("Log me pls _", BufferLog(20, buff2));
    Logless("Log me pls _", int8_t('a'));
    Logless("Log me pls _", uint8_t('a'));
    Logless("Log me pls _", int16_t(0xffff));
    Logless("Log me pls _", uint16_t(0xffff));
    Logless("Log me pls _", int32_t(0xffffffff));
    Logless("Log me pls _", uint32_t(0xffffffff));
    Logless("Log me pls _", int64_t(0xfffffffffffffffful));
    Logless("Log me pls _", uint64_t(0xfffffffffffffffful));
    Logless("Log me pls _", float(4.2));
    Logless("Log me pls _", double(4.2));
    Logless("Log me pls _", BufferLog(6, buff));
    Logless("Log me pls _", "this iz string");

    runBM();

    Logger::getInstance().flush();
}