SHELL=cmd
BUILDIR=bin
SAMPLEDIR=examples

GCC32 = "C:\winlibs\mingw32\bin\gcc.exe"
G++32 = "C:\winlibs\mingw32\bin\g++.exe"

GCC64 = "C:\winlibs\mingw64\bin\gcc.exe"
G++64 = "C:\winlibs\mingw64\bin\g++.exe"

WINDRES32 = "C:\winlibs\mingw32\bin\windres.exe"
WINDRES64 = "C:\winlibs\mingw64\bin\windres.exe"

STRIP32 = "C:\winlibs\mingw32\bin\strip.exe"
STRIP64 = "C:\winlibs\mingw64\bin\strip.exe"

CFLAGS_I386  = -O2
LDFLAGS_I386 = -static -lshlwapi
CPP_FLAGS_I386 = -O2 -w -fpermissive
CPP_LDFLAGS_I386 = -static -lcomctl32 -lshlwapi -municode -mwindows

CFLAGS_X64   = -O2
LDFLAGS_X64  = -static -lshlwapi
CPP_LDFLAGS_X64 = -static -lcomctl32 -lshlwapi -municode -mwindows

CPP_DLLFLAGS = -shared -static

all: samples injectors

injectors: cli_injectors gui_injectors

cli_injectors: injector32 injector64 

gui_injectors: guinjector32 guinjector64

samples: sample_dll sample_safe_dll

sample_dll: $(SAMPLEDIR)\windowsdll_example.cpp
	$(G++32) -o "$(SAMPLEDIR)\sample_dll_pe32.dll" $< $(CPP_DLLFLAGS)
	$(G++64) -o "$(SAMPLEDIR)\sample_dll_pe32+.dll" $< $(CPP_DLLFLAGS)

sample_safe_dll: $(SAMPLEDIR)\windowsdll_safe_eject_example.cpp
	$(G++32) -o "$(SAMPLEDIR)\sample_dll_safe_eject_pe32.dll" $< $(CPP_DLLFLAGS)
	$(G++64) -o "$(SAMPLEDIR)\sample_dll_safe_eject_pe32+.dll" $< $(CPP_DLLFLAGS)
	
injector32: injector.c | build
	$(GCC32) $(CFLAGS_I386) -o "$(BUILDIR)\$@.exe" $< $(LDFLAGS_I386)
	$(STRIP32) --strip-debug "$(BUILDIR)\$@.exe"

injector64: injector.c | build
	$(GCC64) $(CFLAGS_X64) -o "$(BUILDIR)\$@.exe" $< $(LDFLAGS_X64)
	$(STRIP64) --strip-debug "$(BUILDIR)\$@.exe"

guinjector32: gui_injector.cpp gui_injector.h injector.h resource.h icon32.o | build
	$(G++32) $(CPP_FLAGS_I386) -o "$(BUILDIR)\$@.exe" $(filter-out %.h, $^) $(CPP_LDFLAGS_I386)
	$(STRIP32) --strip-debug "$(BUILDIR)\$@.exe"

guinjector64: gui_injector.cpp gui_injector.h injector.h resource.h icon64.o | build
	$(G++64) $(CFLAGS_X64) -o "$(BUILDIR)\$@.exe" $(filter-out %.h, $^) $(CPP_LDFLAGS_X64)
	$(STRIP64) --strip-debug "$(BUILDIR)\$@.exe"

icon32.o: icon.rc icon.ico
	$(WINDRES32) -O COFF -o $@ $< 

icon64.o: icon.rc icon.ico
	$(WINDRES64) -O COFF -o $@ $< 

build:
	-if not exist "$(BUILDIR)" mkdir "$(BUILDIR)"

clean:
	-if exist "$(BUILDIR)" del /Q "$(BUILDIR)\*"
	-if exist "$(SAMPLEDIR)" del /Q "$(SAMPLEDIR)\*.dll"
	del /Q "*.o"

.PHONY: all clean build injectors cli_injectors gui_injectors samples
