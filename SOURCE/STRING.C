#include <TOOLKIT/STRING.H>
#include <TOOLKIT/ALLOCATOR.H>
#include <string.h>
#include <ctype.h>

VOID SVFree(PALLOCATOR allocator, SV string)
{
   if (not allocator) allocator = DefaultAllocator();
   Free(allocator, (PVOID)string.String);
}
BOOL SVEqual(SV s1, SV s2)
{
   if (svinvalid(s1) or svinvalid(s2)) return false;
   if (s1.String == s2.String and s1.Size == s2.Size) return true;

   if (s1.Size != s2.Size) return false;
   if (s1.Size == 0) return true;
   return (memcmp(s1.String, s2.String, s1.Size) == 0);
}
BOOL SVEqualNoCase(SV s1, SV s2)
{
   if (svinvalid(s1) or svinvalid(s2)) return false;
   if (s1.String == s2.String and s1.Size == s2.Size) return true;

   if (s1.Size != s2.Size) return false;
   if (s1.Size == 0)       return true;

   const U8 *p1 = (const U8 *)s1.String;
   const U8 *p2 = (const U8 *)s2.String;
   for (U64 i = 0; i < s1.Size; i++)
   {
	  if (p1[i] != p2[i])
	  {
		 U8 c1 = p1[i] | 0x20;
		 U8 c2 = p2[i] | 0x20;
		 if (c1 != c2 or c1 < 'a' or c1 > 'z')
			return false;
	  }
   }
   return true;
}
I32 SVCompare(SV s1, SV s2)
{
   if (svinvalid(s1) or svinvalid(s2))
	  return (s1.String == s2.String) ? 0 : (s1.String ? 1 : -1);
   // Compare up to the smaller length
   U64 min_size = (s1.Size < s2.Size) ? s1.Size : s2.Size;
   I32 res = memcmp(s1.String, s2.String, min_size);
   if (res != 0)
	  return res;
   // Final size check
   if (s1.Size < s2.Size)      return -1;
   else if (s1.Size > s2.Size) return 1;
   else                          return 0;
}
I64 SVChr(SV s, CHAR c)
{
   if (svinvalid(s)) return -1;
   PCHAR p = (PCHAR)memchr(s.String, c, s.Size);
   if (not p) return -1;
   return (I64)(p - s.String);
}
SV SVStrdup(PALLOCATOR allocator, SV string)
{
   SV r = string;
   if (not allocator) allocator = DefaultAllocator();
   r.String = (PCHAR)Alloc(allocator, sizeof(CHAR) * string.Size, alignof(CHAR));
   if (!r.String)
	  return SV_INVALID;
   memcpy(r.String, string.String, string.Size);
   return r;
}
SV SVStrdupTerminated(PALLOCATOR allocator, SV string)
{
   SV r = string;
   if (not allocator) allocator = DefaultAllocator();
   r.String = (PCHAR)Alloc(allocator, sizeof(CHAR) * (string.Size + 1), alignof(CHAR));
   if (not r.String) return SV_INVALID;
   memcpy(r.String, string.String, string.Size);
   r.String[string.Size] = '\0';
   return r;
}
SV SVJoinList(PALLOCATOR allocator, CHAR separator, U32 strings_count, PSV strings)
{
   U64 size = strings_count; // For separators and null terminator
   for (U32 i = 0; i < strings_count; i++) // For string sizes
	  size += strings[i].Size;

   if (not allocator) allocator = DefaultAllocator();
   PCHAR buffer = (PCHAR)Alloc(allocator, size, alignof(CHAR));
   U64 offset = 0;
   U32 i;
   for (i = 0; i < strings_count - 1; i++)
   {
	  memcpy(buffer + offset, strings[i].String, strings[i].Size);
	  offset += strings[i].Size;
	  buffer[offset] = separator;
	  offset += 1;
   }
   memcpy(buffer + offset, strings[i].String, strings[i].Size);
   offset += strings[i].Size;
   buffer[offset] = '\0';
   return (SV) { .String = buffer, .Size = size };
}
SV SVJoin(PALLOCATOR allocator, U32 strings_count, PSV strings)
{
   U64 size = 1;
   for (U32 i = 0; i < strings_count; i++) // For string sizes
	  size += strings[i].Size;

   PCHAR buffer = (PCHAR)Alloc(allocator, size, alignof(CHAR));
   U64 offset = 0;
   for (U32 i = 0; i < strings_count; i++)
   {
	  memcpy(buffer + offset, strings[i].String, strings[i].Size);
	  offset += strings[i].Size;
   }
   buffer[offset] = '\0';
   return (SV) { .String = buffer, .Size = size };
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
SV SVFind(SV haystack, SV needle)
{
   if (svinvalid(haystack) or svinvalid(needle) or haystack.Size < needle.Size) return SV_INVALID;

   const CHAR needle_first = needle.String[0];
   U64 needle_len = needle.Size;
   U64 needle_len_1 = needle_len - 1;
   // Compute initial window checksum difference and identity check
   I64 sums_diff = 0;
   BOOL identity = true;
   U64  processed_bytes = 0;
   // Build rolling window up to needle length from the absolute start
   while (processed_bytes < needle.Size)
   {
	  sums_diff += haystack.String[processed_bytes];
	  sums_diff -= needle.String[processed_bytes];
	  identity  &= (haystack.String[processed_bytes] == needle.String[processed_bytes]);
	  processed_bytes++;
   }

   if (identity)
	  return sv(haystack.String, needle.Size);

   // Setup rolling window variables
   CSTR it = haystack.String;
   CSTR end = haystack.String + (haystack.Size - needle_len);
   U64 i_haystack = needle_len;
   while (it < end) // Rolling hash execution loop
   {
	  // Slide the window forward: subtract the character exiting, add the one entering
	  sums_diff -= *it++;
	  sums_diff += haystack.String[i_haystack++];

	  // Verify window checksum and check characters
	  if (sums_diff == 0 and needle_first == *it and memcmp(it, needle.String, needle_len_1) == 0)
		 return sv(haystack.String + (U64)(it - haystack.String), needle.Size);
   }
   return SV_INVALID;
}
SV SVFindNoCase(SV haystack, SV needle)
{
   if (svinvalid(haystack) or svinvalid(needle) or haystack.Size < needle.Size)
	  return SV_INVALID;

   const int needle_first = tolower(needle.String[0]);
   U64 needle_len = needle.Size;
   U64 needle_len_1 = needle_len - 1;

   // Compute initial window checksum difference and identity check
   I64 sums_diff = 0;
   BOOL identical = true;
   U64 processed_bytes = 0;

   // Build rolling window up to needle length from the absolute start
   while (processed_bytes < needle_len)
   {
	  int h = tolower(haystack.String[processed_bytes]);
	  int n = tolower(needle.String[processed_bytes]);

	  sums_diff += h;
	  sums_diff -= n;

	  identical &= (h == n);
	  processed_bytes++;
   }

   if (identical)
	  return sv(haystack.String, needle_len);

   // Setup rolling window variables
   CSTR it = haystack.String;
   CSTR end = haystack.String + (haystack.Size - needle_len);
   U64 i_haystack = needle_len;

   while (it < end) // Rolling hash execution loop
   {
	  // Slide the window forward
	  sums_diff -= tolower(*it);
	  it++;
	  sums_diff += tolower(haystack.String[i_haystack++]);

	  // Fast first-character check
	  if (sums_diff == 0 and needle_first == tolower(*it))
	  {
		 BOOL match = true;
		 for (U64 i = 1; i < needle_len; ++i)
		 {
			if (tolower(it[i]) != tolower(needle.String[i]))
			{
			   match = false;
			   break;
			}
		 }

		 if (match)
			return sv(haystack.String + (U64)(it - haystack.String), needle_len);
	  }
   }

   return SV_INVALID;
}