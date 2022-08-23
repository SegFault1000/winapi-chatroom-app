#include "NetworkMessages.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <winsock2.h>
NetworkMessage::NetworkMessage() : doc{rapidjson::kObjectType}, targetDoc{&doc}
{

}

NetworkMessage::NetworkMessage(rapidjson::Document* other) : targetDoc(other){}

NetworkMessage& NetworkMessage::Add(std::string_view key, std::string_view value)
{
	doc.AddMember(rapidjson::Value(key.data(),key.size()), 
		rapidjson::Value(value.data(),value.size()), 
			doc.GetAllocator());
	return *this;
}



bool NetworkMessage::SendJson(SOCKET client)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	targetDoc->Accept(writer);
	const char* const json = buffer.GetString();
	
	int32_t jsonSize = buffer.GetLength();
	int32_t sent_size = send(client, (const char*)&jsonSize, sizeof(jsonSize), 0);
	while(sent_size < sizeof(jsonSize))
	{
		int32_t bytesLeft = sizeof(jsonSize) - sent_size;
		int32_t bytesReceived = send(client, (const char*)&jsonSize + sent_size, bytesLeft, 0 );
		if(bytesReceived <= 0)
		{
			return false;
		}
		sent_size += bytesReceived;			
	}

	sent_size = send(client, json, jsonSize, 0);
	while(sent_size < jsonSize)
	{
		int32_t bytesLeft = jsonSize - sent_size;
		int32_t bytesReceived = send(client, json + sent_size, bytesLeft, 0);
		if(bytesReceived < 0)
		{
			continue;
		}
		sent_size += bytesReceived;			
	}
	return true;
}

std::string NetworkMessage::ToJson() const {
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	targetDoc->Accept(writer);
	return buffer.GetString();
}

NetworkMessage::ErrorFlag NetworkMessage::ReceiveJson(SOCKET sock, char* buffer, int32_t bufferSize)
{		
	int32_t jsonSize = -1;
	int32_t recv_size = recv(sock, (char*)&jsonSize, sizeof(jsonSize), 0);
	
	if(recv_size == SOCKET_ERROR)
	{								
		return ErrorFlag::SocketError;
	}
	while(recv_size < sizeof(jsonSize))
	{	
		int32_t bytesLeft = sizeof(jsonSize) - recv_size;
		int32_t receivedBytes = recv(sock, (char*)&jsonSize + recv_size, bytesLeft, 0);
		if(receivedBytes == SOCKET_ERROR)
		{								
			return ErrorFlag::SocketError;
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
			return ErrorFlag::SocketError;
		}
		recv_size += receivedBytes;
	}
	targetDoc->Parse<rapidjson::kParseStopWhenDoneFlag>(buffer);
	return targetDoc->IsObject() ? ErrorFlag::Success : ErrorFlag::InvalidJson;
}

NetworkMessage& NetworkMessage::EditMember(std::string_view key, std::string_view newValue)
{
	rapidjson::Document::MemberIterator it = doc.FindMember(key.data());
	auto allocator = doc.GetAllocator();
	if(it == doc.MemberEnd())
	{
		doc.AddMember(rapidjson::Value(key.data(), allocator), 
			rapidjson::Value(newValue.data(), allocator),
			allocator
		);
	}
	it->value = rapidjson::Value(newValue.data(), allocator);
	return *this;
}