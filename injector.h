#ifndef INJECTOR_H
#define INJECTOR_H

#include <windows.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <iostream>
#include <wchar.h>
#include "winlamb/sysdlg.h"


namespace injector {
	HWND w = NULL;
	HWND h_log = NULL;
	bool process_found = false;

	static void show_err(std::wstring msg) { 
		::MessageBox(w, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
		//::ExitProcess(1);
	} 	
	
    static void log(const std::wstring& msg) {
    	// set cursor
    	int len = ::GetWindowTextLengthW(h_log);
    	::SendMessageW(h_log, EM_SETSEL, (WPARAM)len, (LPARAM)len);
		// append
    	std::wstring line = msg + L"\r\n";
    	::SendMessageW(h_log, EM_REPLACESEL, FALSE, (LPARAM)line.c_str());
        // scroll
    	::SendMessageW(h_log, EM_SCROLLCARET, 0, 0);
	}	

	static void _inject_dll(HANDLE process, std::wstring dll_path) {
		void *lpBaseAddress = NULL;
		size_t dll_strsize = (dll_path.length() + 1) * sizeof(wchar_t);
	
		HANDLE thread = 0;
		HMODULE kernel32_base = 0;
		unsigned long exit_code = 0;
	
		log(L"[*] Allocating memory at the process...");
		if((lpBaseAddress = ::VirtualAllocEx(process, NULL, dll_strsize,
		 	MEM_COMMIT, PAGE_READWRITE)) == NULL) {
			show_err(L"Allocation failed: " + std::to_wstring(GetLastError()));
			return;
		}
		
		wchar_t buf[32];
		swprintf(buf, 32, L"0x%llX", lpBaseAddress);
		log(L"[*] Writing DLL path to " + (std::wstring)buf + L"...");
		if(::WriteProcessMemory(process, lpBaseAddress, dll_path.c_str(), dll_strsize, NULL) == 0) {
		    show_err(L"Write failed: " + std::to_wstring(GetLastError()));
			return;
		}
		
		log(L"[*] Getting handler to kernel32.dll...");
		if((kernel32_base = ::GetModuleHandleA("kernel32.dll")) == 0) {
			show_err(L"Module handler to kernel32.dll could not be retrieved: " + 
					 std::to_wstring(GetLastError()));
			return;
		}
	
		log(L"[*] Creating remote thread to load " + dll_path);
		if((thread = ::CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE) 
			::GetProcAddress(kernel32_base, "LoadLibraryW"), lpBaseAddress, 0, NULL)) == NULL) { 
			show_err(L"Remote thread execution failed: " + std::to_wstring(GetLastError()));
			return;
		}
	
		log(L"[+] Thread created\r\n"
	         "[*] Waiting...");
		::WaitForSingleObject(thread, 2000);
		::GetExitCodeThread(thread, &exit_code);
	
		// Some cleaning...
		::VirtualFreeEx(process, lpBaseAddress, 0, MEM_RELEASE);
		::CloseHandle(thread);
	
