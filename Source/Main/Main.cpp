#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Http/Http.hpp"
#include "../Network/Network.hpp"
#include "../Main/Config.hpp"

#include <iostream>
#include <thread>

/* @important: closing the console window, or Ctrl+C, does not run atexit
   handlers -- and those are the two ways this program is normally ended.

   Two things must not survive us:

     the route      its sockets and threads. On the next start the shim
                    would fail to bind, for a reason that looks like
                    nothing at all.

     the hosts file while the proxy runs it points www.growtopia1.com and
                    www.growtopia2.com at 127.0.0.1. Left that way, the
                    real game cannot reach Growtopia at all, and nothing
                    says why -- the operator is left editing a system file
                    by hand to get their game back. */
static BOOL WINAPI OnConsoleEvent(DWORD) {
    System::StopRoute();
    System::editHosts("");
    return FALSE;   /* let the default handler finish terminating us */
}

auto main() -> int {
    if (!System::IsTrueAdmin() && !System::RelaunchAsAdmin()) {
        LOG_ERROR("Couldn't relaunched the program as admin!");
        return EXIT_FAILURE;
    }
    System::EnableDebugPrivilege();
    SetConsoleCtrlHandler(OnConsoleEvent, TRUE);
    
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetProcessPriorityBoost(GetCurrentProcess(), TRUE);

    if (enet_initialize() != 0) {
        LOG_ERROR("ENet initialization failed!");
        return -1;
    }
    atexit(enet_deinitialize);

    /* @important: the route is decided here, before Growtopia.exe is
       killed and before the hosts file is written. If any part of it
       cannot be brought up we stop, and stopping at this point has cost
       the operator nothing. We never quietly continue direct: an operator
       acting on the belief that they are routed when they are not is the
       worst outcome available here. */
    Route.MODE = System::ChooseRoute();

    if (Route.MODE == gRoute::SOCKS5) {
        if (!System::LoadRouteConfig("socks5.cfg") ||
            !System::RoutePreflight() ||
            !System::StartConnectShim()) {
            System::StopRoute();
            std::cout << "\n"
                         "   Stopping. Nothing on this PC was changed:\n"
                         "   Growtopia.exe was not touched and the hosts file\n"
                         "   was not modified.\n\n";
            return EXIT_FAILURE;
        }

        atexit([] { System::StopRoute(); });

        std::cout << "\n"
                     "   Route: SOCKS5\n"
                     "\n"
                     "   What is routed:   the server_data.php fetch, and the\n"
                     "                     ENet game session.\n"
                     "   What is NOT:      the game client opens the login page\n"
                     "                     itself. That request never reaches this\n"
                     "                     process, so it still goes out directly.\n\n";
    }
    else {
        std::cout << "\n   Route: DIRECT\n\n";
    }

    if (System::findProcess(L"Growtopia.exe")) {
        System::endProcess(L"Growtopia.exe");
        Sleep(1500);
    }

    System::editHosts("127.0.0.1");

    FastLog::Logger::instance().start();
    FastLog::Logger::set_thread_name("MAIN");

    LOG_WARN("Setting up all threads");

    std::thread http_thread(&HttpManager::Injector, &Http);
    std::thread network_thread(&NetworkManager::Injector, &Network);

    HANDLE hHttp = (HANDLE)http_thread.native_handle();
    HANDLE hNetwork = (HANDLE)network_thread.native_handle();

    SetThreadPriority(hHttp, THREAD_PRIORITY_BELOW_NORMAL);
    SetThreadPriority(hNetwork, THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadAffinityMask(hNetwork, 1 << 2);

    http_thread.join();
    network_thread.join();
    
    LOG_WARN("All threads finished");

    /* Explicit rather than atexit: both of these want the logger, and
       atexit handlers run after it has been stopped. */
    System::StopRoute();
    System::editHosts("");

    FastLog::Logger::instance().stop();

    return EXIT_SUCCESS;
}