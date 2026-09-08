#include "stdafx.h"
#include "IMTeacherWindow.h"
#include "resource.h"
#include "../JiYuTrainer/StringHlp.h"
#include <fstream>
#include <sstream>

static std::wstring GetExeDir()
{
	WCHAR path[MAX_PATH] = {0};
	GetModuleFileNameW(NULL, path, MAX_PATH);
	std::wstring p(path);
	size_t pos = p.find_last_of(L"\\/");
	return (pos != std::wstring::npos) ? p.substr(0, pos + 1) : L".\\";
}

IMTeacherWindow::IMTeacherWindow(HWND parentHWnd)
	: CommonWindow(parentHWnd, 560, 520,
		L"JiYuTrainerIMTeacherWindow", L"极域教师端模拟 (beta)",
		IDR_HTML_IMTEACHER)
	, hStdinWrite_(NULL)
	, running_(false)
{
	ZeroMemory(&pi_, sizeof(pi_));
	logPath_ = GetExeDir() + L"teacher_sim.log";
	FileLogger::Get(L"imteacher")->Log((L"IMTeacherWindow 构造完成，日志文件: " + logPath_).c_str());
	init();
	Show();
}

IMTeacherWindow::~IMTeacherWindow()
{
	StopProcess();
}

sciter::value IMTeacherWindow::docunmentComplete()
{
	CommonWindow::docunmentComplete();
	sciter::dom::element root = get_root();
	if (!root.is_valid()) return sciter::value();

	btn_start = root.get_element_by_id(L"btn_start");
	btn_stop = root.get_element_by_id(L"btn_stop");
	btn_refresh = root.get_element_by_id(L"btn_refresh");
	btn_send = root.get_element_by_id(L"btn_send");
	btn_clear = root.get_element_by_id(L"btn_clear");
	input_cmd = root.get_element_by_id(L"input_cmd");
	text_status = root.get_element_by_id(L"text_status");
	text_students = root.get_element_by_id(L"text_students");
	text_log = root.get_element_by_id(L"text_log");

	Log(L"[系统] 窗口初始化完成");
	return sciter::value();
}

bool IMTeacherWindow::on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason)
{
	if (type != BUTTON_CLICK) return false;
	sciter::dom::element el = target;
	std::wstring id = el.get_attribute("id");

	if (id == L"btn_start") OnStart();
	else if (id == L"btn_stop") OnStop();
	else if (id == L"btn_refresh") OnRefreshStudents();
	else if (id == L"btn_send") OnSendCommand();
	else if (id == L"btn_clear") OnClearLog();
	return true;
}

LRESULT IMTeacherWindow::onWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* handled)
{
	return CommonWindow::onWndProc(hWnd, message, wParam, lParam, handled);
}

void IMTeacherWindow::OnStart()
{
	if (running_) { Log(L"[警告] 教师端已在运行"); return; }
	if (StartProcess())
	{
		running_ = true;
		text_status.set_text(L"运行中");
		Log(L"[信息] 教师端模拟已启动");
		Sleep(500);
		OnRefreshStudents();
	}
	else
	{
		Log(L"[错误] 启动失败，请检查 teacher_sim.exe 是否存在");
	}
}

void IMTeacherWindow::OnStop()
{
	StopProcess();
	running_ = false;
	text_status.set_text(L"已停止");
	Log(L"[信息] 教师端模拟已停止");
}

void IMTeacherWindow::OnSendCommand()
{
	if (!running_) { Log(L"[警告] 请先启动教师端"); return; }
	std::wstring cmd = input_cmd.get_value().to_string();
	if (cmd.empty()) { Log(L"[警告] 请输入命令"); return; }
	if (SendToProcess(cmd))
	{
		Log(L"[发送] " + cmd);
		input_cmd.set_value(sciter::value(L""));
		Sleep(300);
		OnRefreshStudents();
	}
}

void IMTeacherWindow::OnRefreshStudents()
{
	if (!running_) return;
	SendToProcess(L"list");
	Sleep(300);
	std::wstring log = ReadLogFile();
	// 简单解析学生列表
	text_students.set_text(log.c_str());
	AppendLog(log);
}

void IMTeacherWindow::OnClearLog()
{
	text_log.set_text(L"");
}

void IMTeacherWindow::Log(const std::wstring& msg)
{
	FileLogger::Get(L"imteacher")->Log(msg.c_str());
	AppendLog(msg);
}

