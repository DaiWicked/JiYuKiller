#pragma once
#include "stdafx.h"
#include "sciter-x.h"
#include "sciter-x-host-callback.h"
#include "CommonWindow.h"
#include "../JiYuTrainer/AppPublic.h"
#include "../JiYuTrainer/FileLogger.h"
#include <string>

extern JTApp* currentApp;

class IMTeacherWindow : public sciter::host<IMTeacherWindow>, public CommonWindow
{
public:
	IMTeacherWindow(HWND parentHWnd);
	~IMTeacherWindow();

private:
	bool on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason) override;
	LRESULT onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled) override;

	void OnStart();
	void OnStop();
	void OnSendCommand();
	void OnRefreshStudents();
	void OnClearLog();
	void Log(const std::wstring& msg);
	void AppendLog(const std::wstring& msg);

	// 进程管理
	bool StartProcess();
	void StopProcess();
	bool SendToProcess(const std::wstring& cmd);
	std::wstring ReadLogFile();

	PROCESS_INFORMATION pi_;
	HANDLE hStdinWrite_;
	bool running_;
	std::wstring logPath_;

	sciter::dom::element btn_start;
	sciter::dom::element btn_stop;
	sciter::dom::element btn_refresh;
	sciter::dom::element btn_send;
	sciter::dom::element btn_clear;
	sciter::dom::element input_cmd;
	sciter::dom::element text_status;
	sciter::dom::element text_students;
	sciter::dom::element text_log;

protected:
	bool onLoadHtml(LPCBYTE pData, DWORD len) override { return load_html(pData, len); }
	sciter::value docunmentComplete() override;
};