#include "stdafx.h"
#include "ScreenshotWindow.h"
#include "resource.h"
#include "sciter-x-graphics.hpp"
#include "../JiYuTrainer/SysHlp.h"
#include "../JiYuTrainer/StringHlp.h"
#include "../JiYuTrainer/SettingHlp.h"
#include <string>
#include <algorithm>
#include <fstream>
#include <vector>
#include <wincrypt.h>
#include <cctype>
#include <cwctype>
#pragma comment(lib, "crypt32.lib")

extern int screenWidth, screenHeight;

ScreenshotWindow::ScreenshotWindow(HWND parentHWnd) :
	CommonWindow(parentHWnd, 480, 480,
		L"JiYuTrainerScreenshotWindow", L"截图替换",
		IDR_HTML_SCREENSHOT)
{
	init();
	Show();
}

ScreenshotWindow::~ScreenshotWindow()
{
}

bool ScreenshotWindow::on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason)
{
	sciter::dom::element ele(he);
	if (type == HYPERLINK_CLICK || type == BUTTON_CLICK)
	{
		sciter::string id = ele.get_attribute("id");
		if (id == L"btn_choose")
			OnChooseImage();
		else if (id == L"btn_clear")
			OnClearImage();
		else if (id == L"btn_apply")
			OnApply();
		else if (id == L"btn_cancel")
			OnCancel();
	}
	return false;
}

LRESULT ScreenshotWindow::onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled)
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

void ScreenshotWindow::OnChooseImage()
{
	TCHAR strFilename[MAX_PATH] = { 0 };
	if (SysHlp::ChooseFileSingal(_hWnd, NULL, L"请选择用于替换屏幕截图的图片",
		L"图片文件(*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0所有文件(*.*)\0*.*\0\0\0",
		strFilename, NULL, strFilename, MAX_PATH))
	{
		currentImagePath = strFilename;
		// 应用时才保存，此处不立即写入 settings
		// 应用时才保存，此处不立即写入 settings
		RefreshUI();
		text_status.set_text(L"已设置截图替换图片，教师端监控时将显示此图片。");
	}
}

void ScreenshotWindow::OnClearImage()
{
	currentImagePath = L"";
	RefreshUI();
	text_status.set_text(L"已清除截图替换，点击应用后生效。");
}


void ScreenshotWindow::OnApply()
{
	SettingHlp* settings = currentApp->GetSettings();
	settings->SetSettingStr(L"FakeScreenImage", currentImagePath);
	if (currentImagePath.empty())
		text_status.set_text(L"已清除截图替换。");
	else
		text_status.set_text(L"已应用截图替换设置。");
	::PostMessage(_hWnd, WM_CLOSE, 0, 0);
}

void ScreenshotWindow::OnCancel()
{
	::PostMessage(_hWnd, WM_CLOSE, 0, 0);
}

void ScreenshotWindow::RefreshUI()
{
	sciter::dom::element root = get_root();
	sciter::dom::element img_preview = root.get_element_by_id(L"img_preview");
	if (currentImagePath.empty())
	{
		text_path.set_text(L"（未设置）");
		btn_clear.set_attribute("style", L"display: none;");
		if (img_preview.is_valid()) img_preview.set_style_attribute("display", L"none");
	}
	else
	{
		text_path.set_text(currentImagePath.c_str());
		btn_clear.set_attribute("style", L"");
		if (img_preview.is_valid()) {
			// 读取图片文件并转 base64 data URL，绕过 Sciter 本地文件访问限制
			FILE* fp = NULL;
			if (_wfopen_s(&fp, currentImagePath.c_str(), L"rb") == 0 && fp) {
				fseek(fp, 0, SEEK_END);
				size_t fsize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				std::vector<BYTE> fbuf(fsize);
				fread(fbuf.data(), 1, fsize, fp);
				fclose(fp);
				// 判断 MIME 类型
				std::wstring mime = L"image/png";
				std::wstring ext = currentImagePath.substr(currentImagePath.find_last_of(L".") + 1);
				for (auto& ch : ext) ch = towlower(ch);
				if (ext == L"jpg" || ext == L"jpeg") mime = L"image/jpeg";
				else if (ext == L"bmp") mime = L"image/bmp";
				// base64 编码
				DWORD b64len = 0;
				CryptBinaryToString(fbuf.data(), (DWORD)fsize, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &b64len);
				std::wstring b64(b64len, 0);
				CryptBinaryToString(fbuf.data(), (DWORD)fsize, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &b64[0], &b64len);
				std::wstring dataUrl = L"data:" + mime + L";base64," + b64;
				img_preview.set_attribute("src", dataUrl.c_str());
				img_preview.set_style_attribute("display", L"block");
			} else {
				text_status.set_text(L"无法读取图片文件。");
			}
		}
}
}

sciter::value ScreenshotWindow::docunmentComplete()
{
	CommonWindow::docunmentComplete();

	sciter::dom::element root = get_root();
	btn_choose = root.get_element_by_id(L"btn_choose");
	btn_clear = root.get_element_by_id(L"btn_clear");
	text_path = root.get_element_by_id(L"text_path");
	text_status = root.get_element_by_id(L"text_status");

	SettingHlp* settings = currentApp->GetSettings();
	currentImagePath = settings->GetSettingStr(L"FakeScreenImage", L"", 512);
	currentImagePath.resize(wcslen(currentImagePath.c_str()));
	RefreshUI();

	if (!currentImagePath.empty())
		text_status.set_text(L"当前已设置截图替换图片。");
	else
		text_status.set_text(L"尚未设置截图替换图片。");

	return sciter::value(true);
}
