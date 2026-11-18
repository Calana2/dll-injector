// I used MinGW
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <shlwapi.h>
#include <ctype.h>

#define DIE(...) \
do { \
	fprintf(stderr, __VA_ARGS__); \
	exit(1); \
} while(0)

#define bool int
#define true 1

enum ACTION { INJECT, EJECT};

void print_usage_and_exit(char* const argv0) {
	fprintf(stderr, "Usage: %s --dll <path_to_dll> --exe <executable_name> [--action INJECT|EJECT]\n", argv0);
	exit(0); 
}

char* str_tolower (char* p) { char* start = p; for( ; *p ; ++p) { *p = tolower(*p); }; return start; } 

void parse_args(int argc, char* const argv[], char** dll_path, char** exe_name, enum ACTION* action) {
	int opt;
    static struct option long_options[] = {
       {"help", no_argument, 0, 'h'},
       {"dll", required_argument, 0 , 'd'},
       {"exe", required_argument, 0, 'e'},
       {"action", required_argument, 0, 'a'},
       {0, 0, 0, 0}
    };
		
	while((opt = getopt_long(argc, argv, "hd:e:a:", long_options, NULL)) != -1) {
		switch(opt) {
			case 'h':
				print_usage_and_exit(argv[0]);
			case 'd':	
			    *dll_path = (char*)malloc(_MAX_PATH + 1);
				strcpy(*dll_path, optarg);
				break;
			case 'e':
				*exe_name = (char*)malloc(strlen(optarg) + 1);
				strcpy(*exe_name, optarg);
				break;	
			case 'a':
				if(strcmp(str_tolower(optarg), "eject") == 0)
					*action = EJECT;	
		}
	}
	
	if(*dll_path == NULL || *exe_name == NULL) {
		print_usage_and_exit(argv[0]);
	}
	
	DWORD len_fpath = GetFullPathName(*dll_path, _MAX_PATH, *dll_path, NULL);
	if (len_fpath == 0 || len_fpath >= MAX_PATH)
	   DIE("[!] Couldn't resolve full path for '%s'", *dll_path);
}

void inject_dll(HANDLE process, const char* dll_path) {
	void *lpBaseAddress = NULL;
	size_t dll_strsize = strlen(dll_path) + 1;
	
	HANDLE thread = 0;
	HMODULE kernel32_base = 0;
	unsigned long exit_code = 0;
	
	printf("[*] Allocating memory at the process...\n");
	if((lpBaseAddress = VirtualAllocEx(process, NULL, dll_strsize,
		 MEM_COMMIT, PAGE_READWRITE)) == NULL)
		DIE("[!] Allocation failed: %d\n", GetLastError());
		
	printf("[*] Writing DLL path to 0x%p...\n", lpBaseAddress);
	if(WriteProcessMemory(process, lpBaseAddress, dll_path, dll_strsize, NULL) == 0)
		DIE("[!] Write failed: %d\n", GetLastError());
		
	printf("[*] Getting handler to kernel32.dll...\n");
	if((kernel32_base = GetModuleHandleA("kernel32.dll")) == 0)
		DIE("[!] Module handler to kernel32.dll could not be retrieved: %d\n", GetLastError());
	
	printf("[*] Creating remote thread to load %s\n", dll_path);
	if((thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)
		GetProcAddress(kernel32_base, "LoadLibraryA"), lpBaseAddress, 0, NULL)) == NULL)
		DIE("[!] Remote thread execution failed: %d\n", GetLastError());
	
	printf("[+] Thread created\n\n"
	       "[*] Waiting...\n");
	WaitForSingleObject(thread, 3000);
	GetExitCodeThread(thread, &exit_code);
	
	// Some cleaning...
	VirtualFreeEx(process, lpBaseAddress, 0, MEM_RELEASE);
	CloseHandle(thread);
	
	const char *exit_msg = exit_code != 0 ? ( exit_code == 259 ? "OK | STILL_ACTIVE" : "OK | FINISHED") : "FAILED";
	
	printf("[+] Done -> Exit code: %d (%s)\n", exit_code, exit_msg);
};

