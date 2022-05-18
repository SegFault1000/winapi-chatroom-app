#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <future>
#include <string>
#include <windef.h>
#include "RichEditWrapper.h"
#include <rapidjson/encodings.h>
#include <rapidjson/document.h>
class MainWindow
{
	MSG msg;
	
	uint32_t width = 600, height = 400;
	int posX = 0, posY = 0;
	std::wstring title = L"title";
	HWND hwnd = NULL;
	HWND textEdit = NULL;
	HWND btnSubmit = NULL;
	HWND lbMembers = NULL;
	
	inline static bool windowClassRegistered = false;

	RichEdit reUserInput;
	RichEdit reChatBox;
	class ChatClient* chatClient = NULL;
protected:
	HINSTANCE hInstance, hPrevInstance;
	LPSTR pCmdLine;
	int nCmdShow;
				
	virtual MainWindow& OnCreate(HWND);

public:
	std::unordered_map<WPARAM, std::vector<std::pair<uint32_t, std::function<void()>>>> onKeyDownMap;
	std::unordered_map<WPARAM, std::vector<std::pair<uint32_t, std::function<void()>>>> onKeyUpMap;
	std::unordered_map<HMENU, std::function<void()>> OnButtonPressed;
	
	//MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow);
	static bool Create(HINSTANCE hInst, int x, int y, int width, int height, MainWindow* out);
	//1 - Getters:	
	HWND GetHwnd() const{ return hwnd;}
	HINSTANCE GetHInstance() const{ return hInstance;}
	HINSTANCE GetHPrevInstance() const{ return hPrevInstance;}
	LPCSTR GetPCmdLine() const{ return pCmdLine;}
	int GetCmdShow() const{ return nCmdShow;}
	//-----
	//2 - Setters:	
	MainWindow& SetDimensions(uint32_t width, uint32_t height);
	MainWindow& SetPosition(int x, int y);	
	MainWindow& SetTitle(const std::wstring& title);		
	void SetChatClient(ChatClient* chatClient);
	
	static void RegisterWindowClass(HINSTANCE hInstance);

	//thread-safe
	std::mutex chatBoxMutex;
	void AppendUserMessage(std::wstring username, std::wstring message);
	void AppendChatBox(COLORREF color, RichEdit::FontStyle style, std::wstring message);
	
	
	std::mutex lbMembersMutex;
	bool SetMemberListFromJsonArray(rapidjson::Value::Array& val);
	bool AppendToMemberList(const char* memberName, int memberNameLen);
	
	WPARAM Run();	
	//3 - Event stuff:			
	friend LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	friend LRESULT CALLBACK TextEditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	//-----
	void Resize(uint32_t width, uint32_t height);
	operator bool() const{ return hwnd != NULL; }
	bool IsNull() const { return hwnd == NULL; }			
	enum ButtonId : uint32_t
	{
		None, Submit
	};


};


