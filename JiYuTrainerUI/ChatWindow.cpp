#include "stdafx.h"
#include "ChatWindow.h"
#include "resource.h"
#include "../JiYuTrainer/SysHlp.h"
#include "../JiYuTrainer/StringHlp.h"
#include <string>

extern int screenWidth, screenHeight;

ChatWindow::ChatWindow(HWND parentHWnd) :
	CommonWindow(parentHWnd, 520, 560,
		L"JiYuTrainerChatWindow", L"小小私聊(beta) - by jiyu_chat",
		IDR_HTML_CHAT)
{
	init();
	Show();
	JyUdpAttack::currentJyUdpAttack->sendResultReceivehWnd = _hWnd;
}

ChatWindow::~ChatWindow()
{
}

// ============================================================
// 座位号 <-> IP 末位换算（移植自 jiyu_chat chat.py，算法保持原样）
// ============================================================

int ChatWindow::StuID2IPLast(int stuID)
{
	// Python 原逻辑:
	// if StuID / 6 == StuID // 6:  (能被6整除)
	//     OP = StuID / 6;  ip_last = 60 + OP
	// else:
	//     R = StuID % 6;  TP = R * 10;  OP = StuID // 6 + 1;  ip_last = TP + OP
	if (stuID <= 0) return 0;
	if (stuID % 6 == 0)
	{
		int op = stuID / 6;
		return 60 + op;
	}
	else
	{
		int r = stuID % 6;
		int tp = r * 10;
		int op = stuID / 6 + 1;
		return tp + op;
	}
}

int ChatWindow::IPLast2StuID(int ipLastOctet)
{
	// Python 原逻辑:
	// m = ip_last; o = m - 1;
	// result = (m - o // 10 * 10) * 6 - (6 - (o // 10))
	int m = ipLastOctet;
	int o = m - 1;
	int tens = o / 10;
	return (m - tens * 10) * 6 - (6 - tens);
}

// ============================================================
// 网络工具
// ============================================================

std::wstring ChatWindow::GetLocalIP()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return L"127.0.0.1";

	char hostname[256] = { 0 };
	std::wstring result = L"127.0.0.1";
	if (gethostname(hostname, sizeof(hostname)) == 0)
	{
		struct addrinfoW hints, *res = nullptr;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		wchar_t whost[256];
		MultiByteToWideChar(CP_ACP, 0, hostname, -1, whost, 256);
		if (GetAddrInfoW(whost, nullptr, &hints, &res) == 0 && res)
		{
			sockaddr_in* addr = (sockaddr_in*)res->ai_addr;
			char ipstr[INET_ADDRSTRLEN];
			InetNtopA(AF_INET, &addr->sin_addr, ipstr, sizeof(ipstr));
			wchar_t wip[64];
			MultiByteToWideChar(CP_ACP, 0, ipstr, -1, wip, 64);
			result = wip;
			FreeAddrInfoW(res);
		}
	}
	WSACleanup();
	return result;
}

std::wstring ChatWindow::GetLocalIPPrefix()
{
	std::wstring ip = GetLocalIP();
	size_t pos = ip.find_last_of(L'.');
	if (pos == std::wstring::npos) return L"192.168.1.";
	return ip.substr(0, pos + 1);
}

bool ChatWindow::IsOnline(const std::wstring& ip)
{
	// 用 CreateProcess 隐藏 cmd 窗口执行 ping，避免弹窗抢占焦点
	std::wstring cmd = L"cmd /c ping -n 1 -w 1000 " + ip + L" > nul 2>&1";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	ZeroMemory(&pi, sizeof(pi));
	BOOL ok = CreateProcess(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	if (!ok) return false;
	WaitForSingleObject(pi.hProcess, 2000);
	DWORD exitCode = 1;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return exitCode == 0;
}

// ============================================================
// UI 事件
// ============================================================

bool ChatWindow::on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason)
{
	sciter::dom::element ele(he);
	if (type == BUTTON_CLICK)
	{
		sciter::string id = ele.get_attribute("id");
		if (id == L"btn_check_target")
			CheckTarget();
		else if (id == L"btn_send_msg")
			SendChatMessage();
	}
	return false;
}

