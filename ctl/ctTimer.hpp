/*

Copyright (c) Microsoft Corporation
All rights reserved.

Licensed under the Apache License, Version 2.0 (the ""License""); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.

See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

*/

#pragma once

// os headers
#include <Windows.h>

// ReSharper disable once CppInconsistentNaming
namespace ctl
{
    ///
    /// ctTimer namespace contains useful functions when working with QPC/QPF
    ///
    namespace ctTimer
    {
        /// 
        /// Conversion functions between hundred-nanoseconds <-> milliseconds
        ///
        /// nano-second == 10 ^ -9
        /// - therefore 100 nano-seconds == 10 ^ -7
        /// millisecond == 10 ^ -3
        ///

        constexpr long long ConvertMillisToHundredNs(long long milliseconds) noexcept
        {
            return static_cast<long long>(milliseconds * 10000LL);
        }

        constexpr long long ConvertHundredNsToMillis(long long _hundred_nanoseconds) noexcept
        {
            return static_cast<long long>(_hundred_nanoseconds / 10000LL);
        }

        inline FILETIME ConvertHundredNsToAbsoluteFiletime(long long hundred_nanoseconds) noexcept
        {
            ULARGE_INTEGER ulongInteger;
            ulongInteger.QuadPart = hundred_nanoseconds;

            FILETIME returnFiletime;
            returnFiletime.dwHighDateTime = ulongInteger.HighPart;
            returnFiletime.dwLowDateTime = ulongInteger.LowPart;
            return returnFiletime;
        }

        // Create a negative FILETIME, which for some timer APIs indicate a 'relative' time
        // - e.g. SetThreadpoolTimer, where a negative value indicates the amount of time to wait relative to the current time 
        inline FILETIME ConvertHundredNsToRelativeFiletime(long long hundred_nanoseconds) noexcept
        {
            ULARGE_INTEGER ulongInteger;
            ulongInteger.QuadPart = static_cast<ULONGLONG>(-hundred_nanoseconds);

            FILETIME returnFiletime;
            returnFiletime.dwHighDateTime = ulongInteger.HighPart;
            returnFiletime.dwLowDateTime = ulongInteger.LowPart;
            return returnFiletime;
        }

        inline long long ConvertFiletimeToHundredNs(const FILETIME& filetime) noexcept
        {
            ULARGE_INTEGER ulongInteger;
            ulongInteger.HighPart = filetime.dwHighDateTime;
            ulongInteger.LowPart = filetime.dwLowDateTime;

            return ulongInteger.QuadPart;
        }

        inline FILETIME ConvertMillisToAbsoluteFiletime(long long milliseconds) noexcept
        {
            return ConvertHundredNsToAbsoluteFiletime(ConvertMillisToHundredNs(milliseconds));
        }

        // Create a negative FILETIME, which for some timer APIs indicate a 'relative' time
        // - e.g. SetThreadpoolTimer, where a negative value indicates the amount of time to wait relative to the current time 
        inline FILETIME ConvertMillisToRelativeFiletime(long long milliseconds) noexcept
        {
            return ConvertHundredNsToRelativeFiletime(ConvertMillisToHundredNs(milliseconds));
        }

        inline long long ConvertFiletimeToMillis(const FILETIME& filetime) noexcept
        {
            ULARGE_INTEGER ulongInteger;
            ulongInteger.HighPart = filetime.dwHighDateTime;
            ulongInteger.LowPart = filetime.dwLowDateTime;

            return ConvertHundredNsToMillis(ulongInteger.QuadPart);
        }

        namespace Details
        {
            ///
            /// InitOnce the QPF value as it won't change after the OS has booted
            /// - hiding within an unnamed namesapce
            ///
            // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
            static INIT_ONCE g_qpfInitOnce = INIT_ONCE_STATIC_INIT;
            static LARGE_INTEGER g_qpf;

            static BOOL CALLBACK QpfInitOnceCallback(_In_ PINIT_ONCE, _In_ PVOID, _In_ PVOID*) noexcept
            {
                QueryPerformanceFrequency(&g_qpf);
                return TRUE;
            }
        }

        inline long long SnapQpf() noexcept
        {
            InitOnceExecuteOnce(&Details::g_qpfInitOnce, Details::QpfInitOnceCallback, nullptr, nullptr);
            return Details::g_qpf.QuadPart;
        }

#ifdef CTSTRAFFIC_UNIT_TESTS
        inline long long SnapQpcInMillis() noexcept
        {
            return 0;
        }
#else
        inline long long SnapQpcInMillis() noexcept
        {
            InitOnceExecuteOnce(&Details::g_qpfInitOnce, Details::QpfInitOnceCallback, nullptr, nullptr);
            LARGE_INTEGER qpc;
            QueryPerformanceCounter(&qpc);
            // multiplying by 1000 as (qpc / qpf) == seconds
            return static_cast<long long>(qpc.QuadPart * 1000LL / Details::g_qpf.QuadPart);
        }
#endif

        inline FILETIME SnapQpcAsFiletime() noexcept
        {
            return ConvertHundredNsToAbsoluteFiletime(SnapQpcInMillis());
        }

        inline long long SnapSystemTimeInMillis() noexcept
        {
            FILETIME filetime;
            GetSystemTimeAsFileTime(&filetime);
            return ConvertFiletimeToMillis(filetime);
        }
    } // namespace ctTimer
} // namespace ctl
