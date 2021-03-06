#include "ChatServer.h"
#include <thread>
#include "util.h"
#include "NetworkMessages.h"

ChatServer::ChatServer(WSADATA* wsa){
	networkActionMap[NetworkMessage::CHAT_MESSAGE] = [this](SOCKET client, rapidjson::Document& doc)
	{	
		using util::Json_AddMember;	
		ClientInfo& info = clientInfoMap[client];
		NetworkMessage msg;
		msg.Add("type", NetworkMessage::CHAT_MESSAGE)
		.Add("content", doc["content"].GetString())
		.Add("username", info.username.size() == 0 ? "User" : info.username);
		SendJsonToAllClients(msg.ToJson());
	};

	networkActionMap[NetworkMessage::SET_USERNAME] = [this](SOCKET client, rapidjson::Document& doc)
	{		
		
	};
	networkActionMap[NetworkMessage::MEMBER_JOIN] = [this](SOCKET client, rapidjson::Document& doc)
	{		
		//1 - Store the user's desired name
		clientInfoMap[client].username = doc["username"].GetString();		
		//2 - Log to server window:
		std::wstring usernameW = util::mbstowcs(doc["username"].GetString(), doc["username"].GetStringLength());
		Log(fmt::format(L"New client: {}. Socket ID#{}.\n", usernameW, client));				
		//Notify everyone of the entrant
		SendJsonToAllClients(util::DocumentToJson(doc));

		//3 - Add the user to everyone else's member list		
		NetworkMessage msg;
		std::string memberAddJson = msg
			.Add("type", NetworkMessage::MEMBER_LIST_ADD)
			.Add("username", doc["username"].GetString())
			.ToJson();
		std::unordered_set<SOCKET> exceptions{client};
		SendJsonToAllClients(memberAddJson, &exceptions);

		//4 - Send entire list of online members to the entrant:
		rapidjson::Document memberListDoc{rapidjson::kObjectType};
		auto allocator = memberListDoc.GetAllocator();
		util::Json_AddMember(memberListDoc, "type", NetworkMessage::MEMBER_LIST);
		rapidjson::Value list(rapidjson::kArrayType);
		clientsVectorMutex.lock();
		for(size_t i = 0; i < clients.size(); ++i)
		{
			std::string clientUsername = clientInfoMap[clients[i]].username;
			if(clientUsername.size() == 0)
				clientUsername = "User";
			list.PushBack(rapidjson::Value(clientUsername.data(), allocator), allocator);
		}
		clientsVectorMutex.unlock();
		memberListDoc.AddMember(rapidjson::Value("members", allocator), list, allocator);
		SendJsonToClient(client, util::DocumentToJson(memberListDoc));
	};
}

void ChatServer::SetDetails(std::wstring ip, u_short port){
	this->ip = ip;
	this->port = htons(port);
}

void ChatServer::Run() {	
	fd_set master;
	FD_ZERO(&master);

	FD_SET(listener, &master);
	while(true)
	{
		fd_set copy = master;
		int socketCount = select(0, &copy, NULL, NULL, NULL);
		for(int i = 0; i < socketCount; ++i)
		{
			SOCKET clientSock = copy.fd_array[i];
			if(clientSock == listener)
			{
				SOCKET newClient = accept(listener, NULL, NULL);				
				FD_SET(newClient, &master);
				clientsVectorMutex.lock();
				clients.push_back(newClient);
				clientsVectorMutex.unlock();
			}
			else
			{
				char buffer[4000]{};
				int32_t jsonSize = -1;
				int32_t recv_size = recv(clientSock, (char*)&jsonSize, sizeof(jsonSize), 0);
				
				while(recv_size < sizeof(jsonSize))
				{
					int32_t bytesLeft = sizeof(jsonSize) - recv_size;
					int32_t bytesReceived = recv(clientSock, (char*)&jsonSize + recv_size, bytesLeft, 0);
					if(bytesReceived < 0)
					{
						break;
					}					
					recv_size += bytesReceived;
				}
				
				recv_size = recv(clientSock, buffer, jsonSize, 0);
				if(recv_size == SOCKET_ERROR)
				{
					continue;
				}
				
				while(recv_size < jsonSize)
				{
					int32_t bytesLeft = jsonSize - recv_size;
					int32_t bytesReceived = recv(clientSock, (char*)&jsonSize + recv_size, bytesLeft, 0);
					if(bytesReceived < 0)
					{
						break;
					}													
					recv_size += bytesReceived;						
				}
											
				OnJsonReceived(clientSock, buffer, jsonSize);
			}
		}
	}
}

