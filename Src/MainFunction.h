#pragma once
#include <iostream>
using namespace std;
#include <map>
#include "Util.h"
int DispatchFunction(int argc, char * argv[]);
int DummyTemp(int argc, char * argv[]);

typedef map<string, string, StringCompObj>	ArgName2Arg;
int ExtractArgument(int argc, char * argv[], ArgName2Arg & rmapName2Arg);
