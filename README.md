# Logless

Don't log? Logless

```cpp
#include <Logger.hpp>
...
Logless("Your log here xX=_ xY=_ xZ=_...", xX, xY, xZ, ...);
Logger::getInstance().logless(); // ULTRA FAST MODE, LOG ONLY TO FILE
Logger::getInstance().logful(); // SLOW MODE, LOG BOTH STDOUT AND FILE
```

## Format
```
[(H)(TV)*(E)]+

Where:
H (uint16_t) - Log Point Id
T (uint8_t) - Value Type {see Logger.hpp}
V ({8,16,32 bit/signed/unsigned}, float, double, {uint16_t N, uint8_t N}) - value
E (uint8_t) - Log Pointer Ender
```

## How to render

```
# 1st run your program it will output to log.bin
# 2nd generate the string data from binary image
objcopy -O binary --only-section=.rodata YOUR_BINARY RODATA_OUT
# 3rd spawn it
./spawner RODATA_OUT log.bin exiteof
```
You can render while the program is still running.

## Building test
```
ubuntu@ubuntu:~/Development/Logless/$ mkdir build
ubuntu@ubuntu:~/Development/Logless/build$ cd build
ubuntu@ubuntu:~/Development/Logless/build$ ../configure.py
configuring for testing
TLD is ../
PWD is /home/ubuntu/Development/Logless/build/
LOGLESS_SOURCES []
SPAWNER_SOURCES []
['logless.a_build/Logger.cpp.o']
['logless.a_build/Logger.cpp.d']
['../src/Logger.cpp']
['spawner_build/spawner.cpp.o']
['spawner_build/spawner.cpp.d']
['../src/spawner.cpp']
['test_build/main.cpp.o']
['test_build/main.cpp.d']
['../test/main.cpp']
ubuntu@ubuntu:~/Development/Logless/build$ make test_run
Building logless.a_build/Logger.cpp.o..
ar rcs  logless.a logless.a_build/Logger.cpp.o
Building spawner_build/spawner.cpp.o..
g++ spawner_build/spawner.cpp.o logless.a -lpthread -o spawner
Building test_build/main.cpp.o..
g++ test_build/main.cpp.o logless.a -lpthread -o test
objcopy -O binary --only-section=.rodata test test.rodata
./test
Logger::Logger
1568869131567752us 16363694757129654890t Log me pls deadbeefdecaf8edcafedeadbeefdecaf8edcafe
1568869131567838us 16363694757129654890t Log me pls a
1568869131567883us 16363694757129654890t Log me pls a
1568869131567924us 16363694757129654890t Log me pls -1
1568869131567987us 16363694757129654890t Log me pls 65535
1568869131568041us 16363694757129654890t Log me pls -1
1568869131568091us 16363694757129654890t Log me pls 4294967295
1568869131568141us 16363694757129654890t Log me pls 18446744073709551615
1568869131568212us 16363694757129654890t Log me pls -1
1568869131568274us 16363694757129654890t Log me pls 4.200000
1568869131568344us 16363694757129654890t Log me pls 4.200000
1568869131568383us 16363694757129654890t Log me pls deadbeefcafe
1568869131568419us 16363694757129654890t Log me pls this iz string
1 thread 250000 logs:
logtask logcount=250000 logduration=0.13304s lograte=1.87913 megalogs/second 
8 threads 250000 logs:
logtask logcount=250000 logduration=1.0843s lograte=0.230564 megalogs/second 
logtask logcount=250000 logduration=1.08342s lograte=0.23075 megalogs/second 
logtask logcount=250000 logduration=1.08083s lograte=0.231305 megalogs/second 
logtask logcount=250000 logduration=1.09861s lograte=0.22756 megalogs/second 
logtask logcount=250000 logduration=1.11233s lograte=0.224753 megalogs/second 
logtask logcount=250000 logduration=1.10954s lograte=0.225318 megalogs/second 
logtask logcount=250000 logduration=1.09136s lograte=0.229072 megalogs/second 
logtask logcount=250000 logduration=1.09967s lograte=0.227341 megalogs/second 
threaded logtask total logduration=1.12073s lograte=1.78456 megalogs/second
Logger::~Logger
./spawner test.rodata log.bin exiteof > test.log
echo log is written at test.log
log is written at test.log
ubuntu@ubuntu:~/Development/Logless/build$
```
