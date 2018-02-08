// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#include "targetver.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
//#include <tchar.h>
#include <iostream>
using namespace std;
#define printf_s printf
#define sprintf_s sprintf
#define sscanf_s sscanf
#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#define INT_MAX       2147483647    /* maximum (signed) int value */
// TODO: reference additional headers your program requires here
#define _mkdir(path) mkdir((path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
