#pragma once

#define WM_GETVOXEL_BEGIN WM_USER+3
// GetVoxelThread

class GetVoxelThread : public CWinThread
{
	DECLARE_DYNCREATE(GetVoxelThread)

public:
	GetVoxelThread();           // 动态创建所使用的受保护的构造函数
	virtual ~GetVoxelThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	afx_msg void GetVoxel(UINT wParam, LONG lParam);

protected:
	DECLARE_MESSAGE_MAP()
};
