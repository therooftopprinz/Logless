#include <Logger.hpp>

int main()
{
    Logger& logger = Logger::getInstance();

    logger.log(1, uint16_t(42), uint16_t(0xffff));
    logger.log(1, uint16_t(42), uint16_t(0xffff));

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(10ms);
}