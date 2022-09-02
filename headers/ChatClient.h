#pragma once
#include <winsock2.h>
#include <thread>
#include <functional>
#include <string>
#include <functional>
#include <rapidjson/document.h>
#include <mutex>
#include <atomic>

class NetworkMessage;
class ChatClient
{
	WSADATA* wsa = NULL;
	SOCKET sock = 0;
	bool isConnected = false;
	std::thread receptionThread;	
	std::thread pingThread;
	void Run();
	std::unordered_map<std::string, std::function<void(rapidjson::Document&)>> networkActionMap;
	std::mutex sendMutex;
	std::atomic_bool running = false;	
public:
	class LoginWindow* loginWindow = NULL;
	ChatClient(WSADATA* wsa);	
	bool Connect(const WCHAR* ip, u_short port);
	bool IsConnected() const;	
	void SendChatMessage(WCHAR* str, int len);
	void SetUsername(WCHAR* str, int len);
	void Logout();
	
	#pragma thread-safe 	
	bool SendNetworkMessageToServer(const NetworkMessage& networkMessage);
	#pragma endregion
		
};