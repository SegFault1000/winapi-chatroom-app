#include "ChatServer.h"
#include <iostream>
#include <thread>
#include "util.h"
#include "NetworkMessages.h"
#include "RichEditWrapper.h"

ChatServer::ChatServer(WSADATA* wsa)
{
	networkActionMap[NetworkMessage::CHAT_MESSAGE] = [this](SOCKET client, rapidjson::Document& doc)
	{			
		ClientInfo& info = clientInfoMap[client];
		
		NetworkMessage msg;
		msg.Add("type", NetworkMessage::CHAT_MESSAGE)
		.Add("content", doc["content"].GetString())
		.Add("username", info.username.size() == 0 ? "User" : info.username);
		SendJsonToAllClients(msg.ToJson());
	};

	networkActionMap[NetworkMessage::SET_USERNAME] = [this](SOCKET client, rapidjson::Document& doc)
	{		
		//pending implementation - will allow user to change username
	};
	networkActionMap[NetworkMessage::MEMBER_JOIN] = [this](SOCKET client, rapidjson::Document& doc)
	{		
		//1 - Store the user's desired name
		ClientInfo& clientInfo = clientInfoMap[client];
		clientInfo.flags |= ClientInfo::Flag::LoggedIn;
		clientInfo.username = doc["username"].GetString();	
		clientInfo.lastTimeSentPingToServer = Clock::now();
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
		
		for(size_t i = 0; i < clients.size(); ++i)
		{
			std::string clientUsername = clientInfoMap[clients[i]].username;
			if(clientUsername.size() == 0)
				clientUsername = "User";
			list.PushBack(rapidjson::Value(clientUsername.data(), allocator), allocator);
		}
		
		memberListDoc.AddMember(rapidjson::Value("members", allocator), list, allocator);
		SendJsonToClient(client, util::DocumentToJson(memberListDoc));
	};

	networkActionMap[NetworkMessage::PING] = [this](SOCKET client, rapidjson::Document& doc)
	{
		clientInfoMap[client].lastTimeSentPingToServer = Clock::now();
	};
	networkActionMap[NetworkMessage::MEMBER_LOGOUT] = [this](SOCKET client, rapidjson::Document& doc)
	{				
		if(auto it = clientInfoMap.find(client); it != clientInfoMap.end())
		{								
			it->second.flags &= ~ClientInfo::Flag::LoggedIn;
		}		
		else
		{
			std::wcout << L"[MEMBER_LOGOUT]: Could not find client information associated with " << client << L'\n';
		}		
	};
}

void ChatServer::SetDetails(std::wstring ip, u_short port)
{
	this->ip = ip;
	this->port = htons(port);
}

