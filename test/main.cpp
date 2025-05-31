#include <vector>
#include <logless/logger.hpp>
#include <sys/mman.h>

constexpr auto NLOGS = 250000;

using logless::LOGALL;
using logless::log;
using logless::DEBUG;
using logless::buffer_log_t;

logless::logger logger("log.bin");

void logtask(int div)
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    int logcount = NLOGS/div;
    double x = 1.23456789;
    float y = 1.23456789;
    uint64_t z = 99999999;

    for (auto i=0; i<logcount; i++)
    {
        log(logger, DEBUG, LOGALL, "Hello logger: msg number %d", i);
        // log(logger, DEBUG, LOGALL, "Hello logger: msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;, "
        //                               "msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;, "
        //                               "msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;, "
        //                               "msg number %d; here is a double %lf; and here is a float %f; and some 64 int %zu;.",
        //     i, x, y, z, i, x, y, z, i, x, y, z, i, x, y, z);
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    double logduration = double(timeNow - timeBase)/1000000;

    std::stringstream ss;
    ss << "logtask logcount=" << logcount << " logduration=" << logduration << "s lograte=" <<  (logcount/logduration)/1000000 << " megalogs/second \n";
    std::cout << ss.str();
}

void bm()
{
    using namespace std::literals::chrono_literals;

    logger.logless();

    std::cout << "1 thread " << NLOGS << " logs:\n";
    logtask(1);
    constexpr int NTHREAD = 4;
    std::cout << NTHREAD << " threads " << NLOGS << " logs:\n";
    
    std::vector<std::thread> logThread;

    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
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

    log(logger, DEBUG, LOGALL, "Log me pls %%;", buffer_log_t(20, buff2));
    log(logger, DEBUG, LOGALL, "Log me pls %c; and %d; and %x;", int8_t('a'), int8_t('a'), int8_t('a'));
    log(logger, DEBUG, LOGALL, "Log me pl  %d; and %x;", int8_t('a'), int8_t('a'));
    log(logger, DEBUG, LOGALL, "Log me pls %u;", uint8_t('a'));
    log(logger, DEBUG, LOGALL, "Log me pls %d;", int16_t(0xffff));
    log(logger, DEBUG, LOGALL, "Log me pls %u;", uint16_t(0xffff));
    log(logger, DEBUG, LOGALL, "Log me pls %d;", int32_t(0xffffffff));
    log(logger, DEBUG, LOGALL, "Log me pls %u;", uint32_t(0xffffffff));
    log(logger, DEBUG, LOGALL, "Log me pls %zd;", int64_t(0xfffffffffffffffful));
    log(logger, DEBUG, LOGALL, "Log me pls %zu;", uint64_t(0xfffffffffffffffful));
    log(logger, DEBUG, LOGALL, "Log me pls %f;", float(4.2));
    log(logger, DEBUG, LOGALL, "Log me pls %lf;", double(4.2));
    log(logger, DEBUG, LOGALL, "Log me pls %%;", buffer_log_t(6, buff));
    log(logger, DEBUG, LOGALL, "Log me pls %s;", "this iz string");

    bm();

    logger.flush();
}