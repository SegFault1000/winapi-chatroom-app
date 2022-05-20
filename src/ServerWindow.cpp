#include "ServerWindow.h"
#include "ChatServer.h"
#include <windows.h>
#include "RichEditWrapper.h"

LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


bool ServerWindow::Create(HINSTANCE hInstance, int x, int y, int width, int height, ServerWindow* out) 
{
	if(!windowClassRegistered)
	{
		MessageBoxW(0, L"You need to invoke ServerWindow::RegisterWindowClass before using ServerWindow::Create", L"Error", 0);
		std::exit(0);
	}
	HWND hwnd = CreateWindowExW(0, L"SERVERWINDOW", L"Server", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
	300,300, 600, 400, NULL, NULL, hInstance, NULL);
	if(!hwnd)
		return false;	
	if(!RichEdit::Create(hwnd, hInstance,0,0,600,400, true, &out->reServerLog))
	{
		MessageBoxW(0, L"Failed to create richedit for ServerWindow", L"Error", 0);
		std::exit(0);
	}
	out->hwnd = hwnd;
	SetPropW(hwnd, L"WINDOW", (HANDLE)out);		
	
	return true;
}

void ServerWindow::RegisterWindowClass(HINSTANCE hInstance) {
	if(windowClassRegistered)
		return;
	windowClassRegistered = true;
	WNDCLASSW wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszClassName = L"SERVERWINDOW";
	wc.hInstance = hInstance;
	wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc.lpszMenuName = NULL;
	wc.lpfnWndProc = ServerWindowProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassW(&wc);
}
LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg)
	{
	case WM_QUIT:
		PostQuitMessage(0);
	break;
	}
	return DefWindowProcW(hwnd,msg,wParam, lParam);
}

void ServerWindow::SetChatServer(ChatServer* chatServer)
{ 
	this->chatServer = chatServer;
}