LRESULT ChatWindow::onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled)
{
	switch (message)
	{
	case WM_MY_FORCE_HIDE: {
		Hide();
		*handled = TRUE;
		return 0;
	}
	case WM_MY_SEND_ADD_RESULT: {
		auto* str = (std::wstring*)wParam;
		AddChatRecord(str->c_str());
		delete str;
		*handled = TRUE;
		return 0;
	}
	default:
		return 0;
	}
}

void ChatWindow::CheckTarget()
{
	std::wstring seatStr = input_target_seat.get_value().to_string();
	if (seatStr.empty())
	{
		AddChatRecord(L"请输入对方座位号！");
		return;
	}

	targetSeatID = _wtoi(seatStr.c_str());
	if (targetSeatID <= 0)
	{
		AddChatRecord(L"座位号必须是正整数！");
		return;
	}

	int ipLast = StuID2IPLast(targetSeatID);
	targetIP = GetLocalIPPrefix() + std::to_wstring(ipLast);

	AddChatRecord(FormatString(L"正在查找 %d 号同学 (%s) ...", targetSeatID, targetIP.c_str()).c_str());

	if (IsOnline(targetIP))
	{
		text_target_status.set_text(FormatString(L"%d 号同学在线 (%s)", targetSeatID, targetIP.c_str()).c_str());
		AddChatRecord(FormatString(L"%d 号同学似乎在线！可以开始聊天了。", targetSeatID).c_str());
	}
	else
	{
		text_target_status.set_text(FormatString(L"%d 号同学不在线或未开机", targetSeatID).c_str());
		AddChatRecord(FormatString(L"似乎没有找到 %d 号同学...对方可能未开机。", targetSeatID).c_str());
	}
}

void ChatWindow::SendChatMessage()
{
	std::wstring msg = input_message.get_value().to_string();
	if (msg.empty())
	{
		AddChatRecord(L"不可发送空白消息！");
		return;
	}
	if (msg.length() > 80)
	{
		AddChatRecord(L"消息太长了，请控制在80字以内。");
		return;
	}
	if (targetIP.empty())
	{
		AddChatRecord(L"请先点击「查找同学」确定目标！");
		return;
	}

	DWORD port = input_port.get_value().get(4705);
	if (port <= 0 || port > 65535)
	{
		AddChatRecord(L"端口错误，必须是 1-65535 的数字。");
		return;
	}

	// 消息前缀：「X号同学:」，与 jiyu_chat 的 prefix 逻辑一致
	std::wstring fullMsg = FormatString(L"%d号同学:%s", mySeatID, msg.c_str());
	JyUdpAttack::currentJyUdpAttack->SendText(targetIP, port, fullMsg);

	AddChatRecord(FormatString(L"我 -> %d号: %s", targetSeatID, msg.c_str()).c_str());
	input_message.set_value(sciter::value(L""));
}

void ChatWindow::AddChatRecord(LPCWSTR text)
{
	auto ele = chat_log.create("div", text);
	chat_log.append(ele);
	chat_log.scroll_to_view();
}

sciter::value ChatWindow::docunmentComplete()
{
	CommonWindow::docunmentComplete();

	sciter::dom::element root = get_root();
	chat_log = root.get_element_by_id(L"chat_log");
	input_target_seat = root.get_element_by_id(L"input_target_seat");
	input_port = root.get_element_by_id(L"input_port");
	input_message = root.get_element_by_id(L"input_message");
	text_local_info = root.get_element_by_id(L"text_local_info");
	text_target_status = root.get_element_by_id(L"text_target_status");

	// 初始化本机信息
	std::wstring localIP = GetLocalIP();
	size_t pos = localIP.find_last_of(L'.');
	if (pos != std::wstring::npos)
	{
		int ipLast = _wtoi(localIP.substr(pos + 1).c_str());
		mySeatID = IPLast2StuID(ipLast);
	}
	text_local_info.set_text(FormatString(L"本机IP: %s   座位号: %d", localIP.c_str(), mySeatID).c_str());
	input_port.set_value(sciter::value(4705));

	AddChatRecord(L"~~ 欢迎使用小小私聊 ~~");
	AddChatRecord(L"原理：极域学生端不对 UDP 包做身份验证，可构造数据包发送消息。");
	AddChatRecord(L"提示：座位号换算算法移植自 jiyu_chat，按6人一排布局推算。");

	return sciter::value(true);
}
