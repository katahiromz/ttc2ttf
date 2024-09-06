// ttc2ttf.cpp --- Split TTF file from TTC font collection
// License: MIT
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include "ttc2ttf.h"

void ttc2ttf_usage(void)
{
    _tprintf(_T("Usage: ttc2ttf input.ttc [font_index output.ttf]\n"));
}

void ttc2ttf_version(void)
{
    _tprintf(_T("ttc2ttf Version 0.9 by katahiromz\n"));
    _tprintf(_T("License: MIT\n"));
}

static inline uint32_t u32_ceil4(uint32_t n)
{
    return (n + 3) & ~3;
}

static inline int is_big_endian(void)
{
    union
    {
        unsigned int i;
        char c[4];
    } test_union = { 0x01020304 };
    return test_union.c[0] == 1;
}

static inline uint32_t u32_swab(uint32_t value)
{
    uint32_t new_value = (value >> 24);
    new_value |= ((value >> 16) & 0xFF) << 8;
    new_value |= ((value >> 8) & 0xFF) << 16;
    new_value |= (value & 0xFF) << 24;
    return new_value;
}

static inline uint16_t u16_swab(uint16_t value)
{
    uint16_t new_value = (value >> 8);
    new_value |= (value & 0xFF) << 8;
    return new_value;
}

static inline uint32_t u32_endian_fix(uint32_t value)
{
    if (is_big_endian())
        return value;
    return u32_swab(value);
}

static inline uint16_t u16_endian_fix(uint16_t value)
{
    if (is_big_endian())
        return value;
    return u16_swab(value);
}

template <typename T_INPUT>
static inline uint16_t u16_get(const T_INPUT& input, size_t offset)
{
    input.at(offset + sizeof(uint16_t) - 1);
    return u16_endian_fix(*reinterpret_cast<const uint16_t *>(&input.at(offset)));
}

template <typename T_INPUT>
static inline uint32_t u32_get(const T_INPUT& input, size_t offset)
{
    input.at(offset + sizeof(uint32_t) - 1);
    return u32_endian_fix(*reinterpret_cast<const uint32_t *>(&input.at(offset)));
}

template <typename T_OUTPUT>
static inline void u32_set(T_OUTPUT& output, size_t offset, uint32_t value)
{
    output.at(offset + sizeof(uint32_t) - 1);
    *reinterpret_cast<uint32_t *>(&output.at(offset)) = u32_endian_fix(value);
}

bool file_read_all(const tstring& filename, std::vector<char>& content)
{
    struct _stat st;
    size_t size = 0;
    if (!_tstat(filename.c_str(), &st))
        size = st.st_size;

    FILE *fin = _tfopen(filename.c_str(), _T("rb"));
    if (!fin)
        return false;

    bool ret = true;
    content.resize(size);
    if (size)
        ret = fread(content.data(), size, 1, fin);

    fclose(fin);
    return ret;
}

bool file_write_all(const tstring& filename, const void *ptr, size_t size)
{
    FILE *fout = _tfopen(filename.c_str(), _T("wb"));
    if (!fout)
        return false;

    size_t written = fwrite(ptr, size, 1, fout);
    fclose(fout);
    return !!written;
}

void ttc2ttf_extract(std::vector<char>& output, const std::vector<char>& input, uint32_t ttf_offset, int index)
{
    _tprintf(_T("Extract TTF #%d\n"), index);
    _tprintf(_T("\tHeader offset %d\n"), ttf_offset);

    uint16_t table_count = u16_get(input, ttf_offset + 4);
    uint32_t header_length = 12 + table_count * 16;
    _tprintf(_T("\tHeader size %d bytes\n"), header_length);

    uint32_t table_length = 0;
    for (uint32_t j = 0; j < table_count; ++j)
    {
        uint32_t length = u32_get(input, ttf_offset + 12 + 12 + j * 16);
        table_length += u32_ceil4(length);
    }

    uint32_t total_length = header_length + table_length;
    output.resize(total_length);
    std::memcpy(output.data(), &input.at(ttf_offset), header_length);
    uint32_t current_offset = header_length;

    for (uint32_t j = 0; j < table_count; ++j)
    {
        uint32_t offset = u32_get(input, ttf_offset + 12 + 8 + j * 16);
        uint32_t length = u32_get(input, ttf_offset + 12 + 12 + j * 16);
        u32_set(output, 12 + 8 + j * 16, current_offset);
        std::memcpy(&output.at(current_offset), &input.at(offset), length);
        current_offset += u32_ceil4(length);
    }
}

int ttc2ttf_get_ttf_count(const std::vector<char>& input)
{
    // not a TTC file?
    if (input.size() < 4 || std::memcmp(input.data(), "ttcf", 4) != 0)
        return -1;

    // get the count of fonts
    return (int)u32_get(input, 8);
}

TTC2TTF_RET ttc2ttf_data_from_data(std::vector<char>& output, const std::vector<char>& input, int font_index)
{
    // not a TTC file?
    int ttf_count = ttc2ttf_get_ttf_count(input);
    if (ttf_count < 0)
        return TTC2TTF_RET_NOT_TTC;

    // Bad font_index?
    if (!(0 <= font_index && font_index < ttf_count))
        return TTC2TTF_RET_BAD_FONT_INDEX;

    // get offsets
    uint32_t ttf_offset = u32_get(input, 12 + font_index * sizeof(uint32_t));

    // extract specific ttf
    ttc2ttf_extract(output, input, ttf_offset, font_index);

    // done
    return TTC2TTF_RET_NO_ERROR;
}

