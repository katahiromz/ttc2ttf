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

#ifdef UNICODE
    typedef std::wstring tstring;

    template <typename T_VALUE>
    inline tstring to_tstring(const T_VALUE& value)
    {
        return std::to_wstring(value);
    }
#else
    typedef std::string tstring;

    template <typename T_VALUE>
    inline tstring to_tstring(const T_VALUE& value)
    {
        return std::to_string(value);
    }
#endif

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

bool file_read_all(const tstring& filename, std::vector<char>& content);
bool file_write_all(const tstring& filename, const void *ptr, size_t size);

void ttc2ttf_usage(void);
void ttc2ttf_version(void);
int ttc2ttf_get_ttf_count(const std::vector<char>& input);
TTC2TTF_RET ttc2ttf_data_from_data(std::vector<char>& output, const std::vector<char>& input, int font_index);
TTC2TTF_RET ttc2ttf_data_from_file(std::vector<char>& output, const _TCHAR *in_filename, int font_index);
TTC2TTF_RET ttc2ttf_file_from_file(const _TCHAR *out_filename, const _TCHAR *in_filename, int font_index);
TTC2TTF_RET ttc2ttf_main(int argc, _TCHAR **wargv);
