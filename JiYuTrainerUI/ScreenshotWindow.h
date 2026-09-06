#pragma once
#include "stdafx.h"
#include "sciter-x.h"
#include "sciter-x-host-callback.h"
#include "CommonWindow.h"
#include "../JiYuTrainer/AppPublic.h"

extern JTApp* currentApp;

class ScreenshotWindow : public sciter::host<ScreenshotWindow>, public CommonWindow
{
public:
	ScreenshotWindow(HWND parentHWnd);
	~ScreenshotWindow();

private:
	bool on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason) override;
	LRESULT onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled) override;

	void OnChooseImage();
	void OnClearImage();
	void OnApply();
	void OnCancel();
	void RefreshUI();

	sciter::dom::element btn_choose;
	sciter::dom::element btn_clear;
	sciter::dom::element text_path;
	sciter::dom::element text_status;

	std::wstring currentImagePath;

protected:
	bool onLoadHtml(LPCBYTE pData, DWORD len) override { return load_html(pData, len); }
	sciter::value docunmentComplete() override;
};
