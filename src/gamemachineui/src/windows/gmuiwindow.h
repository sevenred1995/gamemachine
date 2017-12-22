﻿#ifndef __GMUIWINDOW_H__
#define __GMUIWINDOW_H__
#include <gmuicommon.h>
#include "gmuidef.h"
#include <gamemachine.h>

BEGIN_UI_NS

GM_PRIVATE_OBJECT(GMUIWindow)
{
	gm::IInput* input = nullptr;
};

class GMUIWindow : public gm::GMObject, public gm::IWindow, private GMUIWindowBase
{
	DECLARE_PRIVATE_AND_BASE(GMUIWindow, GMUIWindowBase)

public:
	~GMUIWindow();

public:
	operator gm::GMWindowHandle() const { return getWindowHandle(); }

	// IWindow
public:
	virtual gm::IInput* getInputMananger() override;
	virtual void update() override;
	virtual gm::GMWindowHandle create(const gm::GMWindowAttributes& attrs) override;
	virtual gm::GMRect getWindowRect() override;
	virtual gm::GMRect getClientRect() override;
	virtual void showWindow() override { showWindowEx(); }
	virtual void centerWindow() override { return Base::CenterWindow(); }
	virtual gm::GMWindowHandle getWindowHandle() const override { return Base::GetHWND(); }
	virtual bool handleMessage() override { return DuiLib::CPaintManagerUI::HandleMessage(); }
	virtual bool event(const gm::GameMachineMessage& msg) override { return false; }
	virtual bool isWindowActivate() override;
	virtual void setLockWindow(bool lock) override;

public:
	virtual LongResult handleMessage(gm::GMuint uMsg, UintPtr wParam, LongPtr lParam);
	virtual gm::GMuint showModal() { return Base::ShowModal(); }
	virtual void showWindowEx(bool show = true, bool takeFocus = true) { Base::ShowWindow(show, takeFocus); }
	virtual void onFinalMessage(gm::GMWindowHandle wndHandle) {}

private:
	virtual GMUIStringPtr getWindowClassName() const = 0;
	virtual gm::GMuint getClassStyle() const { return 0; }

	// From base:
protected:
	virtual LPCTSTR GetWindowClassName() const override { return getWindowClassName(); }
	virtual UINT GetClassStyle() const override { return getClassStyle(); }
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override { return handleMessage(uMsg, wParam, lParam); }
	virtual void OnFinalMessage(HWND hWnd) override { onFinalMessage(hWnd); }
};

END_UI_NS

#endif