void ChatServer::Run() 
{	
	fd_set master;
	FD_ZERO(&master);

	FD_SET(listener, &master);
	while(true)
	{
		const auto now = Clock::now();
		fd_set copy = master;
		int socketCount = select(0, &copy, NULL, NULL, NULL);		
		
		for(int i = 0; i < socketCount; ++i)
		{
			SOCKET clientSock = copy.fd_array[i];			
			if(clientSock == listener)
			{
				SOCKET newClient = accept(listener, NULL, NULL);
				clients.push_back(newClient);
				FD_SET(newClient, &master);											
			}
			else
			{
				using IT = decltype(clientInfoMap.begin());
				const auto RemoveClient = [this, &master](SOCKET client, IT& mapIterator)
				{
					FD_CLR(client, &master);		
					auto it = std::find(clients.begin(), clients.end(), client);
					if(it != clients.end())
					{
						clients.erase(it);
					}
					mapIterator = clientInfoMap.erase(mapIterator);					
				};
				#pragma region check if client is no longer responding
				auto it = clientInfoMap.find(clientSock);
				if(it != clientInfoMap.end())
				{
					ClientInfo& info = it->second;					
					auto diff = std::chrono::duration_cast<std::chrono::seconds>(info.lastTimeSentPingToServer - now).count();

					if(diff >= 10 && disconnectIfNoPings)
					{
						const std::string username = std::move(info.username);
						RemoveClient(clientSock, it);
						NetworkMessage msg;
						msg.Add("type", NetworkMessage::MEMBER_DISCONNECT);
						msg.Add("username", username);									
						SendJsonToAllClients(msg.ToJson());												
						continue;
					}	
					else if(!(info.flags & ClientInfo::Flag::LoggedIn))				
					{						
						const std::string username = std::move(info.username);
						RemoveClient(clientSock, it);						
						std::wstring logoutMsg = fmt::format
						(
							L"\"{}\" (Socket ID #{}) has logged out.\n",
							util::mbstowcs(username), 
							clientSock
						);		
																												
						reServerLog->AppendText(RGB(0,0,0), RichEdit::None, logoutMsg.data());
						std::string json = NetworkMessage()
							.Add("type", NetworkMessage::MEMBER_LOGOUT)
							.Add("username", username)
							.ToJson();						
						SendJsonToAllClients(json);						
						continue;
					}
				}
				#pragma endregion
				char buffer[10'000];
				NetworkMessage msg;
				if(msg.ReceiveJson(clientSock, buffer, sizeof(buffer)) == NetworkMessage::ErrorFlag::Success)
				{
					OnJsonReceived(clientSock, msg.GetDocument());
				}												
			}//end of else
		}//end of for(int i = 0; i < socketCount; ++i){
	}//end of while(true){}
}//end of void ChatServer::Run() {	

bool ChatServer::Listen() 
{

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

void ChatServer::OnJsonReceived(SOCKET sock, char* json, int32_t jsonSize) 
{	
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
void ChatServer::OnJsonReceived(SOCKET sock, rapidjson::Document& doc) 
{		
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

void ChatServer::SendJsonToClient(SOCKET client, const std::string& json) 
{
	const int32_t jsonSize = json.size();
	const int32_t jsonSizeNetworkOrder = htonl(jsonSize);
	int32_t sent_size = 0;
	while(sent_size < sizeof(jsonSize))
	{
		int32_t bytesLeft = sizeof(jsonSize) - sent_size;
		int32_t bytesReceived = send(client, (const char*)&jsonSizeNetworkOrder + sent_size, bytesLeft, 0 );
		if(bytesReceived == SOCKET_ERROR)
		{
			std::wcout << L"Failed to send json size to " << client << L'\n';
			return;
		}
		sent_size += bytesReceived;			
	}
	
	sent_size = 0;
	while(sent_size < jsonSize)
	{
		int32_t bytesLeft = jsonSize - sent_size;
		int32_t bytesReceived = send(client, json.data() + sent_size, bytesLeft, 0);
		if(bytesReceived == SOCKET_ERROR)
		{
			std::wcout << L"Failed to send json to " << client << L'\n';
			return;
		}
		sent_size += bytesReceived;			
	}
}
void ChatServer::SendJsonToAllClients(const std::string& json, const std::unordered_set<SOCKET>& exceptions) 
{
	SendJsonToAllClients(json, &exceptions);
}

void ChatServer::SendJsonToAllClients(const std::string& json, const std::unordered_set<SOCKET>* const exceptions) 
{		
	for(size_t i = 0; i < clients.size(); ++i)
	{
		if(exceptions)
		{
			auto it = exceptions->find(clients[i]);
			if(it != exceptions->end())
				continue;
		}
		SendJsonToClient(clients[i], json);
	}	
}

void ChatServer::Log(std::string content) 
{
	std::scoped_lock<std::mutex> lck{logMutex};	
	std::wstring contentW = util::mbstowcs(content);
	reServerLog->AppendText(RGB(0,0,0), RichEdit::None, contentW.data());		
	reServerLog->ScrollToBottom();	
}
void ChatServer::Log(std::wstring content) 
{
	std::scoped_lock<std::mutex> lck{logMutex};	
	reServerLog->AppendText(RGB(0,0,0), RichEdit::None, content.data());
	reServerLog->ScrollToBottom();
}
