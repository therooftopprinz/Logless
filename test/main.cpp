#include <Logger.hpp>
#include <sys/mman.h>

void logtask()
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint8_t buff[] = {0xde,0xad,0xbe,0xef,0xca,0xfe};

    for (int i=0; i<1024*1024/10; i++)
    {
        Logless("Log me pls _ huhu _", i, BufferLog(6, buff));
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    std::cout << "TD: " << timeNow - timeBase << "\n";
}

void runBM()
{
    using namespace std::literals::chrono_literals;

    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    Logger::getInstance().logless();

    std::thread a(logtask);
    std::thread b(logtask);
    std::thread c(logtask);
    std::thread d(logtask);
    std::thread e(logtask);
    std::thread aa(logtask);
    std::thread ab(logtask);
    std::thread ac(logtask);
    std::thread ad(logtask);
    std::thread ae(logtask);
    a.join();
    b.join();
    c.join();
    d.join();
    e.join();
    aa.join();
    ab.join();
    ac.join();
    ad.join();
    ae.join();

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    std::cout << "ALL: " << timeNow - timeBase << "\n";
}

int main()
{
    // mlockall(MCL_CURRENT|MCL_FUTURE);
    Logger::getInstance().logful();
    uint8_t buff[] = {0xde,0xad,0xbe,0xef,0xca,0xfe};

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

    runBM();
}