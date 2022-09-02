#include <winsock2.h>
#include <windows.h>
#include <unordered_map>
#include <functional>
#include <iostream>
#include "LoginWindow.h"
#include "ChatClient.h"
#include "ChatServer.h"
#include "util.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{			
	HMODULE riched32_dll = LoadLibraryW(L"Riched32.dll");
	if(!riched32_dll)
	{
		MessageBoxW(0, L"Failed to load Riched32.dll", L"Error", 0);
		return 1;
	}
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
	{
		MessageBoxW(0, L"Failed to initialize WSA.", L"Error", 0);
		return 1;
	}
			
	ChatClient chatClient{&wsa};
	ChatServer chatServer{&wsa};
	LoginWindow::RegisterWindowClass(hInstance);
	MainWindow::RegisterWindowClass(hInstance);
	ServerWindow::RegisterWindowClass(hInstance);
	LoginWindow loginWindow; 	

	loginWindow.SetChatClient(&chatClient);
	loginWindow.SetChatServer(&chatServer);
	if(!loginWindow.Create(hInstance, 300,300, 600, 400))
	{
		MessageBoxW(0, L"Failed to create MainWindow", L"Error", 0);
		return 1;
	}			
	
	WPARAM returnValue = loginWindow.Run();
	
	FreeLibrary(riched32_dll);
	WSACleanup();	
	return returnValue;
}


