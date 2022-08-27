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
	std::string wcsvtombs(std::wstring_view str)
	{
		return util::wcstombs(str.data(), str.size());
	}
	std::wstring mbsvtowcs(std::string_view str)
	{
		return util::mbstowcs(str.data(), str.size());
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

std::vector<std::string_view> util::split_sv(std::string_view str,
																						const uint32_t maxSplit)
{
	constexpr auto npos = std::string_view::npos;
	constexpr auto whitespace = " \n\r\t";

	std::vector<std::string_view> vec;
	size_t i = 0;
	size_t amountSplit = 0;
	size_t endIdx;
	while (i != npos)
	{
		if (maxSplit != -1 && ++amountSplit > maxSplit)
		{
			i = str.find_first_not_of(whitespace, i);
			if(i == npos)
			{
				return vec;
			}
			
			if(str[i] == '"')
			{
				endIdx = str.find_first_not_of('"', i + 1);
				if(endIdx != npos)				
				{
					vec.emplace_back(str.data() + i + 1, endIdx - i - 1);
					i = endIdx + 1;
					return vec;
				}
			}
			vec.emplace_back(str.data() + i, str.size() - i);
			return vec;
		}

		i = str.find_first_not_of(whitespace, i);		
		if (i == npos)
		{
			return vec;
		}
		else if(str[i] == '"')
		{
			endIdx = str.find('"', i + 1);
			if(endIdx != npos)
			{
				vec.emplace_back(str.data() + i + 1, endIdx - i - 1);
				i = endIdx + 1;
				continue;
			}
		}

		endIdx = str.find_first_of(whitespace, i + 1);

		vec.emplace_back(str.data() + i,
										(endIdx == npos ? str.size() : endIdx) - i);
		i = endIdx;
	}
	return vec;
}


std::vector<std::wstring_view> util::split_sv(std::wstring_view str,
																						const uint32_t maxSplit)
{
	constexpr auto npos = std::wstring_view::npos;
	constexpr auto whitespace = L" \n\r\t";

	std::vector<std::wstring_view> vec;
	size_t i = 0;
	size_t amountSplit = 0;
	size_t endIdx;
	while (i != npos)
	{
		if (maxSplit != -1 && ++amountSplit > maxSplit)
		{
			i = str.find_first_not_of(whitespace, i);
			if(i == npos)
				return vec;
			
			if(str[i] == L'"')
			{
				endIdx = str.find_first_not_of(L'"', i + 1);
				if(endIdx != npos)				
				{
					vec.emplace_back(str.data() + i + 1, endIdx - i - 1);
					i = endIdx + 1;
					return vec;
				}
			}
			vec.emplace_back(str.data() + i, str.size() - i);
			return vec;
		}

		i = str.find_first_not_of(whitespace, i);		
		if (i == npos)
		{
			return vec;
		}
		else if(str[i] == L'"')
		{
			endIdx = str.find(L'"', i + 1);
			if(endIdx != npos)
			{
				vec.emplace_back(str.data() + i + 1, endIdx - i - 1);
				i = endIdx + 1;
				continue;
			}
		}

		endIdx = str.find_first_of(whitespace, i + 1);

		vec.emplace_back(str.data() + i,
										(endIdx == npos ? str.size() : endIdx) - i);
		i = endIdx;
	}
	return vec;
}
