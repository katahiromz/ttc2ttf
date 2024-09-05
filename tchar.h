#pragma once

#ifdef _WIN32
	#include <tchar.h>
#else
    typedef char _TCHAR;
    #define _T(str) str
    #define _ttoi atoi
    #define _tprintf printf
    #define _ftprintf fprintf
    #define _tfopen fopen
    #define _tcscmp strcmp
    #define _tmain main
#endif
