// g++ -m32 -shared windowsdll_safe_eject example.cpp -o sample_dll_safe_eject_pe32.dll
// g++ -shared windowsdll_safe_eject example.cpp -o sample_dll_safe_eject_pe32+.dll

#ifndef UNICODE
  #define UNICODE
#endif

#include <windows.h>
#include <stdlib.h>

static volatile long g_stop = 0; // spinlock
static HANDLE g_thread = NULL;

void injected_thread() {
 while (InterlockedCompareExchange(&g_stop, 0, 0)) {
  Sleep(1000); // 1 second
 }
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
  
  if (fdwReason == DLL_PROCESS_ATTACH) {
  	// Code to execute when the process is loaded
  	MessageBox(0, L"DLL", L"Library loaded.",0);
    InterlockedExchange(&g_stop, 0);
    g_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)injected_thread, NULL, 0, NULL);
  }

  if (fdwReason == DLL_PROCESS_DETACH) { 
    // Called when its being ejected / unloaded
    InterlockedExchange(&g_stop, 1);
    // lpvReserved == NULL when FreeLibrary() is called
    if (lpvReserved  == NULL && g_thread) {
      WaitForSingleObject(g_thread, 3000); // 3 seconds, just in case
      CloseHandle(g_thread);
      g_thread = NULL;
    }
  }

  return true;
}
