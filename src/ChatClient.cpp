#include "ChatClient.h"
#include <fmt/xchar.h>
#include <thread>
#include <rapidjson/document.h>
#include <iostream>
#include "NetworkMessages.h"
#include "util.h"
#include "RichEditWrapper.h"
#include "MainWindow.h"
#include "LoginWindow.h"

bool ChatClient::Connect(const WCHAR* ip, u_short port) {
	if(IsConnected())
	{
		MessageBoxW(0, L"Tried to connect twice.\nFailed.", L"Warning", 0);
		return false;
	}
	
	if(wcslen(ip) == 0)
	{
		return false;
	}
	sock = socket(AF_INET , SOCK_STREAM , 0 );
	if(sock == INVALID_SOCKET)
	{
		MessageBoxW(0, 
			fmt::format(L"Failed to create socket. Error code: {}", WSAGetLastError()).data(),
			L"Error", 0);		
		return false;
	}
	size_t requiredSize = WideCharToMultiByte(CP_UTF8, 0, ip, -1, NULL, 0, NULL, NULL);
	char* buffer = new char[requiredSize + 1];
	WideCharToMultiByte(CP_UTF8, 0, ip, -1, buffer, requiredSize, NULL, NULL);
	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.S_un.S_addr = inet_addr(buffer);
	server.sin_port = htons(port);
	delete[] buffer;

	if(connect(sock, (sockaddr*)&server, sizeof(server)) != 0)		
	{
		int errorCode = WSAGetLastError();
		if(errorCode == 10061)
			util::ShowMessageBoxFmt(L"Error", L"Connection refused. Error code 10061."
			L"Double-check your details:\nIP: {}\nPort: {}\n", ip, port);
		else
			MessageBoxW(0, fmt::format(L"Failed to connect.\nLast WSA Error: {}", errorCode).data(), 
		L"Connection error", 0);
		return false;
	}
	isConnected = true;
	std::thread{&ChatClient::Run, this}.detach();	
	return true;
}

bool ChatClient::IsConnected() const
{
	return isConnected;
}
void ChatClient::SendChatMessage(WCHAR* str, int len) {	
	int length_required =	WideCharToMultiByte(CP_UTF8, 0, str, len, NULL,0,NULL, NULL);
	std::string message(length_required, ' ');
	WideCharToMultiByte(CP_UTF8, 0, str, len, message.data(), length_required, NULL, NULL);

	const char* jsonFormat = "{{\n\"type\" : \"CHAT_MESSAGE\",\n"
	"\"username\" : \"{}\",\n"
	"\"content\" : \"{}\"\n}}";
		
	SendJsonToServer(fmt::format(jsonFormat, "User", message));

	return;	
}
void ChatClient::Logout() 
{
	NetworkMessage msg;
	msg.Add("type", NetworkMessage::MEMBER_LOGOUT);
	SendJsonToServer(msg.ToJson());	
}
void ChatClient::SetUsername(WCHAR* str, int len){	
	int length_required =	WideCharToMultiByte(CP_UTF8, 0, str, len, NULL,0,NULL, NULL);
	std::string message(length_required, ' ');
	WideCharToMultiByte(CP_UTF8, 0, str, len, message.data(), length_required, NULL, NULL);

	
	rapidjson::Document doc(rapidjson::kObjectType);
	util::Json_AddMember(doc, "type", NetworkMessage::SET_USERNAME);	
	util::Json_AddMember(doc, "username", message);				
	SendJsonToServer(util::DocumentToJson(doc));
}
void ChatClient::SendJsonToServer(std::string json) {
	#define ERROR_QUIT_IF(condition, message_to_send) if(condition){ MessageBoxW(0, message_to_send, L"Error", 0); return;}
	std::scoped_lock<std::mutex> lck{sendMutex};
	int32_t jsonSize = json.size();		
	
	int32_t sent_size = send(sock, (char*)&jsonSize, sizeof(jsonSize), 0);
	ERROR_QUIT_IF(sent_size == SOCKET_ERROR, L"Failed to send chat message");
	while(sent_size < sizeof(jsonSize))
	{
		int32_t bytesLeft = sizeof(jsonSize) - sent_size;
		int32_t bytesSent = send(sock, (char*)&jsonSize + sent_size, bytesLeft, 0);
		ERROR_QUIT_IF(bytesSent == SOCKET_ERROR, L"Failed to send json");
		sent_size += bytesSent;	
	}

	sent_size = send(sock, json.data(), jsonSize, 0);
	while(sent_size < jsonSize)
	{
		int32_t bytesLeft = jsonSize - sent_size;
		int32_t bytesSent = send(sock, (char*)&jsonSize + sent_size, bytesLeft, 0);
		ERROR_QUIT_IF(bytesSent == SOCKET_ERROR, L"Failed to send json");
		sent_size += bytesSent;	
	}		
	#undef ERROR_QUIT_IF
}
void ChatClient::SetChatMessageCallback(std::function<void(std::wstring, std::wstring)> func) {
	chatMessageCallback = func;
}

