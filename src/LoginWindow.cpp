#include "LoginWindow.h"
#include "ChatClient.h"
#include "ChatServer.h"
#include "MainWindow.h"
#include "ServerWindow.h"
#include <windows.h>
#include "util.h"
#include "NetworkMessages.h"
LRESULT CALLBACK LoginWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LoginWindow* window = (LoginWindow*)GetPropW(hwnd, L"LOGINWINDOW");
	HINSTANCE hInstance = (HINSTANCE)GetPropW(hwnd, L"HINSTANCE");	

	switch(msg)
	{		
	case WM_QUIT:
		PostQuitMessage(0);
	break;
	case WM_COMMAND:
	{		
		if(!window || !hInstance)
			break;
		auto id = LOWORD(wParam);
		if(id == LoginWindow::ButtonId::Login)
		{			
			WCHAR ip[255]{};
			GetWindowTextW(window->teIpAddress, ip, 255);
			ChatClient* chatClient = window->chatClient;

			WCHAR port[255]{};
			GetWindowTextW(window->tePort, port, 255);

			ShowWindow(hwnd, SW_HIDE);			
			
			if(!window->mainWindow && !MainWindow::Create(hInstance, 300,300, 600,400, &window->mainWindow))
			{
				MessageBoxW(0, L"Failed to create chat window", L"Error", 0);
				std::exit(0);
			}		
				
			window->mainWindow.SetChatClient(chatClient);				
					
			if(!chatClient->Connect(ip, _wtoi(port)))
			{
				ShowWindow(window->mainWindow.GetHwnd(), SW_HIDE);
				ShowWindow(hwnd, SW_SHOW);
				break;
			}

			WCHAR username[255];
			int usernameLen = GetWindowTextW(window->teUsername, username, 255);
			SetWindowTextW(window->mainWindow.GetHwnd(), fmt::format(L"Chatroom - [{}]", username).data());			
			
			rapidjson::Document doc(rapidjson::kObjectType);
			util::Json_AddMember(doc, "type", NetworkMessage::MEMBER_JOIN);
			util::Json_AddMember(doc, "username", util::wcstombs(username));
			chatClient->SendJsonToServer(util::DocumentToJson(doc));			
		}
		else if(id == LoginWindow::ButtonId::CreateServer)
		{
			ShowWindow(hwnd, SW_HIDE);
			
			if(window->serverWindow.IsNull() && !ServerWindow::Create(hInstance, 300,300, 600,400, &window->serverWindow))
			{
				MessageBoxW(0, L"Failed to create server window", L"Error", 0);
				std::exit(0);
			}
			WCHAR ip[255]{};
			GetWindowTextW(window->teIpAddress, ip, 255);
			ChatClient* chatClient = window->chatClient;

			WCHAR port[255]{};
			GetWindowTextW(window->tePort, port, 255);
			int portAsInt = _wtoi(port);
			window->serverWindow.SetChatServer(window->chatServer);
			window->chatServer->SetDetails(ip, portAsInt);
			window->chatServer->reServerLog = window->serverWindow.GetReServerLog();
			if(!window->chatServer->Listen())
			{
				ShowWindow(hwnd, SW_SHOW);
				break;				
			}
			window->chatServer->Log(fmt::format(L"Server is listening at {}:{}.\n", ip, portAsInt));
		}
	}
	break;	
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK TabProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(msg == WM_CHAR && wparam == VK_TAB)
	{
		const WCHAR* propertyName = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) ? L"PREV" : L"NEXT";
		HWND next = (HWND)GetPropW(hwnd, propertyName);
		SetFocus(next);
	}
	else if(msg == WM_KEYDOWN && wparam == VK_RETURN)
	{
		WPARAM buttonId = (WPARAM)GetPropW(hwnd, L"BUTTONID");
		if(buttonId != 0)
		{
			LoginWindow* window = (LoginWindow*)GetPropW(hwnd, L"LOGINWINDOW");
			if(window != NULL)
			{
				SendMessage(window->GetHwnd(), WM_COMMAND, buttonId, 0);			
			}
		}
	}
	return CallWindowProc((WNDPROC)GetPropW(hwnd, L"OLDPROC"), hwnd, msg, wparam, lparam);
}
void LoginWindow::RegisterWindowClass(HINSTANCE hInstance) {
	if(windowClassRegistered)
		return;
	windowClassRegistered = true;
	WNDCLASSW wc;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.lpszClassName = L"LOGINWINDOW";
  wc.hInstance = hInstance;
  wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  wc.lpszMenuName = NULL;
  wc.lpfnWndProc = LoginWindowProc;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	
	RegisterClassW(&wc);
}

