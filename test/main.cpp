#include <vector>
#include <logless/Logger.hpp>
#include <sys/mman.h>

constexpr auto NLOGS = 1000000;

Logger logger("log.bin");

void logtask(int div)
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    int logcount = NLOGS/div;
    double x = 1.23456789;
    float y = 1.23456789;
    uint64_t z = 99999999;

    for (auto i=0; i<logcount; i++)
    {
        Logless(logger, "Hello logger: msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;, "
                                      "msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;, "
                                      "msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;, "
                                      "msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;.",
            i, x, y, z, i, x, y, z, i, x, y, z, i, x, y, z);
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    double logduration = double(timeNow - timeBase)/1000000;

    std::stringstream ss;
    ss << "logtask logcount=" << logcount << " logduration=" << logduration << "s lograte=" <<  (logcount/logduration)/1000000 << " megalogs/second \n";
    std::cout << ss.str();
}

void runBM()
{
    using namespace std::literals::chrono_literals;

    logger.logless();

    std::cout << "1 thread " << NLOGS << " logs:\n";
    logtask(1);
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    constexpr int NTHREAD = 4;
    std::cout << NTHREAD << " threads " << NLOGS << " logs:\n";
    
    std::vector<std::thread> logThread;
    for (int i=0; i<NTHREAD; i++) logThread.emplace_back(logtask, NTHREAD);
    for (int i=0; i<NTHREAD; i++) logThread[i].join();

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    double logduration = double(timeNow - timeBase)/1000000;
    std::cout << "threaded logtask total logduration=" << logduration << "s lograte="<< (double(NLOGS)/logduration)/1000000 << " megalogs/second\n";
}

int main()
{
    logger.logful();
    uint8_t buff[] = {0xde,0xad,0xbe,0xef,0xca,0xfe};
    uint8_t buff2[] = {0xde,0xad,0xbe,0xef,0xde,0xca,0xf8,0xed,0xca,0xfe,0xde,0xad,0xbe,0xef,0xde,0xca,0xf8,0xed,0xca,0xfe};

    Logless(logger, "Log me pls %%;", BufferLog(20, buff2));
    Logless(logger, "Log me pls %c; and %d; and %x;", int8_t('a'), int8_t('a'), int8_t('a'));
    Logless(logger, "Log me pl  %d; and %x;", int8_t('a'), int8_t('a'));
    Logless(logger, "Log me pls %u;", uint8_t('a'));
    Logless(logger, "Log me pls %d;", int16_t(0xffff));
    Logless(logger, "Log me pls %u;", uint16_t(0xffff));
    Logless(logger, "Log me pls %d;", int32_t(0xffffffff));
    Logless(logger, "Log me pls %u;", uint32_t(0xffffffff));
    Logless(logger, "Log me pls %zd;", int64_t(0xfffffffffffffffful));
    Logless(logger, "Log me pls %zu;", uint64_t(0xfffffffffffffffful));
    Logless(logger, "Log me pls %f;", float(4.2));
    Logless(logger, "Log me pls %lf;", double(4.2));
    Logless(logger, "Log me pls %%;", BufferLog(6, buff));
    Logless(logger, "Log me pls %s;", "this iz string");

    runBM();

    logger.flush();
}