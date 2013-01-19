// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include <WinInet.h>

#pragma comment(lib, "wininet.lib")

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_ICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_ICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	this->m_btn_start = GetDlgItem(IDC_BUTTON_START);
	this->m_btn_stop = GetDlgItem(IDC_BUTTON_STOP);
	this->m_radio_status = GetDlgItem(IDC_RADIO_STATUS);
	this->m_radio_msg = GetDlgItem(IDC_RADIO_MSG);
	this->m_edit_id = GetDlgItem(IDC_EDIT_ID);
	this->m_edit_nick_name = GetDlgItem(IDC_EDIT_NICK_NAME);
	this->m_static_id = GetDlgItem(IDC_STATIC_ID);
	this->m_static_nick_name = GetDlgItem(IDC_STATIC_NICK_NAME);

	this->m_btn_stop.EnableWindow(FALSE);
	this->m_radio_msg.SetCheck(1);
	this->m_static_id.EnableWindow(FALSE);
	this->m_edit_id.EnableWindow(FALSE);
	this->Event = CreateEvent(NULL, TRUE, FALSE, NULL);

	return TRUE;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseHandle(this->Event);
	EndDialog(wID);
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	PTHREAD_PARAMETER ThreadParameter = (PTHREAD_PARAMETER)lpParameter;
	HANDLE Event = ThreadParameter->Event;
	DWORD Type = ThreadParameter->Type;
	TCHAR *Message = ThreadParameter->Message;
	BOOL ShutDown = FALSE;

	if (Type == 0)
	{
		HINTERNET hInt = InternetOpen(TEXT("Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.52 Safari/537.17"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (hInt)
		{
			TCHAR URL[64] = {0};
			wsprintf(URL, TEXT("%s%s%s"), TEXT("http://wpa.qq.com/pa?p=1:"), Message, TEXT(":1"));
			while(TRUE)
			{
				HINTERNET hUrl = InternetOpenUrl(hInt, URL, NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE, 0);
				if (hUrl)
				{
					DWORD DataSize = 0;
					if (InternetQueryDataAvailable(hUrl, &DataSize, 0, 0))
					{
						if (DataSize == 2329)
							ShutDown = TRUE;
						else if (DataSize == 2262)
							ShutDown = FALSE;
					}
					InternetCloseHandle(hUrl);
				}
				if (ShutDown || WaitForSingleObject(Event, TimeElapse * 1000) == WAIT_OBJECT_0)
					break;
			}
			InternetCloseHandle(hInt);
		}
	}
	else if (Type == 1)
	{
		HWND hWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
		if (hWnd)
		{
			hWnd = FindWindowEx(hWnd, NULL, TEXT("TrayNotifyWnd"), NULL);
			if (hWnd)
			{
				hWnd = FindWindowEx(hWnd, NULL, TEXT("SysPager"), NULL);
				if (hWnd)
					hWnd = FindWindowEx(hWnd, NULL, TEXT("ToolbarWindow32"), NULL);
			}
		}
		if (hWnd != NULL)
		{
			TBBUTTON Button;
			DWORD PID = 0;
			TRAY_DATA TrayData;
			GetWindowThreadProcessId(hWnd, &PID);
			HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, PID);
			if (hProcess != INVALID_HANDLE_VALUE)
			{
				LPVOID lpButton = VirtualAllocEx(hProcess, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);
				LPVOID lpRect = VirtualAllocEx(hProcess, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE);
				if (lpButton && lpRect)
				{
					while(TRUE)
					{
						DWORD Count = (DWORD)SendMessage(hWnd, TB_BUTTONCOUNT, 0, 0);
						for (DWORD Index = 0; Index < Count; ++Index)
						{	
							if (SendMessage(hWnd, TB_GETBUTTON, Index, (LPARAM)lpButton))
							{
								ReadProcessMemory(hProcess, lpButton, &Button, sizeof(TBBUTTON), NULL);
								ReadProcessMemory(hProcess, (LPVOID)Button.dwData, &TrayData, sizeof(TRAY_DATA), NULL);
								DWORD dwPathLength = lstrlen(TrayData.szExePath);
								if (lstrcmpi(TEXT("QQ.exe"), TrayData.szExePath + dwPathLength - 6) == 0)
								{
									if (lstrlen(TrayData.szTip) == 0)	// New message comes
									{
										RECT ButtonRect, TrayRect;
										if (SendMessage(hWnd, TB_GETRECT, Button.idCommand, (LPARAM)lpRect))
										{
											ReadProcessMemory(hProcess, lpRect, &ButtonRect, sizeof(RECT), NULL);
											if (GetWindowRect(hWnd, &TrayRect))
											{
												DWORD CenterX = (ButtonRect.left + ButtonRect.right) / 2;
												DWORD CenterY = (ButtonRect.top + ButtonRect.bottom) / 2;
												SetCursorPos(TrayRect.left + CenterX, TrayRect.top + CenterY);
												SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(CenterX, CenterY));
												Sleep(500);
												SendMessage(hWnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(CenterX, CenterY));
												Sleep(1000);
												if (FindWindowEx(NULL, NULL, TEXT("ChatBox_PreviewWnd"), Message))
													ShutDown = TRUE;
											}
										}
									}
								}
							}
						}
						if (ShutDown || WaitForSingleObject(Event, TimeElapse * 1000) == WAIT_OBJECT_0)
							break;
					}
					VirtualFreeEx(hProcess, lpButton, 0, MEM_RELEASE);
					VirtualFreeEx(hProcess, lpRect, 0, MEM_RELEASE);
				}
				CloseHandle(hProcess);
			}
		}
	}
	else
	{
		/////////////////////
	}
	if (ShutDown)
	{
		MessageBox(NULL, TEXT("The computer is gonna shut down in 20 seconds"), TEXT("Hello"), MB_OK);
		system("shutdown /s /f /t 20");
	}

	delete lpParameter;
	return 0;
}

