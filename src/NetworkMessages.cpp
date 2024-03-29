#include "NetworkMessages.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

NetworkMessage::NetworkMessage() : doc{rapidjson::kObjectType}, targetDoc{&doc}
{

}

NetworkMessage::NetworkMessage(rapidjson::Document* other) : targetDoc(other){}

NetworkMessage& NetworkMessage::Add(std::string_view key, std::string_view value)
{	
	doc.AddMember(rapidjson::Value(key.data(),key.size()), 
		rapidjson::Value(value.data(),value.size(), doc.GetAllocator()), 
			doc.GetAllocator());
	return *this;
}
NetworkMessage& NetworkMessage::AddStringRef(std::string_view key, std::string_view value)
{	
	doc.AddMember(rapidjson::Value(key.data(),key.size()), 
		rapidjson::Value(value.data(),value.size()), 
			doc.GetAllocator());
	return *this;
}



bool NetworkMessage::SendJson(SOCKET socket) const
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	targetDoc->Accept(writer);
	const char* const json = buffer.GetString();
	
	const int32_t jsonSize = buffer.GetLength();
	const int32_t jsonSizeNetworkOrder = htonl(jsonSize);
	
	int32_t sent_size = 0;
	while(sent_size < sizeof(jsonSize))
	{
		int32_t bytesLeft = sizeof(jsonSize) - sent_size;
		int32_t bytesReceived = send(socket, (const char*)&jsonSizeNetworkOrder + sent_size, bytesLeft, 0 );
		if(bytesReceived == SOCKET_ERROR)
		{
			return false;
		}
		sent_size += bytesReceived;			
	}

	sent_size = 0;
	while(sent_size < jsonSize)
	{
		int32_t bytesLeft = jsonSize - sent_size;
		int32_t bytesReceived = send(socket, json + sent_size, bytesLeft, 0);
		if(bytesReceived == SOCKET_ERROR)
		{
			return false;
		}
		sent_size += bytesReceived;			
	}
	return true;
}
bool NetworkMessage::SendJson(SOCKET socket, std::mutex& mutex) const
{	
	std::scoped_lock<std::mutex> lck{mutex};
	return SendJson(socket);
}
std::string NetworkMessage::ToJson() const 
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	targetDoc->Accept(writer);
	return buffer.GetString();
}

NetworkMessage::ErrorFlag NetworkMessage::ReceiveJson(SOCKET sock, char* buffer, int32_t bufferSize)
{		
	int32_t jsonSize;

	int32_t recv_size = 0;
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
	jsonSize = ntohl(jsonSize);

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
	buffer[recv_size] = '\0';
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