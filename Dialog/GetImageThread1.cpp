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

//extern GetVoxelThread *m_pget_voxel_thread;
extern char path[MAX_PATH];
extern CCriticalSection critical_rawdata;
extern vector<Mat> vec_depth;
extern vector<Mat> vec_left;
extern vector<Position> vec_position;

extern volatile ProgressStatus progress_status;//标志进程的运行状态，0是暂停，1是进行

volatile get_image_ret_code get_image_status = get_image_is_running; //标志这一GetImage函数是否已经结束

string itos(double i)
{
	stringstream ss;

	ss << i;

	return ss.str();
}

void GetImageThread::GetImage(UINT wParam, LONG lParam)
{
	//获取采集的帧数
	get_image_status = get_image_is_running;//每次调用此函数先得置标志位

	char str_file_count[80];
	string command = "cmd.exe /c dir " + string(path) + "| findstr \"depth[0-9]*\.xml\" | find /v /c \"\">>" + string(path) + "file_count.txt";
	WinExec(command.data(), SW_HIDE);
	fstream file_read(string(path) + "file_count.txt", ios::in);
	file_read.getline(str_file_count, 80);
	int file_count = atoi(str_file_count);
	file_read.close();
	fstream file_clear(string(path) + "file_count.txt", ios::out); //清除里面的内容
	file_clear.close();

	if (!file_count) //没有文件
	{
		get_image_status = no_file;
		::PostMessage((HWND)(GetMainWnd()->GetSafeHwnd()), WM_DISPLAY_IMAGE, get_image_status, NULL);
		return; 
	}

	int count = 0; //用来计数已经打开的文件数量
	FileStorage fs1_depth;
	FileStorage fs1_left;
	cv::Mat depth_image;
	cv::Mat left_image;
	Position position;
	bool is_depth_catch = 0,is_left_catch=0;
	//有可能陷入死循环，关键在于file_count的计数是否准确,如果file_count大于实际的帧数，那么就会陷入死循环
	int no;
	for (no = 1; count<file_count; no++)
	{
		if (progress_status==is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
		{
			get_image_status = get_image_is_stopped;
			::PostMessage((HWND)(GetMainWnd()->GetSafeHwnd()), WM_DISPLAY_IMAGE, get_image_status, NULL);
			return;
		}
		//先判断文件的格式是否正确
		is_depth_catch = is_left_catch = 0;
		try
		{	fs1_depth.open(string(path) + "depth" + itos(no) + ".xml", FileStorage::READ);	}
		catch (std::exception const& e)
		{	continue;	}

		try
		{fs1_left.open(string(path) + "left" + itos(no) + ".xml", FileStorage::READ);}
		catch (std::exception &const e)
		{	continue;	}

		if (!fs1_depth.isOpened() && !fs1_left.isOpened()) //没left和depth
		{
			fs1_left.release(); //不要忘记关闭，否则会出错误
			fs1_depth.release(); //不要忘记关闭，否则会出错误
			critical_rawdata.Unlock();
			continue;
		}
		else if (fs1_depth.isOpened() && fs1_left.isOpened())//有left和depth
		{
			count++;
			fs1_left["image"]["left"] >> left_image;
			fs1_depth["image"]["depth"] >> depth_image;
			fs1_depth["gps"]["gps_x"] >> position.x; fs1_depth["gps"]["gps_y"] >> position.y; fs1_depth["gps"]["gps_z"] >> position.z;
			fs1_depth["attitude"]["yaw"] >> position.yaw; fs1_depth["attitude"]["roll"] >> position.roll; fs1_depth["attitude"]["pitch"] >> position.pitch;
		}
		else if (fs1_depth.isOpened() && !fs1_left.isOpened())//有depth没left
		{
			count++;
			left_image = Mat(Scalar(0));
			fs1_depth["image"]["depth"] >> depth_image;
			fs1_depth["gps"]["gps_x"] >> position.x; fs1_depth["gps"]["gps_y"] >> position.y; fs1_depth["gps"]["gps_z"] >> position.z;
			fs1_depth["attitude"]["yaw"] >> position.yaw; fs1_depth["attitude"]["roll"] >> position.roll; fs1_depth["attitude"]["pitch"] >> position.pitch;
		}
		else//有left没depth
		{
			fs1_left["image"]["left"] >> left_image;
			depth_image = Mat(Scalar(0));
			fs1_left["gps"]["gps_x"] >> position.x; fs1_left["gps"]["gps_y"] >> position.y; fs1_left["gps"]["gps_z"] >> position.z;
			fs1_left["attitude"]["yaw"] >> position.yaw; fs1_left["attitude"]["roll"] >> position.roll; fs1_left["attitude"]["pitch"] >> position.pitch;
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
	::PostMessage((HWND)(GetMainWnd()->GetSafeHwnd()), WM_DISPLAY_IMAGE, get_image_status, NULL);
	
	return;//代表返回正常值
}