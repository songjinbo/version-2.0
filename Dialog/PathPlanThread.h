#pragma once


#define WM_PATHPLAN_BEGIN WM_USER +4
// PathPlanThread

class PathPlanThread : public CWinThread
{
	DECLARE_DYNCREATE(PathPlanThread)

protected:
	PathPlanThread();           // 动态创建所使用的受保护的构造函数
	virtual ~PathPlanThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void PathPlan(UINT wParam, LONG lParam);
};


