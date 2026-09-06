#include "stdafx.h"
#include "LiquidGlassWindow.h"
#include "resource.h"
#include "../JiYuTrainer/SettingHlp.h"
#include <string>

extern int screenWidth, screenHeight;

LiquidGlassWindow::LiquidGlassWindow(HWND parentHWnd) :
	CommonWindow(parentHWnd, 420, 480,
		L"JiYuTrainerLiquidGlassWindow", L"Liquid Glass 设置",
		IDR_HTML_LIQUIDGLASS)
{
	init();
	Show();
}

LiquidGlassWindow::~LiquidGlassWindow()
{
}

bool LiquidGlassWindow::on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason)
{
	return false;
}

LRESULT LiquidGlassWindow::onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled)
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

sciter::value LiquidGlassWindow::applyLiquidGlass(sciter::value opacity, sciter::value color)
{
	int op = opacity.get(0);
	std::wstring col = color.to_string();

	SettingHlp* settings = currentApp->GetSettings();
	wchar_t buf[32];
	swprintf_s(buf, L"%.2f", op / 100.0f);
	settings->SetSettingStr(L"LiquidGlassOpacity", buf);
	settings->SetSettingStr(L"LiquidGlassBgColor", col.c_str());

	return sciter::value(true);
}

sciter::value LiquidGlassWindow::docunmentComplete()
{
	CommonWindow::docunmentComplete();

	SettingHlp* settings = currentApp->GetSettings();
	std::wstring opStr = settings->GetSettingStr(L"LiquidGlassOpacity", L"0.72", 32);
	opStr.resize(wcslen(opStr.c_str()));
	float opacity = (float)_wtof(opStr.c_str());
	if (opacity < 0.3f || opacity > 1.0f) opacity = 0.72f;
	int opacityInt = (int)(opacity * 100);

	std::wstring bgColor = settings->GetSettingStr(L"LiquidGlassBgColor", L"blue", 32);
	bgColor.resize(wcslen(bgColor.c_str()));
	if (bgColor.empty()) bgColor = L"blue";

	sciter::dom::element root = get_root();
	root.call_function("initSettings", sciter::value(opacityInt), sciter::value(bgColor.c_str()));

	return sciter::value(true);
}