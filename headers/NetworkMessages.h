#pragma once
#include <string>
#include <rapidjson/document.h>
#include <winsock2.h>

class NetworkMessage
{		
	rapidjson::Document doc;
	rapidjson::Document* targetDoc = NULL;
public:
	inline static const std::string CHAT_MESSAGE = "CHAT_MESSAGE";
	inline static const std::string SET_USERNAME = "SET_USERNAME";
	inline static const std::string SET_USERNAME_COLOR = "SET_USERNAME_COLOR";
	inline static const std::string MEMBER_JOIN = "MEMBER_JOIN";
	inline static const std::string MEMBER_LIST = "MEMBER_LIST";
	inline static const std::string MEMBER_LIST_ADD = "MEMBER_LIST_ADD";
	enum class ErrorFlag : int32_t
	{
		Success, SocketError, ConnectionFail, InvalidJson
	};
	NetworkMessage();
	
	NetworkMessage(rapidjson::Document* other);
	
	NetworkMessage& Add(std::string_view key, std::string_view value);


	bool SendJson(SOCKET client);
	
	ErrorFlag ReceiveJson(SOCKET sock, char* buffer, int32_t bufferSize);
	
	rapidjson::Document& GetDocument() { return doc; }
	NetworkMessage& EditMember(std::string_view key, std::string_view newValue);
	std::string ToJson() const;	
};