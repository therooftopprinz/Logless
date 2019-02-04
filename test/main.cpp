#include <Logger.hpp>

void logtask()
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    for (int i=0; i<1024*1024; i++)
    {
        Logless(1, uint32_t(i), uint16_t(0xffff));
    }

    uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    std::cout << "TD: " << timeNow - timeBase << "\n";
}

int main()
{
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

    // for (int i=0; i<1024*1024; i++)
    // {
    //     Logless(1, uint32_t(i), uint16_t(0xffff));
    // }

    Logger::getInstance().stop();
}