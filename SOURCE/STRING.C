#include <TOOLKIT/STRING.H>
#include <TOOLKIT/ALLOCATOR.H>
#include <string.h>
#include <ctype.h>

VOID FStrFree(PALLOCATOR allocator, FSTR s)
{
    Free(allocator, (PVOID)s.str);
}
BOOL FStrEqual(FSTR s1, FSTR s2)
{
    if (fstrinvalid(s1) or fstrinvalid(s2)) return false;
    if (s1.str == s2.str and s1.size == s2.size) return true;

    if (s1.size != s2.size) return false;
    if (s1.size == 0) return true;
    return (memcmp(s1.str, s2.str, s1.size) == 0);
}
I32 FStrCompare(FSTR s1, FSTR s2)
{
    if (fstrinvalid(s1) or fstrinvalid(s2))
        return (s1.str == s2.str) ? 0 : (s1.str ? 1 : -1);
    // Compare up to the smaller length
    U64 min_size = (s1.size < s2.size) ? s1.size : s2.size;
    I32 res = memcmp(s1.str, s2.str, min_size);
    if (res != 0)
        return res;
    // Final size check
    if (s1.size < s2.size)      return -1;
    else if (s1.size > s2.size) return 1;
    else                          return 0;
}
I64 FStrChr(FSTR s, CHAR c)
{
    if (fstrinvalid(s)) return -1;
    PCHAR p = (PCHAR)memchr(s.str, c, s.size);
    if (not p) return -1;
    return (I64)(p - s.str);
}
FSTR FstrStrdup(PALLOCATOR allocator, FSTR string)
{
    FSTR r = string;
    r.str = Alloc(allocator, sizeof(CHAR) * string.size, alignof(CHAR));
    if (!r.str)
        return FSTR_INVALID;
    memcpy(r.str, string.str, string.size);
    return r;
}
FSTR FStrJoinList(PALLOCATOR allocator, CHAR separator, U32 strings_count, PFSTR strings)
{
    U64 size = strings_count; // For separators and null terminator
    for (U32 i = 0; i < strings_count; i++) // For string sizes
        size += strings[i].size;

    PCHAR buffer = (PCHAR)Alloc(allocator, size, alignof(CHAR));
    U64 offset = 0;
    U32 i;
    for (i = 0; i < strings_count - 1; i++)
    {
        memcpy(buffer + offset, strings[i].str, strings[i].size);
        offset += strings[i].size;
        buffer[offset] = separator;
        offset += 1;
    }
    memcpy(buffer + offset, strings[i].str, strings[i].size);
    offset += strings[i].size;
    buffer[offset] = '\0';
    return (FSTR) { .str = buffer, .size = size };
}
FSTR FStrJoin(PALLOCATOR allocator, U32 strings_count, PFSTR strings)
{
    U64 size = 1;
    for (U32 i = 0; i < strings_count; i++) // For string sizes
        size += strings[i].size;

    PCHAR buffer = (PCHAR)Alloc(allocator, size, alignof(CHAR));
    U64 offset = 0;
    for (U32 i = 0; i < strings_count; i++)
    {
        memcpy(buffer + offset, strings[i].str, strings[i].size);
        offset += strings[i].size;
    }
    buffer[offset] = '\0';
    return (FSTR) { .str = buffer, .size = size };
}

//
// This algorithm is licensed under the open-source BSD3 license
//
// Copyright (c) 2014, Raphael Javaux
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
FSTR FStrFind(FSTR haystack, FSTR needle)
{
    if (fstrinvalid(haystack) || fstrinvalid(needle) || haystack.size < needle.size) return FSTR_INVALID;

    const CHAR needle_first = needle.str[0];
    U64 needle_len = needle.size;
    U64 needle_len_1 = needle_len - 1;

    // Compute initial window checksum difference and identity check
    U32 sums_diff = 0;
    BOOL identical = true;
    U64  processed_bytes = 0;

    // Build rolling window up to needle length from the absolute start
    while (processed_bytes < needle.size)
    {
        sums_diff += (U32)(unsigned char)haystack.str[processed_bytes];
        sums_diff -= (U32)(unsigned char)needle.str[processed_bytes];
        identical &= (haystack.str[processed_bytes] == needle.str[processed_bytes]);
        processed_bytes++;
    }

    if (identical)
        return fstr(haystack.str, needle.size);

    // Setup rolling window variables
    CSTR it = haystack.str;
    CSTR end = haystack.str + (haystack.size - needle_len);
    U64 i_haystack = needle_len;

    while (it < end) // Rolling hash execution loop
    {
        // Slide the window forward: subtract the character exiting, add the one entering
        sums_diff -= (U32)(unsigned char)*it++;
        sums_diff += (U32)(unsigned char)haystack.str[i_haystack++];

        // Verify window checksum and check characters
        if (sums_diff == 0 and needle_first == *it and memcmp(it, needle.str, needle_len_1) == 0)
            return fstr(haystack.str + (U64)(it - haystack.str), needle.size);
    }
    return FSTR_INVALID;
}

FSTR FStrFindNoCase(FSTR haystack, FSTR needle)
{
    if (fstrinvalid(haystack) || fstrinvalid(needle) || haystack.size < needle.size)
        return FSTR_INVALID;

    const CHAR needle_first = (CHAR)tolower((unsigned char)needle.str[0]);
    U64 needle_len = needle.size;
    U64 needle_len_1 = needle_len - 1;

    // Compute initial window checksum difference and identity check
    U32 sums_diff = 0;
    BOOL identical = true;
    U64 processed_bytes = 0;

    // Build rolling window up to needle length from the absolute start
    while (processed_bytes < needle_len)
    {
        unsigned char h = (unsigned char)tolower((unsigned char)haystack.str[processed_bytes]);
        unsigned char n = (unsigned char)tolower((unsigned char)needle.str[processed_bytes]);

        sums_diff += (U32)h;
        sums_diff -= (U32)n;

        identical &= (h == n);
        processed_bytes++;
    }

    if (identical)
        return fstr(haystack.str, needle_len);

    // Setup rolling window variables
    CSTR it = haystack.str;
    CSTR end = haystack.str + (haystack.size - needle_len);
    U64 i_haystack = needle_len;

    while (it < end) // Rolling hash execution loop
    {
        // Slide the window forward
        sums_diff -= (U32)(unsigned char)tolower((unsigned char)*it);
        it++;
        sums_diff += (U32)(unsigned char)tolower((unsigned char)haystack.str[i_haystack++]);

        // Fast first-character check
        if (sums_diff == 0 and needle_first == (CHAR)tolower((unsigned char)*it))
        {
            BOOL match = true;
            for (U64 i = 1; i < needle_len; ++i)
            {
                unsigned char h = (unsigned char)tolower((unsigned char)it[i]);
                unsigned char n = (unsigned char)tolower((unsigned char)needle.str[i]);

                if (h != n)
                {
                    match = false;
                    break;
                }
            }

            if (match)
                return fstr(haystack.str + (U64)(it - haystack.str), needle_len);
        }
    }

    return FSTR_INVALID;
}