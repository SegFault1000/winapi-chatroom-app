#include "util.h"
#include <winuser.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <windows.h>
namespace util
{
	std::future<int> ShowMessageBox(const std::string& title, const std::string& content, UINT utype) {
		return std::async([=]{ return MessageBoxA(NULL, content.data(), title.data(), 0);});
	}

	std::string DocumentToJson(rapidjson::Document& doc) {	
		using rapidjson::StringBuffer;
		using rapidjson::Writer;
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		doc.Accept(writer);
		return buffer.GetString();
	}

	void Json_AddMember(rapidjson::Document& doc, std::string_view name, std::string_view value) {		
		doc.AddMember(rapidjson::Value(name.data(), doc.GetAllocator()), 
			rapidjson::Value(value.data(), doc.GetAllocator()),
			doc.GetAllocator()
		);			
	}

	std::wstring mbstowcs(const char* str, int len) {
		int length_required = MultiByteToWideChar(CP_UTF8, 0, str, len, NULL, 0);
		std::wstring result(length_required, L' ');
		MultiByteToWideChar(CP_UTF8, 0, str, len, result.data(), result.size());
		return result;	
	}

	std::string wcstombs(const std::wstring& str) {
		return util::wcstombs(str.c_str(), str.size());	
	}

	std::wstring mbstowcs(const std::string& str) {
		return util::mbstowcs(str.c_str(), str.size())		;	
	}

	std::string wcstombs(const wchar_t* str, int len) {	
		int length_required = WideCharToMultiByte(CP_UTF8, 0, str, len, NULL, 0, NULL, NULL);
		std::string result(length_required, ' ');
		WideCharToMultiByte(CP_UTF8, 0, str, len, result.data(), result.size(), NULL, NULL);
		return result;	
	}
}
void* util::GetProperty(HWND hwnd, const std::wstring& propertyName)
{
	auto it = hwndPropertyMap.find(hwnd);
	if(it == hwndPropertyMap.end())
		return NULL;
	auto& propertiesForHwnd = it->second;
	auto it2 = propertiesForHwnd.find(propertyName);
	if(it2 == propertiesForHwnd.end())
		return NULL;
	return it2->second;
}

void util::SetProperty(HWND hwnd, const std::wstring& propertyName, void* data)
{
	hwndPropertyMap[hwnd][propertyName] = data;
}
