# Logless

Don't log? Logless

```cpp
#include <Logger.hpp>
...
Logless(SOME_ENUM_VALUE_xX_xY_xZ_..., xX, xY, xZ, ...);
```

## Format
```
[(H)(TV)+(E)]+

Where:
H (uint16_t) - Log Point Id
T (uint8_t) - Value Type {see Logger.hpp}
V (8,16,32 bit/signed/unsigned, float, double) - value
E (uint8_t) - Log Pointer Ender
```

## Soon
`x/Nxb` logging for buffers