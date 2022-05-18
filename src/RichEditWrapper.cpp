#include "RichEditWrapper.h"

bool RichEdit::Create(HWND parent, HINSTANCE hInst, int x, int y, int width, int height, bool readOnly, RichEdit* out, DWORD extraFlags)
{				
	auto flags = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL | extraFlags;
	if(readOnly)
		flags |= ES_READONLY;


	HWND richEdit = CreateWindowExW(0, RICHEDIT_CLASSW, L"", flags, x,y,width,height, parent, NULL, hInst, NULL);
	if(!richEdit)
		return false;

	out->parent = parent;
	SetPropW(richEdit, L"MAINWINDOW", (HANDLE)parent);
	out->richEdit = richEdit;		
	out->SetFormat(0);

	return true;
}

RichEdit& RichEdit::SetFormat(DWORD dwEffects)
{					
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_BOLD;				
	cf.dwEffects = dwEffects;
	SendMessageW(richEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	return *this;
}

RichEdit& RichEdit::AppendText(COLORREF color, RichEdit::FontStyle fs, const WCHAR* text)
{							
	int oldX, oldY;
	SendMessageW(richEdit, EM_GETSEL, (WPARAM)&oldX, (LPARAM)&oldY);
	int length = GetWindowTextLengthW(richEdit);
	SendMessageW(richEdit, EM_SETSEL, length, length);
	cf.dwEffects = fs;
	cf.crTextColor = color;

	SendMessageW(richEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);			
	
	SendMessageW(richEdit, EM_REPLACESEL, 0, (LPARAM)text);
	SendMessageW(richEdit, EM_SETSEL, oldX, oldY);
	return *this;
}

void RichEdit::ScrollToBottom() const {
	SendMessageW(richEdit, WM_VSCROLL, SB_BOTTOM, 0);				
}
