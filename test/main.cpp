    #include <Logger.hpp>
#include <sys/mman.h>

void logtask()
{
    uint64_t timeBase = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    for (int i=0; i<1024*1024/10; i++)
    {
        Logless("Log me pls _ _ huhu", i, 0xffff);
        Logless("                    _ , _", i, (void*)0xffff);
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

    Logger::getInstance();
    int a = 42;
    float b = 4.2;
    Logless("INT _ FLOAT _", a, b);
    
    // runBM();

}