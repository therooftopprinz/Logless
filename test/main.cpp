#include <Logger.hpp>

void logtask()
{
    for (int i=0; i<1024*8; i++)
    {
        Logless(1, uint32_t(i), uint16_t(0xffff));
    }
}

int main()
{
    std::thread a(logtask);
    std::thread b(logtask);
    std::thread c(logtask);
    std::thread d(logtask);
    a.join();
    b.join();
    c.join();
    d.join();
}