// UnicodeUtil.cpp
//
// PASSWORD TECH
// Copyright (c) 2002-2023 by Christian Thoeing <c.thoeing@web.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//---------------------------------------------------------------------------
#include <vcl.h>
#include <vector>
#include <stdio.h>
#pragma hdrstop

#include "UnicodeUtil.h"
#include "Language.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

const int FORMAT_MAX_LEN = 1500;

static void formatError(void)
{
  throw EUnicodeError("Error while formatting string");
}

static void utf8EncodeError(void)
{
  throw EUnicodeError("Error while converting string to UTF-8");
}

static void utf8DecodeError(void)
{
  throw EUnicodeError("Error while decoding UTF-8 string");
}

//---------------------------------------------------------------------------
WString FormatW(const WString sFormat, ...)
{
  if (sFormat.IsEmpty())
    return WString();

  va_list argptr;
  int nSize = sFormat.Length() * 2 + 50;
  WString sResult;

  for (int i = 0; i < 2; i++) {
    auto buf = std::make_unique<wchar_t[]>(nSize);

    va_start(argptr, sFormat);
    const int nChars = vsnwprintf_s(buf.get(), nSize, sFormat.c_str(), argptr);
    va_end(argptr);

    if (nChars < 0)
      formatError();
    else if (nChars < nSize) {
      sResult = WString(buf.get());
      break;
    }
    else
      nSize = nChars + 1;
  }

  return sResult;
}
//---------------------------------------------------------------------------
SecureWString FormatW_Secure(const WString sFormat, ...)
{
  if (sFormat.IsEmpty())
    return SecureWString();

  va_list argptr;
  int nSize = sFormat.Length() * 2 + 50;
  SecureWString sResult;

  for (int i = 0; i < 2; i++) {
    SecureWString buf(nSize);

    va_start(argptr, sFormat);
    const int nChars = vsnwprintf_s(buf, nSize, sFormat.c_str(), argptr);
    va_end(argptr);

    if (nChars < 0)
      formatError();
    else if (nChars < nSize) {
      sResult.AssignStr(buf, nChars);
      break;
    }
    else
      nSize = nChars + 1;
  }

  return sResult;
}
//---------------------------------------------------------------------------
WString FormatW_ArgList(const WString sFormat, va_list arglist)
{
  if (sFormat.IsEmpty())
    return WString();

  va_list argptr;
  int nSize = sFormat.Length() * 2 + 50;
  WString sResult;

  for (int i = 0; i < 2; i++) {
    auto buf = std::make_unique<wchar_t[]>(nSize);

    const int nChars = vsnwprintf_s(buf.get(), nSize, sFormat.c_str(), arglist);

    if (nChars < 0)
      formatError();
    else if (nChars < nSize) {
      sResult = WString(buf.get());
      break;
    }
    else
      nSize = nChars + 1;
  }

  return sResult;
}
//---------------------------------------------------------------------------
int GetNumOfUnicodeChars(const wchar_t* pwszStr)
{
  int nChars = 0;

  for (; *pwszStr != '\0'; pwszStr++, nChars++) {
    //if (word32(*pwszStr - 0xD800) <= 0x3FF)
    if (*pwszStr >= 0xd800 && *pwszStr <= 0xdbff)
      pwszStr++;
  }

  return nChars;
}
//---------------------------------------------------------------------------
int GetNumOfUtf16Chars(const word32* pStr)
{
  int nChars = 0;

  for (; *pStr != '\0'; pStr++, nChars++) {
    if (*pStr > 0xFFFF)
      nChars++;
  }

  return nChars;
}
//---------------------------------------------------------------------------
int WCharToW32Char(const wchar_t* pwszSrc, word32* pDest)
{
  const word32* pDestStart = pDest;

  while (*pwszSrc != '\0') {
    if (pwszSrc[0] >= 0xd800 && pwszSrc[0] <= 0xdbff) {
      if (pwszSrc[1] >= 0xdc00 && pwszSrc[1] <= 0xdfff)
        *pDest++ = (pwszSrc[1] << 16) | pwszSrc[0];
      else
        throw EUnicodeError("Invalid UTF-16 character encoding");
      pwszSrc += 2;
    }
    else
      *pDest++ = *pwszSrc++;
  }

  *pDest = '\0';

  return pDest - pDestStart;
}
//---------------------------------------------------------------------------
int AsciiCharToW32Char(const char* pszSrc, word32* pDest)
{
  const char* pszSrcStart = pszSrc;

  //for (nLen = 0; pszSrc[nLen] != '\0'; nLen++)
  //  pDest[nLen] = pszSrc[nLen];
  while (*pszSrc != '\0')
    *pDest++ = *pszSrc++;

  *pDest = '\0';

  return pszSrc - pszSrcStart;
}
//---------------------------------------------------------------------------
w32string WStringToW32String(const WString& sSrc)
{
  if (sSrc.IsEmpty())
    return w32string();

  // WARNING: The following code presumes that the STL string implementation
  // uses a contiguous block of memory which is always null-terminated!
  w32string sDest(GetNumOfUnicodeChars(sSrc.c_str()), 0);

  WCharToW32Char(sSrc.c_str(), &sDest[0]);

  return sDest;
}
//---------------------------------------------------------------------------
w32string WCharToW32String(const wchar_t* pwszSrc)
{
  if (*pwszSrc == '\0')
    return w32string();

  w32string sDest(GetNumOfUnicodeChars(pwszSrc), 0);

  WCharToW32Char(pwszSrc, &sDest[0]);

  return sDest;
}
//---------------------------------------------------------------------------
void W32CharToWCharInternal(void* pBuf)
{
  word32* pBuf32 = reinterpret_cast<word32*>(pBuf);
  wchar_t* pBuf16 = reinterpret_cast<wchar_t*>(pBuf);

  for (; *pBuf32 != '\0'; pBuf32++) {
    *pBuf16++ = *pBuf32;
    if (*pBuf32 > 0xFFFF)
      *pBuf16++ = *pBuf32 >> 16;
  }

  *pBuf16 = '\0';
}
//---------------------------------------------------------------------------
WString W32StringToWString(const w32string& sSrc)
{
  if (sSrc.empty())
    return WString();

  WString sDest;
  const word32* pSrc = sSrc.c_str();
  sDest.SetLength(GetNumOfUtf16Chars(pSrc));

  /*for (int nI = 1; *pSrc != '\0'; pSrc++) {
    sDest[nI++] = *pSrc;
    if (*pSrc > 0xFFFF)
      sDest[nI++] = *ptr >> 16;
  }*/
  for (wchar_t* pwszDest = sDest.FirstChar(); *pSrc != '\0'; pSrc++) {
    *pwszDest++ = *pSrc;
    if (*pSrc > 0xffff)
      *pwszDest++ = *pSrc >> 16;
  }

  return sDest;
}
//---------------------------------------------------------------------------
w32string AsciiCharToW32String(const char* pszStr)
{
  int nStrLen = strlen(pszStr);
  if (nStrLen == 0)
    return w32string();

  w32string sDest;
  sDest.reserve(nStrLen);

  while (*pszStr != '\0')
    sDest.push_back(*pszStr++);

  return sDest;
}
//---------------------------------------------------------------------------
AnsiString WStringToUtf8(const WString& sSrc)
{
  if (sSrc.IsEmpty())
    return AnsiString();

  int nDestLen = WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), -1,
    nullptr, 0, nullptr, nullptr);
  if (nDestLen == 0)
    utf8EncodeError();

  AnsiString asDest;
  asDest.SetLength(nDestLen - 1);

  WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), -1, &asDest[1], nDestLen,
    nullptr, nullptr);

  return asDest;
}
//---------------------------------------------------------------------------
SecureAnsiString WStringToUtf8(const wchar_t* pwszSrc)
{
  if (pwszSrc == nullptr || *pwszSrc == '\0')
    return SecureAnsiString();

  int nLen = WideCharToMultiByte(CP_UTF8, 0, pwszSrc, -1, nullptr, 0,
    nullptr, nullptr);
  if (nLen == 0)
    utf8EncodeError();

  SecureAnsiString asDest(nLen);

  WideCharToMultiByte(CP_UTF8, 0, pwszSrc, -1, asDest, nLen, nullptr, nullptr);
  return asDest;
}
//---------------------------------------------------------------------------
/*SecureAnsiString WStringToUtf8(const SecureWString& sSrc)
{
  if (sSrc.IsStrEmpty())
    return SecureAnsiString();

  int nSrcLen = sSrc.StrLen() + 1;
  int nDestLen = WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), nSrcLen,
    nullptr, 0, nullptr, nullptr);
  if (nDestLen == 0)
    utf8EncodeError();

  SecureAnsiString asDest(nDestLen);

  WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), nSrcLen, asDest, nDestLen,
    nullptr, nullptr);
  return asDest;
}*/
//---------------------------------------------------------------------------
WString Utf8ToWString(const AnsiString& asSrc)
{
  if (asSrc.IsEmpty())
    return WString();

  int nDestLen = MultiByteToWideChar(CP_UTF8, 0, asSrc.c_str(), -1, nullptr, 0);
  if (nDestLen == 0)
    utf8DecodeError();

  WString sDest;
  sDest.SetLength(nDestLen - 1);

  MultiByteToWideChar(CP_UTF8, 0, asSrc.c_str(), -1, &sDest[1], nDestLen);

  return sDest;
}
//---------------------------------------------------------------------------
SecureWString Utf8ToWString(const char* pszSrc)
{
  if (pszSrc == nullptr || *pszSrc == '\0')
    return SecureWString();

  int nLen = MultiByteToWideChar(CP_UTF8, 0, pszSrc, -1, nullptr, 0);
  if (nLen == 0)
    utf8DecodeError();

  SecureWString sDest(nLen);

  MultiByteToWideChar(CP_UTF8, 0, pszSrc, -1, sDest, nLen);
  return sDest;
}
//---------------------------------------------------------------------------
size_t w32strlen(const word32* pStr)
{
  const word32* pStart = pStr;
  while (*pStr != '\0') pStr++;
  return static_cast<size_t>(pStr - pStart);
}
//---------------------------------------------------------------------------
