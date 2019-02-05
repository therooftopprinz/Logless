#include <Logger.hpp>
#include <sys/mman.h>

void logtask()
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    for (int i=0; i<1024*1024; i++)
    {
        Logless(1, uint64_t(i), uint64_t(0xffff));
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    std::cout << "TD: " << timeNow - timeBase << "\n";
}

int main()
{
    mlockall(MCL_CURRENT|MCL_FUTURE);

    Logger::getInstance();
    using namespace std::literals::chrono_literals;

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

    Logger::getInstance().stop();
}