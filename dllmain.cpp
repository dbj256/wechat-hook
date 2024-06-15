// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

#include <thread>
#include "core.h"

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);



        std::thread serverThread([]() {
            //readMemory();

            dbj::httpServer::ServerThread();
            });
        serverThread.detach();

        std::thread rcvMsgThread2([]() {
            dbj::sync_msg_hook::SyncMsgHook::GetInstance().Init();

            dbj::sync_msg_hook::SyncMsgHook::GetInstance().Hook();


            });
        rcvMsgThread2.detach();
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

