#pragma once

#include "MainWindow.h"
#include "ServerWindow.h"
#include <windef.h>
class LoginWindow
{
	class ChatClient* chatClient = nullptr;
	class ChatServer* chatServer = nullptr;
	HWND hwnd = NULL;
	HWND teUsername = NULL;
	HWND teIpAddress = NULL;
	HWND tePort = NULL;
	inline static bool windowClassRegistered = false;
	MainWindow mainWindow;
	ServerWindow serverWindow;
public:	
	enum ButtonId{
		None, Login, CreateServer
	};
	static void RegisterWindowClass(HINSTANCE hInstance);
	static bool Create(HINSTANCE hInst, int x, int y, int width, int height, LoginWindow* out);	
	LoginWindow& SetChatClient(ChatClient* chatClient);
	LoginWindow& SetChatServer(ChatServer* chatServer);
	friend LRESULT CALLBACK LoginWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	WPARAM Run();
	const MainWindow* GetMainWindow() const;
	const ServerWindow* GetServerWindow() const;
	MainWindow* GetMainWindow();
	ServerWindow* GetServerWindow();
	HWND GetHwnd() const;
};