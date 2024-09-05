// ttc2ttf.cpp --- Split TTF file from TTC font collection
// License: MIT
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>
#include "ttc2ttf.h"

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

extern "C"
void ttc2ttf_usage(void)
{
    _tprintf(_T("Usage: ttc2ttf input.ttc [font_index output.ttf]\n"));
}

extern "C"
void ttc2ttf_version(void)
{
    _tprintf(_T("ttc2ttf Version 0.5 by katahiromz\n"));
    _tprintf(_T("License: MIT\n"));
}

static inline uint32_t u32_ceil4(uint32_t n)
{
    return (n + 3) & ~3;
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

static bool read_all(const tstring& filename, std::vector<char>& content)
{
    FILE *fin = _tfopen(filename.c_str(), _T("rb"));
    if (!fin)
        return false;

    static char s_buf[1024 * 16];
    for (;;)
    {
        size_t length = fread(s_buf, 1, _countof(s_buf), fin);
        if (!length)
            break;

        content.insert(content.end(), &s_buf[0], &s_buf[length]);
    }

    fclose(fin);
    return true;
}

static bool write_all(const tstring& filename, const void *ptr, size_t size)
{
    FILE *fout = _tfopen(filename.c_str(), _T("wb"));
    if (!fout)
        return false;

    size_t written = fwrite(ptr, size, 1, fout);
    fclose(fout);
    return !!written;
}

static bool
ttc2ttf_extract(const _TCHAR *out_filename, const std::vector<char>& content, uint32_t ttf_offset, int index)
{
    _tprintf(_T("Extract TTF #%d to %s\n"), index, out_filename);
    _tprintf(_T("\tHeader offset %d\n"), ttf_offset);

    uint16_t table_count = u16_swab(*reinterpret_cast<const uint16_t*>(&content.at(ttf_offset + 4)));
    uint32_t header_length = 12 + table_count * 16;
    _tprintf(_T("\tHeader size %d bytes\n"), header_length);

    uint32_t table_length = 0;
    for (uint32_t j = 0; j < table_count; ++j)
    {
        uint32_t length = u32_swab(*reinterpret_cast<const uint32_t*>(&content.at(ttf_offset + 12 + 12 + j * 16)));
        table_length += u32_ceil4(length);
    }

    uint32_t total_length = header_length + table_length;
    std::vector<char> new_buf(total_length);
    std::memcpy(new_buf.data(), &content.at(ttf_offset), header_length);
    uint32_t current_offset = header_length;

    for (uint32_t j = 0; j < table_count; ++j)
    {
        uint32_t offset = u32_swab(*reinterpret_cast<const uint32_t*>(&content.at(ttf_offset + 12 + 8 + j * 16)));
        uint32_t length = u32_swab(*reinterpret_cast<const uint32_t*>(&content.at(ttf_offset + 12 + 12 + j * 16)));
        *reinterpret_cast<uint32_t*>(&new_buf.at(12 + 8 + j * 16)) = u32_swab(current_offset);
        std::memcpy(&new_buf.at(current_offset), &content.at(offset), length);
        current_offset += u32_ceil4(length);
    }

    return write_all(out_filename, new_buf.data(), new_buf.size());
}

extern "C"
TTC2TTF_RET ttc2ttf_except(const _TCHAR *in_filename, int entry_index, const _TCHAR *out_filename)
{
    if (entry_index < 0 && entry_index != -1)
        return TTC2TTF_RET_INVALID_ARGUMENTS;
    if (entry_index == -1 && out_filename)
        return TTC2TTF_RET_INVALID_ARGUMENTS;

    // read all
    std::vector<char> content;
    if (!read_all(in_filename, content))
        return TTC2TTF_RET_READ_ERROR;

    // Remove filename extension
    tstring filename = in_filename;
    if (filename.find(".ttc") == filename.size() - 4)
        filename.erase(filename.size() - 4, 4);

    // not a TTC file?
    if (content.size() >= 4 && std::memcmp(content.data(), "ttcf", 4) != 0)
    {
        filename += _T(".ttf");
        if (!write_all(filename.c_str(), content.data(), content.size()))
            return TTC2TTF_RET_WRITE_ERROR;
        return TTC2TTF_RET_NOT_TTC;
    }

    // get the count of fonts
    uint32_t ttf_count = u32_swab(*reinterpret_cast<uint32_t*>(&content.at(8)));
    _tprintf(_T("ttf_count: %d\n"), ttf_count);

    // get offsets
    std::vector<uint32_t> ttf_offset_array(ttf_count);
    std::memcpy(ttf_offset_array.data(), &content.at(12), ttf_count * sizeof(uint32_t));
    for (auto& offset : ttf_offset_array)
    {
        offset = u32_swab(offset);
    }

    if (entry_index == -1)
    {
        // extract each ttf
        for (uint32_t font_index = 0; font_index < ttf_count; ++font_index)
        {
            tstring output_filename = filename + to_tstring(font_index) + _T(".ttf");
            if (!ttc2ttf_extract(output_filename.c_str(), content, ttf_offset_array.at(font_index), font_index))
                return TTC2TTF_RET_WRITE_ERROR;
        }
    }
    else
    {
        if (entry_index >= ttf_count)
            return TTC2TTF_RET_BAD_FONT_INDEX;

        // extract specific ttf
        uint32_t font_index = entry_index;
        if (!ttc2ttf_extract(out_filename, content, ttf_offset_array.at(font_index), font_index))
            return TTC2TTF_RET_WRITE_ERROR;
    }

    // done
    return TTC2TTF_RET_NO_ERROR;
}

extern "C"
TTC2TTF_RET ttc2ttf_noexcept(const _TCHAR *in_filename, int font_index, const _TCHAR *out_filename)
{
    try
    {
        return ttc2ttf_except(in_filename, font_index, out_filename);
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

extern "C"
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

    ret = ttc2ttf_noexcept(in_filename, font_index, out_filename);
    switch (ret)
    {
    case TTC2TTF_RET_NO_ERROR:
        break;
    case TTC2TTF_RET_NOT_TTC:
        _ftprintf(stderr, "Warning: Not a TTC file\n");
        ret = TTC2TTF_RET_NO_ERROR;
        break;
    case TTC2TTF_RET_INVALID_ARGUMENTS:
        _ftprintf(stderr, _T("Invalid arguments\n"));
        ttc2ttf_usage();
        break;
    case TTC2TTF_RET_READ_ERROR:
        _ftprintf(stderr, _T("Error in reading file: %s\n"), in_filename);
        break;
    case TTC2TTF_RET_WRITE_ERROR:
        _ftprintf(stderr, _T("Error in writing file: %s\n"), out_filename);
        break;
    case TTC2TTF_RET_INVALID_FORMAT:
        _ftprintf(stderr, _T("Invalid file format: %s\n"), in_filename);
        break;
    case TTC2TTF_RET_OUT_OF_MEMORY:
        _ftprintf(stderr, _T("Out of memory\n"));
        break;
    case TTC2TTF_RET_BAD_FONT_INDEX:
        _ftprintf(stderr, _T("The specified font index was out of range\n"));
        break;
    case TTC2TTF_RET_LOGIC_ERROR:
        _ftprintf(stderr, _T("Logical error\n"));
        break;
    }

    return ret;
}

#ifndef TTC2TTF_NO_EXE

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

#endif // ndef TTC2TTF_NO_EXE
