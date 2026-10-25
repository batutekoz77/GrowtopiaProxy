#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Http/Http.hpp"
#include "../Network/Network.hpp"

#include <thread>

auto main() -> int {
    if (!System::IsTrueAdmin() && !System::RelaunchAsAdmin()) {
        LOG_ERROR("Couldn't relaunched the program as admin!");
        return EXIT_FAILURE;
    }
    System::EnableDebugPrivilege();
    
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetProcessPriorityBoost(GetCurrentProcess(), TRUE);

    if (enet_initialize() != 0) {
        LOG_ERROR("ENet initialization failed!");
        return -1;
    }
    atexit(enet_deinitialize);

    if (System::findProcess(L"Growtopia.exe")) {
        System::endProcess(L"Growtopia.exe");
        Sleep(1500);
    }

    atexit([] {
        System::editHosts("");
    });

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

    FastLog::Logger::instance().stop();

    return EXIT_SUCCESS;
}