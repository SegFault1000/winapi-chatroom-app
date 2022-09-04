#pragma once
#include <winsock2.h>
#include <rapidjson/document.h>
#include <string>
#include <mutex>

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
	inline static const std::string MEMBER_LIST_REMOVE = "MEMBER_LIST_REMOVE";


	inline static const std::string PING = "PING";	
	inline static const std::string MEMBER_DISCONNECT = "MEMBER_DISCONNECT";
	inline static const std::string MEMBER_LOGOUT= "MEMBER_LOGOUT";

	inline static const std::string MEMBER_KICK = "MEMBER_KICK";
	inline static const std::string MEMBER_BAN = "MEMBER_LOGOUT";
	inline static const std::string MEMBER_MUTE = "MEMBER_MUTE";


	enum class ErrorFlag : int32_t
	{
		Success, SocketError, ConnectionFail, InvalidJson
	};
	NetworkMessage();
	
	NetworkMessage(rapidjson::Document* other);
	
	NetworkMessage& Add(std::string_view key, std::string_view value);
	NetworkMessage& AddStringRef(std::string_view key, std::string_view value);


	bool SendJson(SOCKET client) const;
	bool SendJson(SOCKET client, std::mutex& mutex) const;
	
	ErrorFlag ReceiveJson(SOCKET sock, char* buffer, int32_t bufferSize);	
	
	rapidjson::Document& GetDocument() { return doc; }
	NetworkMessage& EditMember(std::string_view key, std::string_view newValue);
	std::string ToJson() const;	
	bool SetFromJson(const std::string& json)
	{
		doc.Parse<rapidjson::kParseStopWhenDoneFlag>(json.data());
		return doc.IsObject();
	}
};