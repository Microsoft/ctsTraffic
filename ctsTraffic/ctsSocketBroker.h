/*

Copyright (c) Microsoft Corporation
All rights reserved.

Licensed under the Apache License, Version 2.0 (the ""License""); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.

See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

*/

#pragma once

// cpp headers
#include <vector>
#include <memory>
// os headers
#include <Windows.h>
// wil headers
#include <wil/resource.h>
// ctl headers
#include <ctThreadPoolTimer.hpp>
// project headers
#include "ctsConfig.h"
#include "ctsSocketState.h"

namespace ctsTraffic
{
    // forward declare ctsSocket
    // - can't include ctsSocket.h in this header to avoid circular declarations
    class ctsSocket;

    class ctsSocketBroker : public std::enable_shared_from_this<ctsSocketBroker>
    {
    public:
        // timer to wake up and clean up the socket pool
        // - delete any closed sockets
        // - create new sockets
        static unsigned long m_timerCallbackTimeoutMs;

        // only the c'tor can throw
        ctsSocketBroker();
        ~ctsSocketBroker() noexcept;

        void Start();

        // methods that the child ctsSocketState objects will invoke when they change state
        void InitiatingIo() noexcept;
        void Closing(bool wasActive) noexcept;

        // method to wait on when all connections are completed
        bool Wait(DWORD milliseconds) const noexcept;

        // not copyable
        ctsSocketBroker(const ctsSocketBroker&) = delete;
        ctsSocketBroker& operator=(const ctsSocketBroker&) = delete;
        ctsSocketBroker(ctsSocketBroker&&) = delete;
        ctsSocketBroker& operator=(ctsSocketBroker&&) = delete;

    private:
        // CS to guard access to the vector socket_pool
        wil::critical_section m_lock{ctsConfig::ctsConfigSettings::c_CriticalSectionSpinlock};
        // notification event when we're done
        wil::unique_event_nothrow m_doneEvent;
        // vector of currently active sockets
        // must be shared_ptr since ctsSocketState derives from enable_shared_from_this
        // - and thus there must be at least one refcount on that object to call shared_from_this()
        std::vector<std::shared_ptr<ctsSocketState>> m_socketPool;
        // timer to initiate the savenge routine TimerCallback()
        ctl::ctThreadpoolTimer m_wakeupTimer;
        // keep a burn-down count as connections are made to know when to be 'done'
        ULONGLONG m_totalConnectionsRemaining = 0ULL;
        // track what's pended and what's active
        unsigned long m_pendingLimit = 0UL;
        unsigned long m_pendingSockets = 0UL;
        unsigned long m_activeSockets = 0UL;

        //
        // Callback for the threadpool timer to scavenge closed sockets and recreate new ones
        // - this allows destroying ctsSockets outside of an inline path from ctsSocket
        //
        static void TimerCallback(_In_ ctsSocketBroker* pBroker) noexcept;
    };

} // namespace
