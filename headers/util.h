#pragma once

#include <unordered_map>
#include <string>
#include <windef.h>
#include <rapidjson/document.h>
#include <future>
#include <fmt/xchar.h>
namespace util
{
	inline std::unordered_map<HWND, std::unordered_map<std::wstring, void*>> hwndPropertyMap;	
	void* GetProperty(HWND hwnd, const std::wstring& propertyName);
	void SetProperty(HWND hwnd, const std::wstring& propertyName, void* data);
	std::future<int> ShowMessageBox(const std::string& title, const std::string& content, UINT utype = 0);

	

	template<class... Ts>
	int ShowMessageBoxFmt(const std::wstring& title, const std::wstring& format, Ts&&... args)
	{
		return MessageBoxW(0,fmt::format(format, std::forward<Ts>(args)...).data(),title.data(), 0);
	}
	template<class... Ts>
	std::future<int> ShowMessageBoxFmtAsync(const std::wstring& title, const std::wstring& format, Ts&&... args)
	{
		return std::async([=]{ MessageBoxW(0,fmt::format(format, std::forward<Ts>(args)...).data(),title.data(), 0);	});
	}

	std::string DocumentToJson(rapidjson::Document& doc);	
	void Json_AddMember(rapidjson::Document& doc, std::string_view name, std::string_view value);
	
	std::string wcstombs(const wchar_t* str, int len);
	std::wstring mbstowcs(const char* str, int len);
	std::string wcstombs(const std::wstring& str);
	std::wstring mbstowcs(const std::string& str);
}