		std::wstring exit_msg = exit_code != 0 ? ( exit_code == 259 ? L"OK | STILL_ACTIVE" : L"OK | FINISHED") : L"FAILED";
		log(L"[+] Done -> Exit code: " + std::to_wstring(exit_code) + L" (" + exit_msg + L")");
	};
	
	static void _eject_dll(HANDLE process, std::wstring dll_path, DWORD pid) {
		HANDLE thread = 0;
		HMODULE kernel32_base = 0;
		unsigned long exit_code = 0;
		
    	// Find HMODULE
    	log(L"[*] Listing modules of process " + std::to_wstring(pid));
    	HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    	if (snap == INVALID_HANDLE_VALUE) {
        	show_err(L"[*] Listing modules of process "  + std::to_wstring(pid) +
					 L" failed: " + std::to_wstring(GetLastError()));
        	return;
    	}
    	
    	MODULEENTRY32 module = {0};
    	module.dwSize = sizeof(module);
    	void* remote_base = NULL;
    	std::wstring target = ::PathFindFileName(dll_path.c_str());

    	if (::Module32First(snap, &module)) {
        	do {
            	if (_wcsicmp(module.szModule, target.c_str()) == 0) {
                	remote_base = module.modBaseAddr;
                	wchar_t buf[32];
					swprintf(buf, 32, L"0x%llX", remote_base);
                	log(L"[+] Module '" + (std::wstring)module.szModule + 
						L"' found -> Base Address: " + (std::wstring)buf);
                	break;
            	}
        	} while (::Module32Next(snap, &module));
    	}
    	::CloseHandle(snap);

    	if (!remote_base) {
        	show_err(L"DLL not loaded in target process");
        	return;
    	}

   		// CreateRemoteThread -> FreeLibraryW(remote_base)
    	kernel32_base = ::GetModuleHandleA("kernel32.dll");
    	if(kernel32_base == NULL) {
			show_err(L"Module handler to kernel32.dll could not be retrieved: " + 
					 std::to_wstring(GetLastError()));
			return;
		}
    	
    	log(L"[*] Creating remote thread to unload " + dll_path);
		if((thread = ::CreateRemoteThread(process, NULL, 0, 
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(kernel32_base, "FreeLibrary"), 
			remote_base, 0, NULL)) == NULL) { 
			show_err(L"Remote thread execution failed: " + std::to_wstring(GetLastError()));
			return;
		}
    	
    	log(L"[+] Thread created\r\n"
	         "[*] Waiting...");
		::WaitForSingleObject(thread, 2000);
		::GetExitCodeThread(thread, &exit_code);
	
		// Some cleaning...
		::CloseHandle(thread);
	
		std::wstring exit_msg = exit_code != 0 ? ( exit_code == 259 ? L"OK | STILL_ACTIVE" : L"OK | FINISHED") : L"FAILED";
		log(L"[+] Done -> Exit code: " + std::to_wstring(exit_code) + L" (" + exit_msg + L")");
	}
	
	static HANDLE find_process(HWND parent, const std::wstring& exe_path, DWORD* save_pid) {
    	w = parent;
    	process_found = false;
    	std::wstring exe_name = ::PathFindFileName(exe_path.c_str());

    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        show_err(L"Process listing failed: " + std::to_wstring(GetLastError()));
        return NULL;
    }

    PROCESSENTRY32 pe32 = {0};
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE process = NULL;

    if (Process32First(snapshot, &pe32)) {
    	log(L"[*] Listing processes...\n");
        do {
            if (wcscmp(pe32.szExeFile, exe_name.c_str()) == 0) {
            	
				log(L"[+] Process running '" + exe_name + L"' found -> PID: " + 
					std::to_wstring(pe32.th32ProcessID) + 
					L"\r\n[*] Getting a handle to the process...");
				if(save_pid != NULL) 
					*save_pid = pe32.th32ProcessID;
					
                process = ::OpenProcess(PROCESS_ALL_ACCESS, TRUE, pe32.th32ProcessID);
                if (!process) {
                    show_err(L"Process opening failed");
                } else {
                    process_found = true;
                }
                break;
            }
        } while (Process32Next(snapshot, &pe32));
    }

    ::CloseHandle(snapshot);

    if (!process_found && process == NULL) {
        show_err(L"Process running '" + exe_name + L"' not found!");
    }
    return process;
}

	void inject_dll(HWND parent, std::wstring dll_path, std::wstring exe_path, HWND log_window) {
		h_log = log_window;
		log(L"**** RUNNING DLL INJECTION ****");
    	HANDLE process = find_process(parent, exe_path, NULL);
    	if (process) {
       		_inject_dll(process, dll_path);
       	 	::CloseHandle(process);
   	 	}
   	 	log(L"**** END OF DLL INJECTION LOGS ****\r\n");
	}

	void eject_dll(HWND parent, std::wstring dll_path, std::wstring exe_path, HWND log_window) {
		h_log = log_window;
		log(L"**** RUNNING DLL EJECTION ****");
		DWORD pid = 0;
    	HANDLE process = find_process(parent, exe_path, &pid);
    	if (process && pid) {
        	_eject_dll(process, dll_path, pid);
        	::CloseHandle(process);
    	}
    	log(L"**** END OF DLL EJECTION LOGS ****\r\n");
	}
};

#endif