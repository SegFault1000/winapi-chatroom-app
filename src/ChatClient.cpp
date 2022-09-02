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

bool ChatClient::Connect(const WCHAR* ip, u_short port) 
{
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
	sockaddr_in server;
	server.sin_family = AF_INET;
	std::string buffer = util::wcstombs(ip);
	server.sin_addr.S_un.S_addr = inet_addr(buffer.data());
	server.sin_port = htons(port);	

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
void ChatClient::SendChatMessage(WCHAR* str, int len) 
{	
	NetworkMessage msg;
	msg.AddStringRef("type", NetworkMessage::CHAT_MESSAGE);
	msg.AddStringRef("username", "User");
	const std::string content = util::wcstombs(str, len);
	msg.AddStringRef("content", content);
	if(!SendNetworkMessageToServer(msg))
	{
		if(!loginWindow || !loginWindow->GetMainWindow())
		{
			MessageBoxW(0, L"Failed to send chat message.", L"Error", 0);			
		}
		else
		{
			loginWindow->GetMainWindow()->AppendChatBox(RGB(255,0,0), RichEdit::Bold, L"Failed to send chat message.\n");
		}
	}
	
	return;		
}
void ChatClient::Logout() 
{
	NetworkMessage msg;
	msg.Add("type", NetworkMessage::MEMBER_LOGOUT);
	SendNetworkMessageToServer(msg);	
	running = false;	
}
void ChatClient::SetUsername(WCHAR* str, int len)
{		
	NetworkMessage msg;
	msg.Add("type", NetworkMessage::SET_USERNAME);
	msg.Add("username", util::wcstombs(str, len));			
	SendNetworkMessageToServer(msg);	
}

bool ChatClient::SendNetworkMessageToServer(const NetworkMessage& msg)
{
	std::scoped_lock<std::mutex> lck{sendMutex};
	return msg.SendJson(sock);
}

void ChatClient::Run() 
{
	std::wcout << L"Client started recieving: \n";	
	running.store(true);

	
	pingThread = std::thread([this]
	{		
		uint32_t failCount = 0;
		NetworkMessage pingMsg;
		pingMsg.AddStringRef("type", NetworkMessage::PING);	
		bool askedToQuit = false;	
		const int HIGH_FAIL_COUNT = 3;
		while(running)
		{
			std::this_thread::sleep_for(std::chrono::seconds(2));			
			if(SendNetworkMessageToServer(pingMsg))
			{
				failCount = 0;
			}
			else
			{
				failCount += failCount < HIGH_FAIL_COUNT; //only increment if failCount is smaller than HIGH_FAIL_COUNT
			}

			if(failCount >= HIGH_FAIL_COUNT && !askedToQuit)
			{
				askedToQuit = true;
				int mb_result = MessageBoxW(0,
				 L"Failing to communicate with server.\n"
				 L"Would you like to log out?"
				, L"Error", MB_YESNO);
				if(mb_result == IDYES)
				{
					// pending implementation - should disconnect socket and return to login window here
				}										
			}
		}		
	});

	char buffer[10'000];	
	uint32_t recv_fails = 0;
	bool notifiedOfDisconnect = false;
	while(running)
	{								
		NetworkMessage incomingMsg;
		auto status = incomingMsg.ReceiveJson(sock, buffer, sizeof(buffer));				
		if(status == NetworkMessage::ErrorFlag::Success)
		{			
			notifiedOfDisconnect = false;
			recv_fails = 0;						
			try
			{
				rapidjson::Document& doc = incomingMsg.GetDocument();
				std::string type = doc["type"].GetString();
				auto it = networkActionMap.find(type);
				if(it != networkActionMap.end())
				{
					it->second(doc);
				}
		
			}
			catch(const std::exception& e)
			{				
				std::wcout << L"Failed to parse json.\n";
			}			
		}
		else if(status == NetworkMessage::ErrorFlag::SocketError)
		{	
			if(!notifiedOfDisconnect && ++recv_fails >= 30)
			{
				notifiedOfDisconnect = true;
				if(auto mainWindow = loginWindow->GetMainWindow())
				{
					mainWindow->AppendChatBox(RGB(229,0,100), RichEdit::Bold, L"WARNING: Experiencing connection issues.\n");
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(2));			
		}
		else if(status == NetworkMessage::ErrorFlag::InvalidJson)
		{
			std::wcout << L"Received invalid json from server\n";
		}
	} //end of loop
}
ChatClient::ChatClient(WSADATA* wsa) : wsa(wsa)
{
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
		MainWindow* const mainWindow = loginWindow->GetMainWindow();
		std::string username = doc["username"].GetString();
		mainWindow->AppendToMemberList(doc["username"].GetString(), doc["username"].GetStringLength());
	};
	networkActionMap[NetworkMessage::MEMBER_LOGOUT] = [this](rapidjson::Document& doc)
	{
		std::wcout << L"Received logout message\n";
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
		MainWindow* const mainWindow = loginWindow->GetMainWindow();
		std::wstring usernameW = util::mbsvtowcs(doc["username"].GetString());		
		mainWindow->RemoveFromMemberList(usernameW.data());
		std::wstring chatMsg = fmt::format(L"[{}] has disconnected from the chat.", usernameW);
		mainWindow->AppendChatBox(RGB(255,0,0), RichEdit::Bold, chatMsg);
	};
}

		
