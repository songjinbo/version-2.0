// GetImageThread.cpp : 实现文件
//
#include "stdafx.h"
#include <string>
#include <iostream>
#include <fstream>
#include <queue>
#include <tchar.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include "Dialog.h"
#include "DialogDlg.h"
#include "GetImageThread.h"
#include "GetVoxelThread.h"


IMPLEMENT_DYNCREATE(GetImageThread, CWinThread)

GetImageThread::GetImageThread()
{
}

GetImageThread::~GetImageThread()
{
}

BOOL GetImageThread::InitInstance()
{
	// TODO:    在此执行任意逐线程初始化
	return TRUE;
}

int GetImageThread::ExitInstance()
{
	// TODO:    在此执行任意逐线程清理
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(GetImageThread, CWinThread)
	ON_THREAD_MESSAGE(WM_GETIMAGE_BEGIN, GetImage)
END_MESSAGE_MAP()


// GetImageThread 消息处理程序

using namespace cv;
using namespace std;

extern char path[MAX_PATH];
extern CCriticalSection critical_rawdata;
extern vector<Mat> vec_depth;
extern vector<Mat> vec_left;
extern vector<Position> vec_position;


extern volatile ProgressStatus progress_status;//标志进程的运行状态，0是暂停，1是进行

extern int file_count;
extern int count_opened; //用来计数已经打开的文件数量
extern int no;

volatile get_image_ret_code get_image_status = get_image_is_running; //标志这一GetImage函数是否已经结束

string itos(double i)
{
	stringstream ss;

	ss << i;

	return ss.str();
}

//string转化成LPCWSTR类型
LPCWSTR stringToLPCWSTR(std::string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t) *(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
	return wcstring;
}

void GetImageThread::GetImage(UINT wParam, LONG lParam)
{
	get_image_status = get_image_is_running;//每次调用此函数先得置标志位

	if (!file_count) //没有文件
	{
		get_image_status = no_file;
		::PostMessage((HWND)(GetMainWnd()->GetSafeHwnd()), WM_UPDATE_STATUS, get_image_status, NULL);
		return; 
	}

	FileStorage fs1_depth;
	FileStorage fs1_left;

	//程序临时变量
	cv::Mat depth_image;
	cv::Mat left_image;
	Position position;

	while(count_opened<file_count)
	{
		if (progress_status==is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
		{
			get_image_status = get_image_is_stopped;
			::PostMessage((HWND)(GetMainWnd()->GetSafeHwnd()), WM_UPDATE_STATUS, get_image_status, NULL);
			return;
		}
		//先判断文件的格式是否正确
		try
		{	fs1_depth.open(string(path) + "depth" + itos(no) + ".xml", FileStorage::READ);	}
		catch (std::exception const& e) //当文件存在且格式不正确的时候才会发生异常，此时fs1_depth.isOpened == false
		{
			count_opened++; //这里不要遗忘count_opended++
			no++;
			continue;
		}

		try
		{fs1_left.open(string(path) + "left" + itos(no) + ".xml", FileStorage::READ);}
		catch (std::exception &const e)
		{
		}

		if (!fs1_depth.isOpened()) //没depth
		{
			fs1_left.release(); //不要忘记关闭，否则会出错误
			fs1_depth.release(); //不要忘记关闭，否则会出错误
			no++;
			continue;
		}
		else if (fs1_depth.isOpened() && fs1_left.isOpened())//left和depth存在且格式都正确
		{
			count_opened++;
			no++;
			fs1_left["image"]["left"] >> left_image;
			fs1_depth["image"]["depth"] >> depth_image;
			fs1_depth["gps"]["gps_x"] >> position.x; fs1_depth["gps"]["gps_y"] >> position.y; fs1_depth["gps"]["gps_z"] >> position.z;
			fs1_depth["attitude"]["yaw"] >> position.yaw; fs1_depth["attitude"]["roll"] >> position.roll; fs1_depth["attitude"]["pitch"] >> position.pitch;
		}
		else//有depth没left
		{
			count_opened++;
			no++;
			left_image = Mat(Scalar(0));
			fs1_depth["image"]["depth"] >> depth_image;
			fs1_depth["gps"]["gps_x"] >> position.x; fs1_depth["gps"]["gps_y"] >> position.y; fs1_depth["gps"]["gps_z"] >> position.z;
			fs1_depth["attitude"]["yaw"] >> position.yaw; fs1_depth["attitude"]["roll"] >> position.roll; fs1_depth["attitude"]["pitch"] >> position.pitch;
		}
		//将读取的数据存到容器中
		critical_rawdata.Lock();
		vec_depth.push_back(depth_image.clone());
		vec_left.push_back(left_image.clone());
		vec_position.push_back(position);
		critical_rawdata.Unlock();

		fs1_depth.release();
		fs1_left.release();
	}

	get_image_status = get_image_complete;
	::PostMessage((HWND)(GetMainWnd()->GetSafeHwnd()), WM_UPDATE_STATUS, get_image_status, NULL);
	return;//代表返回正常值
}