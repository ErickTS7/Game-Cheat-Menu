// dllmain.cpp : Define o ponto de entrada para o aplicativo DLL.
#include <windows.h>
#include <iostream>
#include <cstdio>
#include "Hooks/Minhook/include/MinHook.h"

#include "Hooks/Hooks_Core/Hooks_Present.h"


void start() {
    //MH_Initialize();
    Beep(500, 800);
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "DLL Injected" << std::endl;
    KieroHooks::Init();
}

void DllThread() {
    start();
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)DllThread, hModule, 0, 0));


        //case DLL_THREAD_ATTACH:
        //case DLL_THREAD_DETACH:
        //case DLL_PROCESS_DETACH:

        break;
    }
    return TRUE;
}

