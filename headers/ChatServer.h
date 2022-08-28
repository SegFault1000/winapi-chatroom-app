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
	using Clock = std::chrono::steady_clock;
	struct ClientInfo
	{
		std::string username;
		COLORREF nameColor;				
		Clock::time_point lastTimeSentPingToServer;
		enum Flag : uint64_t
		{
			None							= 0 << 0, 
			LoggedIn 					= 1 << 0,
			CanDeleteMessages = 1 << 1,
			Muted 						= 1 << 2
		};
		std::underlying_type_t<Flag> flags = Flag::LoggedIn;
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
	void SendJsonToClient(SOCKET client, const std::string& json);
	void SendJsonToAllClients(const std::string& json, const std::unordered_set<SOCKET>* const exceptions = NULL);
	void SendJsonToAllClients(const std::string& json, const std::unordered_set<SOCKET>& exceptions);


	std::mutex logMutex;
	void Log(std::string);
	void Log(std::wstring);
};