TTC2TTF_RET ttc2ttf_data_from_file(std::vector<char>& output, const _TCHAR *in_filename, int font_index)
{
    try
    {
        std::vector<char> input;
        if (!file_read_all(in_filename, input))
            return TTC2TTF_RET_READ_ERROR;

        return ttc2ttf_data_from_data(output, input, font_index);
    }
    catch (const std::out_of_range& e)
    {
        return TTC2TTF_RET_INVALID_FORMAT;
    }
    catch (const std::bad_alloc& e)
    {
        return TTC2TTF_RET_OUT_OF_MEMORY;
    }
    catch (const std::logic_error& e)
    {
        return TTC2TTF_RET_LOGIC_ERROR;
    }
}

TTC2TTF_RET ttc2ttf_file_from_file(const _TCHAR *out_filename, const _TCHAR *in_filename, int font_index)
{
    try
    {
        std::vector<char> input;
        if (!file_read_all(in_filename, input))
            return TTC2TTF_RET_READ_ERROR;

        if (font_index == -1)
        {
            assert(!out_filename);

            int ttf_count = ttc2ttf_get_ttf_count(input);
            if (ttf_count < 0)
                return TTC2TTF_RET_NOT_TTC;

            for (font_index = 0; font_index < ttf_count; ++font_index)
            {
                std::vector<char> output;
                TTC2TTF_RET ret = ttc2ttf_data_from_data(output, input, font_index);
                if (ret != TTC2TTF_RET_NO_ERROR)
                    return ret;

                auto out_name = tstring(_T("font")) + to_tstring(font_index) + _T(".ttf");
                if (!file_write_all(out_name.c_str(), output.data(), output.size()))
                    return TTC2TTF_RET_WRITE_ERROR;
            }
        }
        else
        {
            std::vector<char> output;
            TTC2TTF_RET ret = ttc2ttf_data_from_data(output, input, font_index);
            if (ret != TTC2TTF_RET_NO_ERROR)
                return ret;

            if (!file_write_all(out_filename, output.data(), output.size()))
                return TTC2TTF_RET_WRITE_ERROR;
        }

        return TTC2TTF_RET_NO_ERROR;
    }
    catch (const std::out_of_range& e)
    {
        return TTC2TTF_RET_INVALID_FORMAT;
    }
    catch (const std::bad_alloc& e)
    {
        return TTC2TTF_RET_OUT_OF_MEMORY;
    }
    catch (const std::logic_error& e)
    {
        return TTC2TTF_RET_LOGIC_ERROR;
    }
}

TTC2TTF_RET ttc2ttf_main(int argc, _TCHAR **wargv)
{
    TTC2TTF_RET ret;
    const _TCHAR *in_filename = NULL;
    const _TCHAR *out_filename = NULL;
    int font_index = -1;
    if (argc == 2)
    {
        if (_tcscmp(wargv[1], "--help") == 0)
        {
            ttc2ttf_usage();
            return TTC2TTF_RET_NO_ERROR;
        }
        if (_tcscmp(wargv[1], "--version") == 0)
        {
            ttc2ttf_version();
            return TTC2TTF_RET_NO_ERROR;
        }
        in_filename = wargv[1];
    }
    else if (argc == 4)
    {
        in_filename = wargv[1];
        font_index = _ttoi(wargv[2]);
        out_filename = wargv[3];
    }
    else
    {
        ttc2ttf_usage();
        return TTC2TTF_RET_INVALID_ARGUMENTS;
    }

    ret = ttc2ttf_file_from_file(out_filename, in_filename, font_index);
    switch (ret)
    {
    case TTC2TTF_RET_NO_ERROR:
        break;
    case TTC2TTF_RET_NOT_TTC:
        _ftprintf(stderr, "Error: Not a TTC file\n");
        break;
    case TTC2TTF_RET_INVALID_ARGUMENTS:
        _ftprintf(stderr, _T("Error: Invalid arguments\n"));
        ttc2ttf_usage();
        break;
    case TTC2TTF_RET_READ_ERROR:
        _ftprintf(stderr, _T("Error: Unable to read file: %s\n"), in_filename);
        break;
    case TTC2TTF_RET_WRITE_ERROR:
        if (out_filename)
            _ftprintf(stderr, _T("Error: Unable to write file: %s\n"), out_filename);
        else
            _ftprintf(stderr, _T("Error: Unable to write file\n"));
        break;
    case TTC2TTF_RET_INVALID_FORMAT:
        _ftprintf(stderr, _T("Error: Invalid file format: %s\n"), in_filename);
        break;
    case TTC2TTF_RET_OUT_OF_MEMORY:
        _ftprintf(stderr, _T("Error: Out of memory\n"));
        break;
    case TTC2TTF_RET_BAD_FONT_INDEX:
        _ftprintf(stderr, _T("Error: The specified font index was out of range\n"));
        break;
    case TTC2TTF_RET_LOGIC_ERROR:
        _ftprintf(stderr, _T("Error: Logical error\n"));
        break;
    }

    return ret;
}

#ifndef TTC2TTF_NO_MAIN

extern "C"
int _tmain(int argc, _TCHAR **wargv)
{
    return (int)ttc2ttf_main(argc, wargv);
}

#ifdef UNICODE
int main(void)
{
    int argc;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int ret = wmain(argc, wargv);
    LocalFree(wargv);
    return ret;
}
#endif

#endif // ndef TTC2TTF_NO_MAIN