void ChatClient::Run() {
	char buffer [6000];
	std::atomic_bool running = true;
	std::thread pinging([this, &running]
	{
		NetworkMessage pingMsg;
		pingMsg.Add("type", NetworkMessage::PING);
		const std::string pingJson = pingMsg.ToJson();
		while(running)
		{
			std::this_thread::sleep_for(std::chrono::seconds(2));
			SendJsonToServer(pingJson);
		}
	});

	
	while(running)
	{		
		int32_t jsonSize = -1;
		int32_t recv_size = recv(sock, (char*)&jsonSize, sizeof(jsonSize), 0);
		
		if(recv_size == SOCKET_ERROR)
		{
			MessageBoxW(0, L"SOCKET_ERROR in ChatClient::Run()", L"Error", 0);			
			std::exit(0);
		}
		while(recv_size < sizeof(jsonSize))
		{	
			int32_t bytesLeft = sizeof(jsonSize) - recv_size;
			int32_t receivedBytes = recv(sock, (char*)&jsonSize + recv_size, bytesLeft, 0);
			if(receivedBytes == SOCKET_ERROR)
			{
				MessageBoxW(0, L"There was an error communicating with the server", L"Error", 0);				
				return;
			}
			recv_size += receivedBytes;
		}						

		recv_size = 0;		
		while(recv_size < jsonSize)
		{
			int32_t bytesLeft = jsonSize - recv_size;
			int32_t receivedBytes = recv(sock, buffer + recv_size, bytesLeft, 0);
			if(receivedBytes == SOCKET_ERROR)
			{
				MessageBoxW(0, L"There was an error communicating with the server", L"Error", 0);				
				return;
			}
			recv_size += receivedBytes;
		}
		rapidjson::Document doc;
		doc.Parse<rapidjson::kParseStopWhenDoneFlag>(buffer);
		if(doc.IsObject())
		{			
			try
			{
				std::string type = doc["type"].GetString();
				auto it = networkActionMap.find(type);
				if(it != networkActionMap.end())
				{
					it->second(doc);
				}
		
			}
			catch(const std::exception& e)
			{				
				//MessageBoxW(0, L"Failed to parse json.", L"Error", 0);
			}			
		}
		else
		{
			//MessageBoxW(0, L"Not a valid json object...", L"Error", 0);
		}
	} //end of loop
}
ChatClient::ChatClient(WSADATA* wsa) : wsa(wsa){
	networkActionMap[NetworkMessage::CHAT_MESSAGE] = [this](rapidjson::Document& doc)
	{
		if(!loginWindow)
			return;
		std::string username = doc["username"].GetString();
		std::string content = doc["content"].GetString();		
		loginWindow->GetMainWindow()->AppendUserMessage(util::mbstowcs(username), util::mbstowcs(content));
	};
	networkActionMap[NetworkMessage::MEMBER_JOIN] = [this](rapidjson::Document& doc)
	{
		if(!loginWindow)
			return;
		std::wstring username = util::mbstowcs(doc["username"].GetString());
		std::wstring result = fmt::format(L"[{}] has joined the chat.\n", username);
		loginWindow->GetMainWindow()->AppendChatBox(RGB(56,56,56), RichEdit::Bold, result);
	};
	networkActionMap[NetworkMessage::MEMBER_LIST] = [this](rapidjson::Document& doc)
	{
		if(!loginWindow || !doc["members"].IsArray())
			return;		
		MainWindow* mainWindow = loginWindow->GetMainWindow();
		rapidjson::Value::Array arr = doc["members"].GetArray();
		mainWindow->SetMemberListFromJsonArray(arr);
	};

	networkActionMap[NetworkMessage::MEMBER_LIST_ADD] = [this](rapidjson::Document& doc)
	{
		if(!loginWindow)
			return;		
		MainWindow* mainWindow = loginWindow->GetMainWindow();
		std::string username = doc["username"].GetString();
		mainWindow->AppendToMemberList(doc["username"].GetString(), doc["username"].GetStringLength());
	};
	networkActionMap[NetworkMessage::MEMBER_LOGOUT] = [this](rapidjson::Document& doc)
	{
		if(!loginWindow)
			return;
		MainWindow* mainWindow = loginWindow->GetMainWindow();
		if(!mainWindow)
			return;		
		const auto& rjValue = doc["username"];		
		mainWindow->RemoveFromMemberList(rjValue.GetString(), rjValue.GetStringLength());
		const std::wstring logoutChatMessage = fmt::format(L"[{}] has logged out.\n", 
			util::mbstowcs(rjValue.GetString(), rjValue.GetStringLength()));
		mainWindow->AppendChatBox(RGB(255,0,0), RichEdit::Bold, logoutChatMessage);
	};
	networkActionMap[NetworkMessage::MEMBER_DISCONNECT] = [this](rapidjson::Document& doc)
	{
		if(!loginWindow)
			return;
		MainWindow* mainWindow = loginWindow->GetMainWindow();
		std::wstring usernameW = util::mbsvtowcs(doc["username"].GetString());		
		mainWindow->RemoveFromMemberList(usernameW.data());
		std::wstring chatMsg = fmt::format(L"[{}] has disconnected from the chat.", usernameW);
		mainWindow->AppendChatBox(RGB(255,0,0), RichEdit::Bold, chatMsg);
	};
}

		