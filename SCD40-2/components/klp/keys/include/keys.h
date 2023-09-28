#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>

typedef uint32_t myKey_t;

myKey_t getKeyPins( void);  // Low level function to provide
extern "C" void keysTimerHandler_ms (int ms);  // call this from timer (min 1ms , resolution 1ms )

extern myKey_t keysRT;
extern myKey_t keysRepeat;

myKey_t key (myKey_t key);

#define REPEATPROCESS_INTERVAL	10 //ms
#define DEBOUNCETIME 			50
#define KEYREPEAT_TIME			500


#endif



