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
./spawner RODATA_OUT log.bin
```
You can render while the program is still running.
