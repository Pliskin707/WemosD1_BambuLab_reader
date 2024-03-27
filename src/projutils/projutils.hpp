#ifndef __PROJUTILS_HPP__
#define __PROJUTILS_HPP__

#ifndef DEBUG_PRINT
#define dprintf(...) ;
#else
#define dprintf(fstr, ...) Serial.printf_P(PSTR(fstr), ##__VA_ARGS__);
#endif

#endif