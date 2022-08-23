#pragma once
#include <winsock2.h>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <rapidjson/document.h>
#include <functional>
#include <unordered_set>
class ChatServer
{
	WSADATA* wsa;
	SOCKET listener;
	std::vector<SOCKET> clients;
	std::mutex clientsVectorMutex;
	void Run();	
	bool isListening = false;
	std::wstring ip = L"127.0.0.1";
	u_short port = htons(80);
	struct ClientInfo
	{
		std::string username;
		COLORREF nameColor;				
		std::chrono::steady_clock::time_point lastTimeSentPingToServer;
	};
	std::unordered_map<std::string, std::function<void(SOCKET, rapidjson::Document&)>> networkActionMap;
	std::unordered_map<SOCKET, ClientInfo> clientInfoMap;
public:	
	class RichEdit* reServerLog = NULL;
	class LoginWindow* loginWindow = NULL;
	bool Listen();
	bool IsListening();
	ChatServer(WSADATA* wsa);	
	void SetDetails(std::wstring ip, u_short port);
	void OnJsonReceived(SOCKET sock, char* json, int32_t jsonSize);
	void OnJsonReceived(SOCKET sock, rapidjson::Document& doc);
	void SendJsonToClient(SOCKET client, std::string json);
	void SendJsonToAllClients(std::string json, std::unordered_set<SOCKET>* exceptions = NULL);

	std::mutex logMutex;
	void Log(std::string);
	void Log(std::wstring);
};