bool ChatServer::Listen() {
	listener = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = port;
	hint.sin_addr.S_un.S_addr = inet_addr(util::wcstombs(ip.data(), ip.size()).data());
	
	if(bind(listener, (sockaddr*)&hint, sizeof(hint)) != 0)
	{
		int errorCode = WSAGetLastError();
		if(errorCode == 10049)
		{
			util::ShowMessageBoxFmt(L"Error", L"Invalid address.\nError code: 10049");
		}
		else
		{
			util::ShowMessageBoxFmt(L"Error", L"Could not bind. Last WSA error: {}",errorCode );
			std::exit(0);
		}
		return false;
	}
	if(listen(listener, SOMAXCONN) != 0)
	{
		util::ShowMessageBoxFmt(L"Error", L"Failed to listen. Last WSA error: ", WSAGetLastError());
		std::exit(0);
	}

	if(IsListening())
		return false;
	std::thread{[this]{
		Run();
	}}.detach();
	isListening = true;
	return true;
}

bool ChatServer::IsListening(){ return isListening; }

void ChatServer::OnJsonReceived(SOCKET sock, char* json, int32_t jsonSize) {	
	rapidjson::Document doc;
	doc.Parse<rapidjson::kParseStopWhenDoneFlag>(json);
	if(!doc.IsObject()) return;
	try
	{
		std::string type = doc["type"].GetString();

		auto it = networkActionMap.find(type);
		if(it != networkActionMap.end())
		{
			it->second(sock, doc);
		}
	}
	catch(const std::exception& e)
	{
		Log(std::string("There was a json error:\n") + e.what());
	}	
}

void ChatServer::SendJsonToClient(SOCKET client, std::string json) {
	int32_t jsonSize = json.size();
	int32_t sent_size = send(client, (const char*)&jsonSize, sizeof(jsonSize), 0);
	while(sent_size < sizeof(jsonSize))
	{
		int32_t bytesLeft = sizeof(jsonSize) - sent_size;
		int32_t bytesReceived = send(client, (const char*)&jsonSize + sent_size, bytesLeft, 0 );
		if(bytesReceived <= 0)
		{
			continue;
		}
		sent_size += bytesReceived;			
	}

	sent_size = send(client, json.data(), jsonSize, 0);
	while(sent_size < jsonSize)
	{
		int32_t bytesLeft = jsonSize - sent_size;
		int32_t bytesReceived = send(client, json.data() + sent_size, bytesLeft, 0);
		if(bytesReceived <= 0)
		{
			continue;
		}
		sent_size += bytesReceived;			
	}
}

void ChatServer::SendJsonToAllClients(std::string json, std::unordered_set<SOCKET>* exceptions) {
	clientsVectorMutex.lock();
	const int32_t jsonSize = json.size();
	for(size_t i = 0; i < clients.size(); ++i)
	{
		if(exceptions != NULL && exceptions->find(clients[i]) != exceptions->end())
		{
			continue;
		}
		int32_t sent_size = send(clients[i], (const char*)&jsonSize, sizeof(jsonSize), 0);
		while(sent_size < sizeof(jsonSize))
		{
			int32_t bytesLeft = sizeof(jsonSize) - sent_size;
			int32_t bytesReceived = send(clients[i], (const char*)&jsonSize + sent_size, bytesLeft, 0 );
			if(bytesReceived <= 0)
			{
				continue;
			}
			sent_size += bytesReceived;			
		}

		sent_size = send(clients[i], json.data(), jsonSize, 0);
		while(sent_size < jsonSize)
		{
			int32_t bytesLeft = jsonSize - sent_size;
			int32_t bytesReceived = send(clients[i], json.data() + sent_size, bytesLeft, 0);
			if(bytesReceived <= 0)
			{
				continue;
			}
			sent_size += bytesReceived;			
		}
	}
	clientsVectorMutex.unlock();
}

#include <RichEditWrapper.h>

void ChatServer::Log(std::string content) {
	std::scoped_lock<std::mutex> lck{logMutex};
	int length_required = MultiByteToWideChar(CP_UTF8, NULL, content.data(), -1, NULL, 0);
	WCHAR* buffer = new WCHAR[length_required + 1];	
	MultiByteToWideChar(CP_UTF8, NULL, content.data(), -1, buffer, length_required);
	reServerLog->AppendText(RGB(0,0,0), RichEdit::None, buffer);	
	
	reServerLog->ScrollToBottom();
}
void ChatServer::Log(std::wstring content) {
	std::scoped_lock<std::mutex> lck{logMutex};	
	reServerLog->AppendText(RGB(0,0,0), RichEdit::None, content.data());
	reServerLog->ScrollToBottom();
}

/*
		rapidjson::Document responseDoc(rapidjson::kObjectType);
		Json_AddMember(responseDoc, "type", NetworkMessage::CHAT_MESSAGE);
		Json_AddMember(responseDoc, "content", doc["content"].GetString());
		Json_AddMember(responseDoc, "username", info.username.size() == 0 ? "User" : info.username);
		SendJsonToAllClients(util::DocumentToJson(responseDoc));			
		*/
