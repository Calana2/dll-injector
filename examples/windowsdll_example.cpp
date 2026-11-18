// g++ -m32 -static -shared windowsdll_example.cpp -o sample_dll_pe32.dll
// g++ -static -shared windowsdll_example.cpp -o sample_dll_pe32+.dll

#ifndef UNICODE
  #define UNICODE
#endif

#include <windows.h>
#include <stdlib.h>

void injected_thread() {
 while (true) {
  Sleep(1000); // 1second
 }
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
  	// Code to execute when the process is loaded
  	MessageBox(0, L"DLL", L"Library loaded.",0);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)injected_thread, NULL, 0, NULL);
  }
  return true;
}