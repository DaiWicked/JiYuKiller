#pragma once
#include "stdafx.h"
#include "sciter-x.h"
#include "sciter-x-host-callback.h"
#include "CommonWindow.h"
#include "../JiYuTrainer/AppPublic.h"
#include <string>
#include <vector>

extern JTApp* currentApp;

class UdpAttackWindow : public sciter::host<UdpAttackWindow>, public CommonWindow
{
public:
	UdpAttackWindow(HWND parentHWnd);
	~UdpAttackWindow();

private:
	bool on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason) override;
	LRESULT onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled) override;

	void OnSend();
	void OnClearLog();
	void Log(const std::wstring& msg);

	// IP 解析：支持单IP、范围(192.168.1.10-50)、C段(192.168.1.1/24)
	std::vector<std::wstring> ParseIP(const std::wstring& input);

	// 构造数据包
	std::vector<unsigned char> BuildMsgPacket(const std::wstring& msg);
	std::vector<unsigned char> BuildCmdPacket(const std::wstring& cmd);

	// UDP 发送
	bool SendUdp(const std::wstring& ip, int port, const std::vector<unsigned char>& data);

	sciter::dom::element input_ip;
	sciter::dom::element input_port;
	sciter::dom::element select_mode;
	sciter::dom::element input_content;
	sciter::dom::element input_loop;
	sciter::dom::element input_interval;
	sciter::dom::element btn_send;
	sciter::dom::element btn_clear;
	sciter::dom::element text_log;

	std::wstring currentLog;

protected:
	bool onLoadHtml(LPCBYTE pData, DWORD len) override { return load_html(pData, len); }
	sciter::value docunmentComplete() override;
};
