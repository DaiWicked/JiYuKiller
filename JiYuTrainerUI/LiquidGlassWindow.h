#pragma once
#include "stdafx.h"
#include "sciter-x.h"
#include "sciter-x-host-callback.h"
#include "CommonWindow.h"
#include "../JiYuTrainer/AppPublic.h"

extern JTApp* currentApp;

class LiquidGlassWindow : public sciter::host<LiquidGlassWindow>, public CommonWindow
{
public:
	LiquidGlassWindow(HWND parentHWnd);
	~LiquidGlassWindow();

	sciter::value applyLiquidGlass(sciter::value opacity, sciter::value color);

private:
	bool on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason) override;
	LRESULT onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled) override;

protected:
	bool onLoadHtml(LPCBYTE pData, DWORD len) override { return load_html(pData, len); }
	sciter::value docunmentComplete() override;

	BEGIN_FUNCTION_MAP
		FUNCTION_2("applyLiquidGlass", applyLiquidGlass);
	END_FUNCTION_MAP
};