void IMTeacherWindow::AppendLog(const std::wstring& msg)
{
	if (!text_log.is_valid()) return;
	std::wstring cur = text_log.get_value().to_string();
	cur += msg + L"\r\n";
	constexpr size_t kMaxLogCharacters = 16384;
	if (cur.size() > kMaxLogCharacters)
		cur.erase(0, cur.size() - kMaxLogCharacters);
	text_log.set_text(cur.c_str());
}

bool IMTeacherWindow::StartProcess()
{
	std::wstring exePath = GetExeDir() + L"teacher_sim.exe";
	if (GetFileAttributes(exePath.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		FileLogger::Get(L"imteacher")->LogError((L"teacher_sim.exe 不存在: " + exePath).c_str());
		return false;
	}

	SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
	HANDLE hStdinRead = NULL, hStdoutWrite = NULL;
	if (!CreatePipe(&hStdinRead, &hStdinWrite_, &sa, 0) ||
		!SetHandleInformation(hStdinWrite_, HANDLE_FLAG_INHERIT, 0))
	{
		FileLogger::Get(L"imteacher")->LogError((L"创建教师端标准输入管道失败，错误码=" + std::to_wstring(GetLastError())).c_str());
		if (hStdinRead) CloseHandle(hStdinRead);
		if (hStdinWrite_) CloseHandle(hStdinWrite_);
		hStdinWrite_ = NULL;
		return false;
	}

	STARTUPINFO si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = hStdinRead;
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.wShowWindow = SW_HIDE;

	std::wstring cmdLine = L"\"" + exePath + L"\"";
	std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
	cmdBuf.push_back(L'\0');
	std::wstring workDir = GetExeDir();
	BOOL ok = CreateProcess(NULL, cmdBuf.data(), NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, workDir.c_str(), &si, &pi_);
	CloseHandle(hStdinRead);

	if (!ok)
	{
		FileLogger::Get(L"imteacher")->LogError((L"CreateProcess 失败，错误码=" + std::to_wstring(GetLastError())).c_str());
		CloseHandle(hStdinWrite_);
		hStdinWrite_ = NULL;
		return false;
	}
	FileLogger::Get(L"imteacher")->LogInfo((L"进程已启动，PID=" + std::to_wstring(pi_.dwProcessId)).c_str());
	return true;
}

void IMTeacherWindow::StopProcess()
{
	if (pi_.hProcess)
	{
		TerminateProcess(pi_.hProcess, 0);
		WaitForSingleObject(pi_.hProcess, 2000);
		CloseHandle(pi_.hProcess);
		CloseHandle(pi_.hThread);
		ZeroMemory(&pi_, sizeof(pi_));
	}
	if (hStdinWrite_) { CloseHandle(hStdinWrite_); hStdinWrite_ = NULL; }
}

bool IMTeacherWindow::SendToProcess(const std::wstring& cmd)
{
	if (!hStdinWrite_) return false;
	std::string utf8;
	int len = WideCharToMultiByte(CP_UTF8, 0, cmd.c_str(), -1, NULL, 0, NULL, NULL);
	if (len <= 1) return false;
	utf8.resize(static_cast<size_t>(len - 1));
	if (!WideCharToMultiByte(CP_UTF8, 0, cmd.data(), static_cast<int>(cmd.size()), &utf8[0], len - 1, NULL, NULL))
		return false;
	utf8 += "\n";
	DWORD written = 0;
	BOOL ok = WriteFile(hStdinWrite_, utf8.c_str(), (DWORD)utf8.size(), &written, NULL);
	return ok == TRUE && written == utf8.size();
}

std::wstring IMTeacherWindow::ReadLogFile()
{
	std::ifstream ifs(logPath_, std::ios::binary);
	if (!ifs.is_open()) return L"";
	std::stringstream ss;
	ss << ifs.rdbuf();
	std::string utf8 = ss.str();
	if (utf8.size() > 2048) {
		size_t begin = utf8.size() - 2048;
		while (begin < utf8.size() && (static_cast<unsigned char>(utf8[begin]) & 0xC0) == 0x80)
			++begin;
		utf8 = utf8.substr(begin);
	}
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), NULL, 0);
	if (len <= 0) return L"";
	std::wstring result;
	result.resize(static_cast<size_t>(len));
	if (!MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &result[0], len))
		return L"";
	return result;
}
