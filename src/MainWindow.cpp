#include "MainWindow.h"
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <iostream>
#include "util.h"
#include "ChatClient.h"
#define VK_ENTER VK_RETURN

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK TextEditWndProc(HWND textEdit, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	switch(msg)
	{
	case WM_CHAR:	
		if(wParam == VK_RETURN && !(GetAsyncKeyState(VK_LSHIFT) & 0x8000))
		{						
			MainWindow* window = (MainWindow*)GetPropW(textEdit, L"WINDOW");			
			WCHAR buffer[4000];
			int length = GetWindowTextW(textEdit, buffer, 4000);		
			if(length > 0 && window->chatClient != NULL)
			{				
				window->chatClient->SendChatMessage(buffer, length);
				SetWindowTextW(textEdit, L"");
				SendMessageW(textEdit, EM_SETSEL, 0,0);			
				window->reChatBox.ScrollToBottom();
			}
			wParam = 0;									
		}
	break;
	case WM_LBUTTONDOWN:
		SetFocus(textEdit);
	break;
	}
	return CallWindowProcW((WNDPROC)GetPropW(textEdit, L"OLDPROC"), textEdit, msg, wParam, lParam);
}

MainWindow* window = nullptr;

bool MainWindow::Create(HINSTANCE hInst, int x, int y, int width, int height, MainWindow* out) {	
	//::window = this;
	if(!windowClassRegistered)  
	{
		MessageBoxW(0, L"MainWindow::RegisterWindowClass needs to be invoked prior to creating a MainWindow object", L"Error", 0);
		std::exit(0);
	}
  out->hwnd = CreateWindowExW(0,L"WINDOW", L"ChatRoom",
                       WS_OVERLAPPEDWINDOW | WS_VISIBLE, x, y, width, height,
                       NULL, NULL, hInst, NULL);	
	if(!out->hwnd)											
		return false;
	SetPropW(out->hwnd, L"WINDOW", out);		
	out->OnCreate(out->hwnd);		
	return true;		
}
MainWindow& MainWindow::SetDimensions(uint32_t width, uint32_t height) {
	this->width = width;
	this->height = height;
	return *this;
}
MainWindow& MainWindow::SetPosition(int x, int y) {
	this->posX = x;
	this->posY = y;
	return *this;
}

MainWindow& MainWindow::SetTitle(const std::wstring& title) {
	this->title = title;
	return *this;
}

void MainWindow::SetChatClient(ChatClient* chatClient) { 
	this->chatClient = chatClient;
	
}

void MainWindow::RegisterWindowClass(HINSTANCE hInstance) {
	if(windowClassRegistered)
		return;
	windowClassRegistered = true;
	WNDCLASSW wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszClassName = L"WINDOW";
	wc.hInstance = hInstance;
	wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc.lpszMenuName = NULL;
	wc.lpfnWndProc = MainWindowProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	
	RegisterClassW(&wc);
}

void MainWindow::AppendUserMessage(std::wstring username, std::wstring message)
{
	std::scoped_lock<std::mutex> lck{chatBoxMutex};
	if(!reChatBox) return;		
	std::wstring usernameResult;
	usernameResult.reserve(username.size() + 6);
	usernameResult += L"[";
	usernameResult += username;
	usernameResult += L"]: ";

	message += L"\n";
	reChatBox
	.AppendText(RGB(0,0,0), RichEdit::Bold, usernameResult.data())
	.AppendText(RGB(0,0,0), RichEdit::None, message.data());	
}

void MainWindow::AppendChatBox(COLORREF color, RichEdit::FontStyle style, std::wstring message) {
	std::scoped_lock<std::mutex> lck{chatBoxMutex};
	if(!reChatBox) return;
	reChatBox.AppendText(color, style, message.data());
}

bool MainWindow::SetMemberListFromJsonArray(rapidjson::Value::Array& arr)
{
	std::scoped_lock<std::mutex> lck{lbMembersMutex};	
	SendMessageW(lbMembers, LB_RESETCONTENT, 0,0);	
	for(auto it = arr.Begin(), end = arr.End(); it != end; ++it)
	{
		if(it->IsString())		
		{
			std::wstring str = util::mbstowcs(it->GetString(), it->GetStringLength());
			SendMessageW(lbMembers, LB_ADDSTRING, 0, (LPARAM)str.data());
		}
	}
}

