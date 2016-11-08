
// DialogDlg.h : 头文件
//

#pragma once

//#include "afxwin.h"
#include "GetImageThread.h"
#include "GetVoxelThread.h"
#include "PathPlanThread.h"
//#include "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\atlmfc\include\afxwin.h"
#include "dialog_opengl.h"
#include "Resource.h"


#define WM_DISPLAY_IMAGE WM_USER+2
#define WM_UPDATE_STATUS WM_USER+5

// CDialogDlg 对话框
class CDialogDlg : public CDialogEx
{
// 构造
public:
	CDialogDlg(CWnd* pParent = NULL);	// 标准构造函数
// 对话框数据
	enum { IDD = IDD_DIALOG_DIALOG };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStart();
	double m_dx;
	double m_dy;
	double m_dz;
	double m_dyaw;
	double m_dpitch;
	double m_droll;
protected:
	//创建线程需要用到
	HANDLE hThread_get_image;
	DWORD ThreadID_get_image;
	void InitWindow(CStatic *, CStatic *, CStatic *);
	void InitVariable();
	LRESULT DisplayImage(WPARAM wParam, LPARAM lParam);
	LRESULT UpdateStatus(WPARAM wParam, LPARAM lParam);

public:
	CStatic m_DisplayLeft;
	CStatic m_DisplayDepth;
	afx_msg void OnBnClickedBrowse();
	void InitThread();
	GetImageThread* m_pget_image_thread;
	GetVoxelThread *m_pget_voxel_thread;
	PathPlanThread *m_ppath_plan_thread;
	double m_dendx;
	double m_dendy;
	double m_dendz;
	double m_dstartx;
	double m_dstarty;
	double m_dstartz;
	CStatic m_DisplayMap;
	dialog_opengl map_window;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
private:
	CFont titleFont;
	CFont groupFont;
	CFont poseFont;
};

enum ProgressStatus
{
	//主线程的状态标志位
	is_ruuning = 1,
	is_stopped,
	complete,

	//GetImage函数的状态标志位
	get_image_is_running,
	no_file,
	get_image_is_stopped,
	get_image_complete,

	//GetVoxel函数的状态标志位
	get_voxel_is_running,
	no_data_in_queue,
	get_one_voxel, //处理完一帧的传感器数据
	get_voxel_is_stopped,
	get_all_voxel_complete,

	//PathPlan函数的状态标志位
	path_plan_is_running,
	path_plan_is_stopped,
	no_voxel_in_queue,
	subpath_accessible, //找到一段子路径的标志
	path_accessible,
	no_path_accessible
};

typedef ProgressStatus get_image_ret_code;
typedef ProgressStatus get_voxel_ret_code;
typedef ProgressStatus path_plan_ret_code;