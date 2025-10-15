/*
 * Copyright © 2025, Niklas Haas
 * Copyright © 2018, VideoLAN and dav1d authors
 * Copyright © 2018, Two Orioles, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CHECKASM_ATTRIBUTES_H
#define CHECKASM_ATTRIBUTES_H

#include <stddef.h>

#ifndef ATTR_FORMAT_PRINTF
    #ifdef __GNUC__
        #if defined(__MINGW32__) && !defined(__clang__)
            #define ATTR_FORMAT_PRINTF(fmt, attr)\
                __attribute__((__format__(__gnu_printf__, fmt, attr)))
        #else
            #define ATTR_FORMAT_PRINTF(fmt, attr)\
                __attribute__((__format__(__printf__, fmt, attr)))
        #endif
    #else
        #define ATTR_FORMAT_PRINTF(fmt, attr)
    #endif
#endif

#ifndef CHECKASM_API
    #ifdef _WIN32
      #ifdef CHECKASM_BUILDING_DLL
        #define CHECKASM_API __declspec(dllexport)
      #else
        #define CHECKASM_API
      #endif
    #else
      #if __GNUC__ >= 4
        #define CHECKASM_API __attribute__ ((visibility ("default")))
      #else
        #define CHECKASM_API
      #endif
    #endif
#endif

#endif /* CHECKASM_ATTRIBUTES_H */
