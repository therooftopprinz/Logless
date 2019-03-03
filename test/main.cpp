#include <Logger.hpp>
#include <sys/mman.h>

void logtask(int div)
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint8_t buff[] = {0xde,0xad,0xbe,0xef,0xca,0xfe};

    for (int i=0; i<1024*1024/div; i++)
    {
        Logless("Hello logger: msg number _", i);
        // Logless("Log me pls _ huhu _", i, BufferLog(6, buff));
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    std::cout << "TD(/" << div << "): " << timeNow - timeBase << "\n";
}

void runBM()
{
    using namespace std::literals::chrono_literals;

    Logger::getInstance().logless();

    std::cout << "1 thread 1000000 logs:\n";
    logtask(1);
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    std::cout << "10 threads 1000000 logs:\n";
    std::thread a(logtask, 10);
    std::thread b(logtask, 10);
    std::thread c(logtask, 10);
    std::thread d(logtask, 10);
    std::thread e(logtask, 10);
    std::thread aa(logtask, 10);
    std::thread ab(logtask, 10);
    std::thread ac(logtask, 10);
    std::thread ad(logtask, 10);
    std::thread ae(logtask, 10);
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
    uint8_t buff2[] = {0xde,0xad,0xbe,0xef,0xde,0xca,0xf8,0xed,0xca,0xfe};

    Logless("Log me pls _", BufferLog(10, buff2));
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
}