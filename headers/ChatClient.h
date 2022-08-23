#pragma once
#include <winsock2.h>
#include <thread>
#include <functional>
#include <string>
#include <functional>
#include <rapidjson/document.h>
#include <mutex>

class ChatClient
{
	WSADATA* wsa = NULL;
	SOCKET sock = NULL;
	bool isConnected = false;
	std::thread receptionThread;	
	void Run();
	std::unordered_map<std::string, std::function<void(rapidjson::Document&)>> networkActionMap;
	std::mutex sendMutex;
public:
	class LoginWindow* loginWindow = NULL;
	ChatClient(WSADATA* wsa);	
	bool Connect(const WCHAR* ip, u_short port);
	bool IsConnected() const;	
	void SendChatMessage(WCHAR* str, int len);
	void SetUsername(WCHAR* str, int len);
	void Logout();
	void SendJsonToServer(std::string json);
	
	std::function<void(std::wstring, std::wstring)> chatMessageCallback;
	void SetChatMessageCallback(std::function<void(std::wstring, std::wstring)> func);	
};
