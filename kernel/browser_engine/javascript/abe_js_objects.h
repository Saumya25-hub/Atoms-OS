#ifndef ABE_JS_OBJECTS_H
#define ABE_JS_OBJECTS_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_JSObjects_Init(void);
ABE_Error ABE_JSObjects_Shutdown(void);

// Standard Library Math Operations
double ABE_JSMath_Floor(double v);
double ABE_JSMath_Ceil(double v);
double ABE_JSMath_Round(double v);
double ABE_JSMath_Max(double a, double b);
double ABE_JSMath_Min(double a, double b);

// Console Output Bridge
void ABE_JSConsole_Log(const char* msg);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_OBJECTS_H
