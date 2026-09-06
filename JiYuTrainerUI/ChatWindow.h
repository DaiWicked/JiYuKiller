#pragma once
#include "stdafx.h"
#include "sciter-x.h"
#include "sciter-x-host-callback.h"
#include "CommonWindow.h"
#include "../JiYuTrainer/AppPublic.h"
#include "../JiYuTrainer/JyUdpAttack.h"

extern JTApp* currentApp;

class ChatWindow : public sciter::host<ChatWindow>, public CommonWindow
{
public:
	ChatWindow(HWND parentHWnd);
	~ChatWindow();

private:

	bool on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason) override;
	LRESULT onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled) override;

	// 座位号 <-> IP 末位换算（移植自 jiyu_chat，算法保持原样未改动）
	static int StuID2IPLast(int stuID);
	static int IPLast2StuID(int ipLastOctet);

	// 获取本机 IP
	static std::wstring GetLocalIP();
	// 获取本机 IP 前缀（如 192.168.1.）
	static std::wstring GetLocalIPPrefix();

	// ping 检测对方是否在线
	static bool IsOnline(const std::wstring& ip);

	void CheckTarget();
	void SendChatMessage();
	void AddChatRecord(LPCWSTR text);

	sciter::dom::element chat_log;
	sciter::dom::element input_target_seat;
	sciter::dom::element input_port;
	sciter::dom::element input_message;
	sciter::dom::element text_local_info;
	sciter::dom::element text_target_status;

	int mySeatID = 0;
	int targetSeatID = 0;
	std::wstring targetIP;

protected:
	bool onLoadHtml(LPCBYTE pData, DWORD len) override { return load_html(pData, len); }
	sciter::value docunmentComplete() override;

};
