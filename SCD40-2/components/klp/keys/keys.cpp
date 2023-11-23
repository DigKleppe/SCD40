#include "keys.h"

#define TMR_OFF				-1

static myKey_t keysAreRead;
myKey_t keysRT;
myKey_t keysRepeat;


static void processKeys (void);


__attribute__((weak)) myKey_t getKeyPins(void){
	return 0;
}

__attribute__((weak)) myKey_t pinsToBits(myKey_t in){
	return in;
}




/**********************************************************************
 reads and debounces keys
 output: keysRT
 call periodically

 **********************************************************************/
extern "C"
{
void keysTimerHandler_ms (int ms)
{
	static int debounceTmr = DEBOUNCETIME;
	static myKey_t oldInputs = 0;
	static int repTmr = REPEATPROCESS_INTERVAL;
	myKey_t inputKeyBuffer = 0;

	inputKeyBuffer = getKeyPins();

	if (inputKeyBuffer != oldInputs) // changed"?
		debounceTmr = DEBOUNCETIME; // yes, debounce
	else
	{ // no , put in Real Time buffer
		if (debounceTmr <= 0)
			keysRT = pinsToBits( inputKeyBuffer);
		else
			debounceTmr -= ms;
	}
	oldInputs = inputKeyBuffer;
	inputKeyBuffer = 0;
	repTmr -= ms;

	if (repTmr <= 0){
		repTmr = REPEATPROCESS_INTERVAL;
		processKeys(); // repeating keys after debounce
	}
}

}
// repeating keys

static void processKeys (void)
{
	myKey_t keyBuffer;
	static int keyRepTmr = TMR_OFF;

	keyBuffer = keysRT; // vanwege interrupt
	keysAreRead &= keyBuffer; // wis gelezenvlaggen niet ingedrukte toetsen

	if (keyBuffer & keysRepeat)
	{
		if (keyRepTmr == TMR_OFF)
		{
			keyRepTmr = (KEYREPEAT_TIME/REPEATPROCESS_INTERVAL);
		}
		else
		{
			keyRepTmr--;
			if (!keyRepTmr)
			{
				keyRepTmr = KEYREPEAT_TIME/REPEATPROCESS_INTERVAL;
				keysAreRead &= ~keysRepeat; // wis keysAreRead vlaggen van repet. toets
			}
		}
	}
	else
		keyRepTmr = TMR_OFF;
}


myKey_t key (myKey_t key)
{
	myKey_t result = false;
	keysAreRead &= keysRT;
	result = key & keysRT & ~keysAreRead;
	keysAreRead |= result;
	return (result);
}
