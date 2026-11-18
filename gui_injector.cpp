#include <windows.h>
#include "gui_injector.h"
#include "injector.h"

//g++ gui_injector.cpp icon.o -o example -lcomctl32 -lshlwapi -municode -mwindows

My_Window::My_Window()
{
    setup.wndClassEx.lpszClassName = L"INJECTOR_CLASS_NAME"; 
    setup.title = L"mkaja-dllinjector";
    setup.style |= WS_MINIMIZEBOX;
    setup.size = {800, 400};
    
    // Icon
    HINSTANCE hInst = GetModuleHandle(nullptr);
    HICON hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCE(IDI_MAIN_ICON), IMAGE_ICON,
				                    0, 0, LR_DEFAULTSIZE|LR_SHARED);
    setup.wndClassEx.hIcon = hIcon;
    setup.wndClassEx.hIconSm = hIcon;
    setup.wndClassEx.hbrBackground = CreateSolidBrush(RGB(0x78, 0x7f, 0x89));
    
    // Center
    int x_center = (GetSystemMetrics(SM_CXSCREEN) - setup.size.cx) / 2;
    int y_center = (GetSystemMetrics(SM_CYSCREEN) - setup.size.cy) / 2;
    setup.position = {x_center, y_center};

    //my_font.create(L"Segoe UI", 11, false);

    on_message(WM_CREATE, [this](wl::wm::create p) -> LRESULT {
    	// set name
        set_text(L"mkaja-DLLinjector");
        
        int btn_width = 73;
		int btn_height = 25;
		int text_field_w = btn_width * 3 + (btn_width / 2);
		int text_field_h = btn_height;
        
		// create dll forms
		// inject
		label_dll_path.create(this->hwnd(), ID_LABEL_PATH_DLL, L"DLL to inject", 
			{ LABEL_X_MARGIN, SMALL_Y_MARGIN}, 
			{text_field_w , text_field_h });
		
		tb_dll_path.create(this->hwnd(), ID_TEXTBOX_PATH_DLL, 
					wl::textbox::type::NORMAL, { LABEL_X_MARGIN, SMALL_Y_MARGIN + LABEL_Y_MARGIN },
					text_field_w , text_field_h);
					
		btn_select_dll.create(this->hwnd(), ID_BTN_SELECT_DLL, L"...", 
			{ LABEL_X_MARGIN + text_field_w + SMALL_X_MARGIN, SMALL_Y_MARGIN + LABEL_Y_MARGIN },
			{btn_width, btn_height});
		
		// eject
		label_dll_path_ej.create(this->hwnd(), ID_LABEL_PATH_DLL_EJECT, L"DLL to eject", 
				{ LABEL_X_MARGIN + text_field_w + btn_width + SMALL_X_MARGIN * 2, SMALL_Y_MARGIN}, 
				{text_field_w , text_field_h });

		tb_dll_path_ej.create(this->hwnd(), ID_TEXTBOX_PATH_DLL_EJECT, 
				wl::textbox::type::NORMAL, 
				{ LABEL_X_MARGIN + text_field_w + btn_width + SMALL_X_MARGIN * 2, SMALL_Y_MARGIN + LABEL_Y_MARGIN}, 
				text_field_w , text_field_h);
				
		btn_select_dll_ej.create(this->hwnd(), ID_BTN_SELECT_DLL_EJECT, L"...", 
				{ LABEL_X_MARGIN + text_field_w * 2 + btn_width + SMALL_X_MARGIN * 3, SMALL_Y_MARGIN + LABEL_Y_MARGIN}, 
				{btn_width, btn_height});
		//aaa
		// create exec forms
		// inject
		label_exe_path.create(this->hwnd(), ID_LABEL_PATH_EXE, L"Process executable", 
					{ LABEL_X_MARGIN, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 2}, 
					{text_field_w , text_field_h });
		
		tb_exe_path.create(this->hwnd(), ID_TEXTBOX_PATH_EXE, 
					wl::textbox::type::NORMAL, { LABEL_X_MARGIN, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 3}, 
					text_field_w , text_field_h);
					
		btn_select_exe.create(this->hwnd(), ID_BTN_SELECT_EXE, L"...", 
					{ LABEL_X_MARGIN + text_field_w + SMALL_X_MARGIN, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 3}, 
					{btn_width, btn_height});
		// eject
		label_exe_path_ej.create(this->hwnd(), ID_LABEL_PATH_EXE_EJECT, L"Process executable", 
					{ LABEL_X_MARGIN + text_field_w + btn_width + SMALL_X_MARGIN * 2, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 2}, 
					{text_field_w , text_field_h });
		
		tb_exe_path_ej.create(this->hwnd(), ID_TEXTBOX_PATH_EXE_EJECT, 
					wl::textbox::type::NORMAL, { 
					LABEL_X_MARGIN + text_field_w + btn_width + SMALL_X_MARGIN * 2, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 3}, 
					text_field_w , text_field_h);
		
		btn_select_exe_ej.create(this->hwnd(), ID_BTN_SELECT_EXE_EJECT, L"...", 
					{  LABEL_X_MARGIN + text_field_w * 2 + btn_width + SMALL_X_MARGIN * 3, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 3}, 
					{btn_width, btn_height});
		
		// Copy text from a form with Ctrl+A
		// inject
		form_subclass::install(tb_dll_path, subclass_tb_dll);
		form_subclass::install(tb_exe_path, subclass_tb_exe);
		// eject
		form_subclass::install(tb_dll_path_ej, subclass_tb_dll_ej);
		form_subclass::install(tb_exe_path_ej, subclass_tb_exe_ej);
					
		// create 'inject' button
		btn_confirm.create(this->hwnd(), ID_BTN_CONFIRM, L"Inject", 
				   { LABEL_X_MARGIN + text_field_w + SMALL_X_MARGIN, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 4 }, 
				   {btn_width, btn_height});
		// create 'eject' button
		btn_confirm_ej.create(this->hwnd(), ID_BTN_CONFIRM_EJECT, L"Eject", { 
				   LABEL_X_MARGIN + text_field_w * 2 + btn_width + SMALL_X_MARGIN * 3, SMALL_Y_MARGIN + LABEL_Y_MARGIN * 4}, 
				   {btn_width, btn_height});
		
		// create log console (manual)
		tb_log_console = CreateWindowExW(
    		WS_EX_CLIENTEDGE,
    		L"EDIT",
    		L"",
    		WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        	ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
    		5, setup.size.cy - 220,
    		setup.size.cx - 17, 180,
    		this->hwnd(),
    		(HMENU)(INT_PTR)ID_TEXTBOX_LOG,
    		GetModuleHandleW(nullptr),
    		nullptr);
    	// read-only, no text limit
    	SendMessageW(tb_log_console, EM_SETREADONLY, TRUE, 0);
		SendMessageW(tb_log_console, EM_SETLIMITTEXT, 0, 0);
		// autoscroll
		LONG_PTR style = GetWindowLongPtrW(tb_log_console, GWL_STYLE);
		style |= WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY;
		SetWindowLongPtrW(tb_log_console, GWL_STYLE, style);
		SetWindowPos(tb_log_console, nullptr, 0, 0, 0, 0,
        	SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // ????			
        return 0;
    });
    
    // Select files on click  
    // inject
    on_command(ID_BTN_SELECT_DLL, [this](wl::params p) -> INT_PTR  {
		return utils::select_file(this->hwnd(), tb_dll_path, DLL_FILTER);
    });
    on_command(ID_BTN_SELECT_EXE, [this](wl::params p) -> INT_PTR  {
    	return utils::select_file(this->hwnd(), tb_exe_path, EXEC_FILTER);
    });
    // eject
    on_command(ID_BTN_SELECT_DLL_EJECT, [this](wl::params p) -> INT_PTR  {
		return utils::select_file(this->hwnd(), tb_dll_path_ej, DLL_FILTER);
    });
    on_command(ID_BTN_SELECT_EXE_EJECT, [this](wl::params p) -> INT_PTR  {
    	return utils::select_file(this->hwnd(), tb_exe_path_ej, EXEC_FILTER);
    });

    // Inject on click
    on_command(ID_BTN_CONFIRM, [this](wl::params p) -> INT_PTR {
    	injector::inject_dll(this->hwnd(), tb_dll_path.get_text(), tb_exe_path.get_text(), tb_log_console);
    	return 0;
    });
    // Eject on click
    on_command(ID_BTN_CONFIRM_EJECT, [this](wl::params p) -> INT_PTR {
    	injector::eject_dll(this->hwnd(), tb_dll_path_ej.get_text(), tb_exe_path_ej.get_text(), tb_log_console);
    	return 0;
    });
}

RUN(My_Window) // generate WinMain call and instantiate My_Window