bool LoginWindow::Create(HINSTANCE hInst, int x, int y, int width, int height, LoginWindow* out) {	

	HWND hwnd = out->hwnd = CreateWindowExW(0, L"LOGINWINDOW", L"Login", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
	300,300,300,170, NULL, NULL, hInst, NULL);
	if(hwnd == NULL)
		return false;

	#pragma region HELPERS
	auto GetLabelTextSize = [](HWND staticControl) -> SIZE
	{
		WCHAR buffer[1000];
		int size = GetWindowTextW(staticControl, buffer, 1000);
		SIZE result{};
		GetTextExtentPoint32W(GetDC(staticControl), buffer, size, &result);
		return result;
	};
	auto CreateLabel = [hwnd, hInst](const WCHAR* text, int x, int y, int width = 200, int height = 50) -> HWND
	{
		return CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x,y,width, height, hwnd, NULL, hInst, NULL);
	};
	auto CreateEdit = [hwnd, hInst](const WCHAR* text, int x, int y, int width = 120, int height = 50) -> HWND
	{
		return CreateWindowExW(0, L"EDIT", text, WS_CHILD | WS_VISIBLE | WS_BORDER, x,y,width, height, hwnd, NULL, hInst, NULL);
	};
	#pragma endregion

	SetPropW(hwnd, L"LOGINWINDOW", (HANDLE)out);
	SetPropW(hwnd, L"HINSTANCE", hInst);
	HWND usernameLabel = CreateLabel(L"Username:", 0, 0);
	SIZE size = GetLabelTextSize(usernameLabel);	
	HWND teUsername = CreateEdit(L"User", size.cx, 0, 120, size.cy);	
	
	HWND ipLabel = CreateLabel(L"IP: ", 0, size.cy + 10);
	HWND teIpAddress = CreateEdit(L"127.0.0.1", size.cx, size.cy + 10, 120, size.cy);

	HWND portLabel = CreateLabel(L"Port: ", 0, size.cy + 30);
	HWND tePort = CreateWindowExW(0, L"EDIT", L"80", WS_CHILD | WS_VISIBLE | WS_BORDER, 
	size.cx, size.cy + 30, 120, size.cy, hwnd, NULL, hInst, NULL);
	

		

	HWND btnLogin = CreateWindowExW(0, L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE,
	0, size.cy + 60, 50,20, hwnd, (HMENU)ButtonId::Login, hInst, NULL);

	SIZE serverBtnTextSize;
	GetTextExtentPoint32W(GetDC(hwnd), L"Create server", 13, &serverBtnTextSize);
	HWND btnServer = CreateWindowExW(0, L"BUTTON", L"Create server", WS_CHILD | WS_VISIBLE,
	0, size.cy + 90, serverBtnTextSize.cx, 20, hwnd, (HMENU)ButtonId::CreateServer, hInst, NULL);
	
	out->teUsername = teUsername;
	out->teIpAddress = teIpAddress;	
	out->tePort = tePort;

	//Allow user to navigate with tab
	SetPropW(teUsername, L"NEXT", (HANDLE)teIpAddress);
	SetPropW(teIpAddress, L"NEXT", (HANDLE)tePort);
	SetPropW(tePort, L"NEXT", (HANDLE)btnLogin);
	SetPropW(btnLogin, L"NEXT", (HANDLE)btnServer);
	SetPropW(btnServer, L"NEXT", (HANDLE)teUsername);

	SetPropW(teUsername, L"PREV", (HANDLE)btnServer);
	SetPropW(teIpAddress, L"PREV", (HANDLE)teUsername);
	SetPropW(tePort, L"PREV", (HANDLE)teIpAddress);
	SetPropW(btnLogin, L"PREV", (HANDLE)tePort);
	SetPropW(btnServer, L"PREV", (HANDLE)btnLogin);

	SetPropW(teUsername, L"OLDPROC", (HANDLE)SetWindowLongW(teUsername, GWLP_WNDPROC, (LONG)TabProc));
	SetPropW(teIpAddress, L"OLDPROC", (HANDLE)SetWindowLongW(teIpAddress, GWLP_WNDPROC, (LONG)TabProc));
	SetPropW(tePort, L"OLDPROC", (HANDLE)SetWindowLongW(tePort, GWLP_WNDPROC, (LONG)TabProc));
	SetPropW(btnLogin, L"OLDPROC", (HANDLE)SetWindowLongW(btnLogin, GWLP_WNDPROC, (LONG)TabProc));
	SetPropW(btnServer, L"OLDPROC", (HANDLE)SetWindowLongW(btnServer, GWLP_WNDPROC, (LONG)TabProc));

	//so user can invoke button from the enter button
	SetPropW(btnLogin, L"BUTTONID", (HANDLE)MAKEWPARAM(ButtonId::Login, BN_CLICKED));
	SetPropW(btnServer, L"BUTTONID", (HANDLE)MAKEWPARAM(ButtonId::CreateServer, BN_CLICKED));
	SetPropW(btnLogin, L"LOGINWINDOW", (HANDLE)out);
	SetPropW(btnServer, L"LOGINWINDOW", (HANDLE)out);
	return true;
}

LoginWindow& LoginWindow::SetChatClient(ChatClient* chatClient){
	 this->chatClient = chatClient;
	 this->chatClient->loginWindow = this;
}

LoginWindow& LoginWindow::SetChatServer(ChatServer* chatServer){ 
	this->chatServer = chatServer;
	this->chatServer->loginWindow = this;
}

WPARAM LoginWindow::Run() {
	ShowWindow(hwnd, SW_SHOW);	
	UpdateWindow(hwnd);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
  {
		TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
	
	return msg.wParam;
}

const MainWindow* LoginWindow::GetMainWindow() const { return &mainWindow;}
const ServerWindow* LoginWindow::GetServerWindow() const { return &serverWindow;}
MainWindow* LoginWindow::GetMainWindow()  { return &mainWindow;}
ServerWindow* LoginWindow::GetServerWindow() { return &serverWindow;}

HWND LoginWindow::GetHwnd() const{ return hwnd;}
