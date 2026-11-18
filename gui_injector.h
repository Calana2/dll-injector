#include "winlamb/window_main.h"
#include "winlamb/button.h"
#include "winlamb/label.h"
#include "winlamb/textbox.h"
#include "winlamb/subclass.h"
#include "winlamb/sysdlg.h"
#include "resource.h"

#define ID_BTN_CONFIRM 1000

#define ID_BTN_SELECT_DLL 1001
#define ID_LABEL_PATH_DLL 1002
#define ID_TEXTBOX_PATH_DLL  1003

#define ID_BTN_SELECT_EXE 1004
#define ID_LABEL_PATH_EXE 1005
#define ID_TEXTBOX_PATH_EXE  1006

#define ID_BTN_CONFIRM_EJECT 1100

#define ID_BTN_SELECT_DLL_EJECT 1101
#define ID_LABEL_PATH_DLL_EJECT 1102
#define ID_TEXTBOX_PATH_DLL_EJECT  1103

#define ID_BTN_SELECT_EXE_EJECT 1104
#define ID_LABEL_PATH_EXE_EJECT 1105
#define ID_TEXTBOX_PATH_EXE_EJECT 1106

#define ID_TEXTBOX_LOG 1200

#define SMALL_X_MARGIN 20
#define SMALL_Y_MARGIN 10
#define BTN_CON_Y_MARGIN 40

#define LABEL_X_MARGIN 20
#define LABEL_Y_MARGIN 30

#define BETWEEN_FORM_Y_PADDING 10

const wchar_t* DLL_FILTER = L"Dynamic libraries|*.dll|All the files|*.*";
const wchar_t* EXEC_FILTER = L"Executable files|*.exe|All the files|*.*";

class My_Window : public wl::window_main {
public:
    My_Window();
private: 
    // inject form 1
	wl::label label_dll_path;
	wl::textbox tb_dll_path;
	wl::button btn_select_dll;
	wl::subclass subclass_tb_dll;
	// inject form 2
	wl::label label_exe_path;
	wl::textbox tb_exe_path;
	wl::button btn_select_exe;
	wl::subclass subclass_tb_exe;
	// inject button
	wl::button btn_confirm;	
	
	// eject form 1
	wl::label label_dll_path_ej;
	wl::textbox tb_dll_path_ej;
	wl::button btn_select_dll_ej;
	wl::subclass subclass_tb_dll_ej;
	// eject form 2
	wl::label label_exe_path_ej;
	wl::textbox tb_exe_path_ej;
	wl::button btn_select_exe_ej;
	wl::subclass subclass_tb_exe_ej;
	// eject button
	wl::button btn_confirm_ej;	
	
	// log console
	//wl::textbox tb_log_console;
	HWND tb_log_console;
};

namespace form_subclass {
	bool ctrl_down = false;
	
	inline void install(wl::textbox& txt, wl::subclass& sc){
	  sc.install_subclass(txt);		
	  
	  sc.on_message(WM_KEYDOWN, [&txt](wl::wm::keydown p) -> LRESULT {
			if(p.vkey_code() == VK_CONTROL) {
				ctrl_down = true;
				return 0;
			}
			else if(ctrl_down && p.vkey_code() == 'A'){
				txt.select_all();
				return 0;
			}
		    return ::DefSubclassProc(txt.hwnd(), WM_KEYDOWN, p.vkey_code(), p.lParam);
	   });		
		
	   sc.on_message(WM_KEYUP, [&txt](wl::wm::keydown p) -> LRESULT {
			if(p.vkey_code() == VK_CONTROL) {
				ctrl_down = false;
		    	return 0; 
			}
			 return ::DefSubclassProc(txt.hwnd(), WM_KEYUP, p.vkey_code(), p.lParam);
	    });
		
		sc.on_message(WM_CHAR, [&txt](wl::wm::char_ p) -> LRESULT {
		 if(ctrl_down && (p.char_code() == 1 || p.char_code() == 'A'))
		 	return 0;
		 return ::DefSubclassProc(txt.hwnd(), WM_CHAR, p.char_code(), p.lParam);
		});
	}
};

namespace utils {
  inline int select_file(HWND w, wl::textbox& tb, const wchar_t* filter) {
  	std::wstring path;
 	if(wl::sysdlg::open_file(w, filter, path))
 		tb.set_text(path);
    return 0;
   }
   
   inline BOOL CALLBACK set_font_to_child_proc(HWND hwndChild, LPARAM lParam) {
    	::SendMessageW(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
    	return TRUE;
   }

	inline void apply_font_to_all_controls(HWND hwndParent, HFONT hFont) {
    	::EnumChildWindows(hwndParent, set_font_to_child_proc, (LPARAM)hFont);
	}	
};