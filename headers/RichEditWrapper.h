#pragma once
#include <windef.h>
#include <richedit.h>
class RichEdit
{
	HWND parent = NULL;
	HWND richEdit = NULL;
	CHARFORMAT cf{};
	
public:
	enum FontStyle : DWORD
	{
		None = 0,
		Bold = CFE_BOLD,
		Italic = CFE_ITALIC
	};
	static bool Create(HWND parent, HINSTANCE hInst, int x, int y, int width, int height, bool readOnly, RichEdit* out, DWORD extraFlags = 0);
	
	RichEdit& SetFormat(DWORD dwEffects);
	RichEdit& AppendText(COLORREF color, RichEdit::FontStyle fs, const WCHAR* text);
	void ScrollToBottom() const;
		

	HWND GetHwnd() const{ return richEdit;}
	HWND GetParent() const{ return parent;}
	bool IsNull() const { return richEdit == NULL;}
	operator bool() const{
		return richEdit != NULL;
	}
};