LRESULT CMainDlg::OnBnClickedButtonStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR Message[MessageLength];
	DWORD Type = -1;

	ResetEvent(this->Event);
	if (this->m_radio_status.GetCheck() == 1)
	{
		if (this->m_edit_id.GetWindowTextLength() <= 0)
		{
			MessageBox(TEXT("Please input QQ ID!"), TEXT("Error"), MB_OK);
			return 0;
		}
		this->m_edit_id.GetWindowText(Message, MessageLength);
		Type = 0;
	}
	else if (this->m_radio_msg.GetCheck() == 1)
	{
		if (this->m_edit_nick_name.GetWindowTextLength() <= 0)
		{
			MessageBox(TEXT("Please input QQ Nick Name!"), TEXT("Error"), MB_OK);
			return 0;
		}
		this->m_edit_nick_name.GetWindowText(Message, MessageLength);
		Type = 1;
	}
	else
	{
		MessageBox(TEXT("What the fuck???"), TEXT("Fatal Error"), MB_OK);
		return 0;
	}
	PTHREAD_PARAMETER ThreadParameter = new THREAD_PARAMETER;
	if (ThreadParameter)
	{
		ThreadParameter->Event = this->Event;
		ThreadParameter->Type = Type;
		CopyMemory(ThreadParameter->Message, Message, MessageLength * sizeof(TCHAR));
		HANDLE hThread = CreateThread(NULL, 0, ThreadProc, ThreadParameter, 0, NULL);
		if (hThread != INVALID_HANDLE_VALUE)
		{
			this->m_radio_status.EnableWindow(FALSE);
			this->m_radio_msg.EnableWindow(FALSE);
			this->m_btn_start.EnableWindow(FALSE);
			this->m_btn_stop.EnableWindow(TRUE);
			CloseHandle(hThread);
			return 0;
		}
	}
	MessageBox(TEXT("Aha? Failed to start"), TEXT("Error"), MB_OK);
	return 0;
}


LRESULT CMainDlg::OnBnClickedButtonStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	this->m_btn_stop.EnableWindow(FALSE);
	SetEvent(this->Event);
	this->m_radio_status.EnableWindow(TRUE);
	this->m_radio_msg.EnableWindow(TRUE);
	this->m_btn_start.EnableWindow(TRUE);
	return 0;
}


LRESULT CMainDlg::OnBnClickedRadioStatus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	this->m_static_id.EnableWindow(TRUE);
	this->m_edit_id.EnableWindow(TRUE);
	this->m_static_nick_name.EnableWindow(FALSE);
	this->m_edit_nick_name.EnableWindow(FALSE);
	return 0;
}


LRESULT CMainDlg::OnBnClickedRadioMsg(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	this->m_static_id.EnableWindow(FALSE);
	this->m_edit_id.EnableWindow(FALSE);
	this->m_static_nick_name.EnableWindow(TRUE);
	this->m_edit_nick_name.EnableWindow(TRUE);
	return 0;
}
