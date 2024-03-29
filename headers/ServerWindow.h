#pragma once
#include <windef.h>
#include "RichEditWrapper.h"
#include <string>
#include <mutex>
class ChatServer;
class ServerWindow
{
	inline static bool windowClassRegistered = false;
	HWND hwnd = NULL;
	HWND label = NULL;
	RichEdit re;
	RichEdit reServerLog;
	class ChatServer* chatServer = NULL;
public:
	static void RegisterWindowClass(HINSTANCE hInstance);
	static bool Create(HINSTANCE hInstance, int x, int y, int width, int height, ServerWindow* out);	
	bool Create(HINSTANCE hInstance, int x, int y, int width, int height);	
	friend LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void SetChatServer(ChatServer* chatServer);

	bool IsNull() const{ return hwnd == NULL;}
	operator bool() const{ return hwnd != NULL;}
	std::mutex logMutex;
	HWND GetHwnd() const{ return hwnd;}
	void Hide(){ ShowWindow(hwnd, SW_HIDE);}
	void Show(){ ShowWindow(hwnd, SW_SHOW);}
	RichEdit* GetReServerLog(){ return &reServerLog;}	
};