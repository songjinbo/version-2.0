#pragma once


//#include "afxmt.h"
//#include <vector>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <iostream>
//#include "GetVoxelThread.h"


#define WM_GETIMAGE_BEGIN WM_USER+1


// GetImageThread

class GetImageThread : public CWinThread
{
	DECLARE_DYNCREATE(GetImageThread)

public:
	GetImageThread();           // 动态创建所使用的受保护的构造函数
	virtual ~GetImageThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void GetImage(UINT wParam, LONG lParam);
};


//存储位置和姿态信息
class Position
{
public:
	double x;
	double y;
	double z;
	double roll;
	double pitch;
	double yaw;
};
