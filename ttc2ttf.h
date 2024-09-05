// ttc2ttf.cpp --- Split TTF file from TTC font collection
// License: MIT

#pragma once

#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#endif
#if defined(_WIN32) || defined(UNICODE)
    #include <windows.h>
#endif

#include "tchar.h"

typedef enum TTC2TTF_RET
{
    TTC2TTF_RET_NO_ERROR = 0,
    TTC2TTF_RET_NOT_TTC,
    TTC2TTF_RET_INVALID_ARGUMENTS,
    TTC2TTF_RET_READ_ERROR,
    TTC2TTF_RET_WRITE_ERROR,
    TTC2TTF_RET_INVALID_FORMAT,
    TTC2TTF_RET_OUT_OF_MEMORY,
    TTC2TTF_RET_BAD_FONT_INDEX,
    TTC2TTF_RET_LOGIC_ERROR,
} TTC2TTF_RET;

#ifdef __cplusplus
extern "C" {
#endif

void ttc2ttf_usage(void);
void ttc2ttf_version(void);

TTC2TTF_RET ttc2ttf_except(const _TCHAR *in_filename, int font_index, const _TCHAR *out_filename);
TTC2TTF_RET ttc2ttf_noexcept(const _TCHAR *in_filename, int font_index, const _TCHAR *out_filename);
TTC2TTF_RET ttc2ttf_main(int argc, _TCHAR **wargv);

#ifdef __cplusplus
} // extern "C"
#endif