bool MainWindow::AppendToMemberList(const char* memberName, int memberNameLen) {
	std::scoped_lock<std::mutex> lck{lbMembersMutex};	
	std::wstring memberNameW = util::mbstowcs(memberName, memberNameLen);
	SendMessageW(lbMembers, LB_ADDSTRING, 0, (LPARAM)memberNameW.data());
}


WPARAM MainWindow::Run()
{  										 		
	ShowWindow(hwnd, nCmdShow);	
	UpdateWindow(hwnd);
	
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return msg.wParam;
}


//utility
void MainWindow::Resize(uint32_t width, uint32_t height)
{
	SetWindowPos(this->hwnd, HWND_TOP, posX, posY, width, height, SWP_NOOWNERZORDER | SWP_NOMOVE);
}




LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{						
	MainWindow* window = (MainWindow*)GetPropW(hwnd, L"WINDOW");
	auto ShowMessageBox = std::bind(util::ShowMessageBox, 
		std::placeholders::_1, std::placeholders::_2, 0);
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
	break;
	case WM_PAINT:
		if(!window) break;		
	break;
	case WM_KEYDOWN:
	{
		if(!window)
		{									
			break;
		} 


	}
	break;
	case WM_KEYUP:
	{
		if(!window) 
			break;

	}
	break;
	case WM_SIZE:
		if(!window) 
			break;
		window->width = LOWORD(lParam);
		window->height = HIWORD(lParam);

	break;	
	case WM_WINDOWPOSCHANGED:
		if(!window) 

			break;
		window->posX = LOWORD(lParam);
		window->posY = HIWORD(lParam);
	break;
	case WM_COMMAND:		
	{	
		MainWindow::ButtonId id = (MainWindow::ButtonId)LOWORD(wParam);
		if(id == MainWindow::ButtonId::Submit && window->chatClient != NULL)
		{
			WCHAR buffer[4000];
			int length = GetWindowTextW(window->textEdit, buffer, 4000);
			if(length == 0)
			{
				break;
			}
			window->chatClient->SendChatMessage(buffer, length);
			SetWindowTextW(window->textEdit, L"");
			SendMessage(window->textEdit, EM_SETSEL, 0,0);
		}		
	}
	break;
	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
	break;
	case WM_CREATE:

	break;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}




MainWindow& MainWindow::OnCreate(HWND hwnd) {	
	textEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE,
		0,300, 350,200, hwnd, NULL,hInstance, NULL);	
			
	auto oldTextEditProc = SetWindowLongW(textEdit, GWLP_WNDPROC, (LONG_PTR)TextEditWndProc);		
	SetPropW(textEdit, L"OLDPROC", (HANDLE)oldTextEditProc);
		
	if(!RichEdit::Create(hwnd, hInstance, 0,0,350,290, true, &reChatBox))
	{
		MessageBoxW(0, L"Failed to create textbox for chat messages", L"Error", 0);
		std::exit(0);
	}
	CreateWindowExW(0, L"STATIC", L"Users:", WS_CHILD | WS_VISIBLE,
	370, 0, 100, 30,hwnd, NULL, hInstance, NULL);
	lbMembers = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
		370, 30, 175, 290, hwnd, NULL, hInstance, NULL
	);
	SetPropW(hwnd, L"CHATBOX", (HANDLE)&reChatBox);
	SetPropW(hwnd, L"WINDOW", this);

	SetPropW(textEdit, L"CHATBOX", (HANDLE)&reChatBox);
	SetPropW(textEdit, L"WINDOW", this);
	btnSubmit = CreateWindowExW(0, L"BUTTON", L"Submit", WS_CHILD | WS_VISIBLE,
	350,300,100,40, hwnd, (HMENU)ButtonId::Submit, hInstance, NULL);	
	
	return *this;
}