void eject_dll(HANDLE process, const char* dll_name, DWORD pid) {
	HANDLE thread = 0;
	HMODULE kernel32_base = 0;
	unsigned long exit_code = 0;
		
    // Find HMODULE
    printf("[*] Listing modules of process %d\n", pid);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE)
        DIE("[*] Listing modules of process %d failed: %d\n", pid, GetLastError());
    	
    MODULEENTRY32 module = {0};
    module.dwSize = sizeof(module);
    void* remote_base = NULL;

    if (Module32First(snap, &module)) {
        do {
            if (strcmp(module.szModule, dll_name) == 0) {
                remote_base = module.modBaseAddr;
                wchar_t buf[32];
				swprintf(buf, 32, L"0x%llX", remote_base);
                printf("[+] Module '%s' found -> Base Address: 0x%p\n", module.szModule, remote_base);
                	break;
            }
        } while (Module32Next(snap, &module));
    }
    CloseHandle(snap);
    
    if (!remote_base)
        DIE("[!] DLL not loaded in target process\n");

   	// CreateRemoteThread -> FreeLibraryW(remote_base)
    kernel32_base = GetModuleHandleA("kernel32.dll");
    if(kernel32_base == NULL)
		DIE("[!] Module handler to kernel32.dll could not be retrieved: %s\n", GetLastError());

    printf("[*] Creating remote thread to unload %s\n", dll_name);
		if((thread = CreateRemoteThread(process, NULL, 0, 
			(LPTHREAD_START_ROUTINE) GetProcAddress(kernel32_base, "FreeLibrary"), 
			remote_base, 0, NULL)) == NULL) 
			DIE("[!] Remote thread execution failed: %d\n", GetLastError());
    	
    	printf("[+] Thread created\n\n"
	           "[*] Waiting...\n");
		WaitForSingleObject(thread, 2000);
		GetExitCodeThread(thread, &exit_code);
		
		// Some cleaning...
		CloseHandle(thread);
	
		const char* exit_msg = exit_code != 0 ? ( exit_code == 259 ? "OK | STILL_ACTIVE" : "OK | FINISHED") : "FAILED";
		printf("[+] Done -> Exit code: %d (%s)\n", exit_code, exit_msg);
}

int main(int argc, char *argv[]) {
	// 1. Parse
	char *dll_path = NULL;
	char *exe_name = NULL;
	enum ACTION action = INJECT;
	
	parse_args(argc, argv, &dll_path, &exe_name, &action);
    
	// 2. Find the process
	HANDLE process = NULL;
	HANDLE snapshot = 0;
	PROCESSENTRY32 pe32 = {0};
	
	pe32.dwSize = sizeof(PROCESSENTRY32); 
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if (Process32First(snapshot, &pe32)) {
		printf("[*] Listing processes...\n");
		do {
			if (strcmp(pe32.szExeFile, exe_name) == 0) {
				printf("[+] Process running %s found. PID: %d\n\n", exe_name, pe32.th32ProcessID);
				
				// 3. Inject
				printf("[*] Getting a handle to the process...\n");
				if((process = OpenProcess(PROCESS_ALL_ACCESS, true, pe32.th32ProcessID)) == NULL)
					DIE("[!] Process opening failed: %d\n", GetLastError());
				if(action == INJECT)
					inject_dll(process, dll_path);
				else {
					const char* dll_name = PathFindFileName(dll_path);
					eject_dll(process, dll_name, pe32.th32ProcessID); 
				}		
				
				// Cleanup
				CloseHandle(snapshot);
				CloseHandle(process);
				free(exe_name);
				free(dll_path);
				return 0;
			}
		} while (Process32Next(snapshot, &pe32));
	} else {
	  DIE("[!] Process listing failed: %d\n", GetLastError());
	}
	
	DIE("[!] Process running %s not found!\n", exe_name);	
}