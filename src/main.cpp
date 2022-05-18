#include <winsock2.h>
#include <windows.h>
#include <unordered_map>
#include <functional>
#include <iostream>
#include "LoginWindow.h"
#include "ChatClient.h"
#include "ChatServer.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{	
	if(!LoadLibrary(TEXT("Riched32.dll")))
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
	if(!LoginWindow::Create(hInstance, 300,300, 600, 400, &loginWindow))
	{
		MessageBoxW(0, L"Failed to create MainWindow", L"Error", 0);
		return 1;
	}	
	loginWindow.SetChatClient(&chatClient);
	loginWindow.SetChatServer(&chatServer);
	WPARAM returnValue = loginWindow.Run();

	WSACleanup();
	return returnValue;
}



