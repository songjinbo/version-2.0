#include "..\\StdAfx.h"
#include "Star.h"
#include <cmath>
#include <fstream> 
#include <vector>
#include "time.h"
#include <algorithm>
#include "..\\DialogDlg.h"

Star::Star(void)
{
}

Star::~Star(void)
{
} 

using namespace std;

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define MASK 123459876
#define MAX_PROB 20
#define PROB_THRESHOLD 15
#define sizeofBlock  0.4

#define flightsize 3 //仅用在路径规划这一部分
#define bigsize 100000 //仅用在路径规划这一部分

//全局变量
extern double start_and_end[6]; //传给路径规划模块,有冲突隐患
double startRealPosition[3]={-1.1,-5.5,2}; //起点坐标，用户界面输入
double endRealPosition[3] = { -2, -1, 4 };//终点坐标，用户界面输入

extern vector<double> voxel_x; //本线程的输 入变量,GetVoxelTread的输出
extern vector<double> voxel_y;
extern vector<double> voxel_z;

//本cpp文件中用到的变量
string outputfile1_3DAStar = "./data/allPoint.txt";
string outputfile2_3DAStar = "./data/drawPoint.txt";
double minValue[3]; //x y z 的最小值
double maxValue[3]; //x y z 的最大值

extern volatile ProgressStatus progress_status;
extern volatile path_plan_ret_code path_plan_status; //标志这一GetImage函数是否已经结束

//主要函数，Creategraph函数调用Find_path函数，Find_path函数调用searchchNode函数
//Creategraph函数被findpath函数调用，findpath函数又被main函数调用
bool Star::Creatgraph()
{
	int i,j,k;

	//首先给起点和终点坐标赋值
	startRealPosition[0] = start_and_end[0];
	startRealPosition[1] = start_and_end[1];
	startRealPosition[2] = start_and_end[2];
	
	endRealPosition[0] = start_and_end[3];
	endRealPosition[1] = start_and_end[4];
	endRealPosition[2] = start_and_end[5];

	//---------initialization ----------------------
	if (voxel_x.size() == 0) //在引用voxel_x[0]之前，必须用这个条件语句进行判断。否则可能出错
	{
		path_plan_status = no_path_accessible;
		return 0;
	}
	minValue[0] = voxel_x[0];
	minValue[1] = voxel_y[0];
	minValue[2] = voxel_z[0];
	maxValue[0] = voxel_x[0];
	maxValue[1] = voxel_y[0];
	maxValue[2] = voxel_z[0];
	
	int len = voxel_x.size();
	for (i = 1; i < len; i++)
	{
		if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
		{
			path_plan_status = path_plan_is_stopped;
			return 0;
		}

		if (minValue[0] > voxel_x[i])
			minValue[0] = voxel_x[i];
		if (maxValue[0] < voxel_x[i])
			maxValue[0] = voxel_x[i];

		if (minValue[1] > voxel_y[i])
			minValue[1] = voxel_y[i];
		if (maxValue[1] < voxel_y[i])
			maxValue[1] = voxel_y[i];

		if (minValue[2] > voxel_z[i])
			minValue[2] = voxel_z[i];
		if (maxValue[2] < voxel_z[i])
			maxValue[2] = voxel_z[i];
	}

	for (i=0; i<3; i++)
	{
		minValue[i] -= 3;
		maxValue[i] += 3;
	}  

	xDepth = (int)((maxValue[0] - minValue[0])/sizeofBlock + 0.5);
	yDepth = (int)((maxValue[1] - minValue[1])/sizeofBlock + 0.5);
	zDepth = (int)((maxValue[2] - minValue[2])/sizeofBlock + 0.5);

	//-----------allocation memory ---------------------
	//动态分配内存时禁止强制结束
	nodes = (Node3D ***)calloc(zDepth, sizeof(Node3D **));

	for(k=0; k<zDepth; k++)
	{
		nodes[k] = (Node3D **)calloc(yDepth, sizeof(Node3D *));
		for(i=0; i<yDepth; i++)
		{
			nodes[k][i] = (Node3D *)calloc(xDepth, sizeof(Node3D));
		}
	}

	array = (short ***)calloc(zDepth, sizeof(short **));

	for(k=0; k<zDepth; k++)
	{
		array[k] = (short **)calloc(yDepth, sizeof(short *));
		for(i=0; i<yDepth; i++)
		{
			array[k][i] = (short *)calloc(xDepth, sizeof(short));
		}
	} 

	//----------------begin-------------

	Node3D node; 

	for(i=0; i<zDepth; i++)
	{
		for(j=0; j<yDepth; j++)
		{
			for(k=0; k<xDepth; k++)
			{
				if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
				{
					path_plan_status = path_plan_is_stopped;
					for (k = 0; k<zDepth; k++)
					{

						for (i = 0; i<yDepth; i++)
						{
							delete  nodes[k][i];
						}
						delete nodes[k];
					}
					delete nodes;

					for (k = 0; k<zDepth; k++)
					{

						for (i = 0; i<yDepth; i++)
						{
							delete  array[k][i];
						}
						delete array[k];
					}
					delete array;
					return 0;
				}

				array[i][j][k] = 1;
			}
		}
	}

	//读取障碍物信息
	int tenInf[3]; //用作一个临时变量
	for (vector<double>::iterator iter_x = voxel_x.begin(), iter_y = voxel_y.begin(), iter_z = voxel_z.begin(); iter_x != voxel_x.end(); 
		iter_x++, iter_y++, iter_z++)
	{
		if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
		{
			path_plan_status = path_plan_is_stopped;
			for (k = 0; k<zDepth; k++)
			{

				for (i = 0; i<yDepth; i++)
				{
					delete  nodes[k][i];
				}
				delete nodes[k];
			}
			delete nodes;

			for (k = 0; k<zDepth; k++)
			{

				for (i = 0; i<yDepth; i++)
				{
					delete  array[k][i];
				}
				delete array[k];
			}
			delete array;
			return 0;
		}

		tenInf[0] = int((*iter_x - minValue[0]) / sizeofBlock + 0.5);
		tenInf[1] = int((*iter_y - minValue[1]) / sizeofBlock + 0.5);
		tenInf[2] = int((*iter_z - minValue[2]) / sizeofBlock + 0.5);
		array[tenInf[2]][tenInf[1]][tenInf[0]] = 0; //!!!  z y x
	}

	ofstream fs_test("F:\\test1.txt");
	for (i = 0; i < zDepth; i++)
		for (j = 0; j < yDepth; j++)
			for (k = 0; k < xDepth; k++)
			{
				if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
				{
					path_plan_status = path_plan_is_stopped;
					for (k = 0; k<zDepth; k++)
					{

						for (i = 0; i<yDepth; i++)
						{
							delete  nodes[k][i];
						}
						delete nodes[k];
					}
					delete nodes;

					for (k = 0; k<zDepth; k++)
					{

						for (i = 0; i<yDepth; i++)
						{
							delete  array[k][i];
						}
						delete array[k];
					}
					delete array;
					return 0;
				}

				fs_test << array[i][j][k]<<"\n";
			}
	fs_test.close();

	//---------------将每个点的位置等赋值--------------
 
	FILE *fp; 
	if ((fp = fopen(outputfile1_3DAStar.c_str(), "w")) != 0) //全局变量3
	{

		for (i = 0 ; i < zDepth ; i++ )
		{
			for (j  = 0; j < yDepth ; j++ )
			{
				for (k =0 ; k < xDepth ; k++ )
				{
					if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
					{
						path_plan_status = path_plan_is_stopped;
						for (k = 0; k<zDepth; k++)
						{

							for (i = 0; i<yDepth; i++)
							{
								delete  nodes[k][i];
							}
							delete nodes[k];
						}
						delete nodes;

						for (k = 0; k<zDepth; k++)
						{

							for (i = 0; i<yDepth; i++)
							{
								delete  array[k][i];
							}
							delete array[k];
						}
						delete array;
						return 0;
					}

					Point pi;
					pi.xPos = k ;
					pi.yPos = j ;
					pi.zPos = i ;				
					bool isWalkable = array[i][j][k] >= 1 ? true : false;
					node.setWalkable( isWalkable );
					if (isWalkable == false) 
					{
						fprintf(fp, "%d ", pi.xPos);
						fprintf(fp, "%d ", pi.yPos);
						fprintf(fp, "%d ", pi.zPos);
						fprintf(fp, "%d \n", 0); 
					}
					 
					node.setPoint( pi );
					nodes[i][j][k] = node; 
				}
			}
		}
		fclose(fp);
	}

	//---------------规划路线-----------------------------

	int pos[3];
	for (i=0; i<3; i++)
	{
		pos[i] = (int)((startRealPosition[i] - minValue[i])/sizeofBlock + 0.5);
	}
	startp = nodes[pos[2]][pos[1]][pos[0]];			/// 设置开始节点

	//////////////////////
	double farthp[3];
	double rowdis[bigsize];
	double farrow;
	farrow = sqrt((minValue[0] - startRealPosition[0])*(minValue[0] - startRealPosition[0]) + (minValue[1] - startRealPosition[1])*(minValue[1] - startRealPosition[1]) + (minValue[2] - startRealPosition[2])*(minValue[2] - startRealPosition[2]));   //给距离赋初值

	double tempInf[3]; //用作一个临时变量
	for (vector<double>::iterator iter_x = voxel_x.begin(), iter_y = voxel_y.begin(), iter_z = voxel_z.begin(); iter_x != voxel_x.end();
		iter_x++, iter_y++, iter_z++)
	{
		if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
		{
			path_plan_status = path_plan_is_stopped;
			for (k = 0; k<zDepth; k++)
			{

				for (i = 0; i<yDepth; i++)
				{
					delete  nodes[k][i];
				}
				delete nodes[k];
			}
			delete nodes;

			for (k = 0; k<zDepth; k++)
			{

				for (i = 0; i<yDepth; i++)
				{
					delete  array[k][i];
				}
				delete array[k];
			}
			delete array;
			return 0;
		}

		tempInf[0] = *iter_x;
		tempInf[1] = *iter_y;
		tempInf[2] = *iter_z;

		rowdis[i] = sqrt((tempInf[0] - startRealPosition[0])*(tempInf[0] - startRealPosition[0]) + (tempInf[1] - startRealPosition[1])*(tempInf[1] - startRealPosition[1]) + (tempInf[2] - startRealPosition[2])*(tempInf[2] - startRealPosition[2]));
		if (farrow <= rowdis[i])
		{
			farrow = rowdis[i];
			for (i = 0; i<3; i++)
				farthp[i] = tempInf[i];
		}
	}

	double mediump[3];
	if (minValue[0] <= (farthp[0] - flightsize) <= maxValue[0])
	{
		mediump[0] = farthp[0] - flightsize; mediump[1] = farthp[1]; mediump[2] = farthp[2];
	}
	if (minValue[0] <= (farthp[0] + flightsize) <= maxValue[0])
	{
		mediump[0] = farthp[0] + flightsize; mediump[1] = farthp[1]; mediump[2] = farthp[2];
	}
	if (minValue[1] <= (farthp[1] - flightsize) <= maxValue[1])
	{
		mediump[0] = farthp[0]; mediump[1] = farthp[1] - flightsize; mediump[2] = farthp[2];
	}
	if (minValue[1] <= (farthp[1] + flightsize) <= maxValue[1])
	{
		mediump[0] = farthp[0]; mediump[1] = farthp[1] + flightsize; mediump[2] = farthp[2];
	}

	int mediup[3];
	//double endRealPosition[3];
	for (i = 0; i<3; i++)
		if ((minValue[i] <= mediump[i] - 3 <= maxValue[i]) && (minValue[i] <= mediump[i] + 3 <= maxValue[i]))
		{
			//endRealPosition[i]=mediump[i];
			mediup[i] = int((mediump[i] - minValue[i]) / sizeofBlock + 0.5);
		}

	if (minValue[0] <= mediump[0] <= maxValue[0] && minValue[1] <= mediump[1] <= maxValue[1] && minValue[2] <= mediump[2] <= maxValue[2])
		endp = nodes[mediup[2]][mediup[1]][mediup[0]];				//重新设置开始、结束结点
	else
	{
		//	cout << "没找到终点\n" << endl;
		//返回没有找到终点的标志位
	}
	////////////////////
	Find_path( &startp);


	//---------------删除内存------------------
	for (k = 0; k<zDepth; k++)
	{

		for (i = 0; i<yDepth; i++)
		{
			delete  nodes[k][i];
		}
		delete nodes[k];
	}
	delete nodes;

	for (k = 0; k<zDepth; k++)
	{

		for (i = 0; i<yDepth; i++)
		{
			delete  array[k][i];
		}
		delete array[k];
	}
	delete array;
	return 0;
}

bool Star::Find_path(Node3D * node)
{
	//以下一段求取minVlue[i]为冗余部分，可以考虑用全局变量替代，即一个函数里调用另一个函数的变量
	int i,j,k;

	if ( *node != endp  && endp.getWalkable() )
	{
		do 
		{
			close[node] =  node->f;
			int	sz = max(node->point.zPos -1 , 0 );
			int	ez = min(node->point.zPos +1 ,zDepth-1);

			int	sy = max(node->point.yPos -1 , 0 );
			int	ey = min(node->point.yPos +1 ,yDepth-1);

			int	sx = max(node->point.xPos -1 , 0 );
			int	ex = min(node->point.xPos +1 ,xDepth-1);

			for ( int i = sz ; i <= ez ; i++ )
			{
				for ( int j  = sy ; j <= ey ; j++ )
				{
					for ( int k = sx ; k <= ex ; k++ )
					{
						if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
						{
							path_plan_status = path_plan_is_stopped;
							return 0;
						}

						if ( !( i == node->point.zPos && j== node->point.yPos && k == node->point.xPos ))
						{
							//adjNode  = nodes[i][j][k];
							searchchNode( &nodes[i][j][k], node );
						}
					}
				}
			}

			Node3D * best = NULL;
			if ( open.size() >= 1 )
			{
				best = open.begin()->first;
				open.erase( open.begin() );
			}
			node = best;
		} while ( node != NULL  && (*node != endp)) ;
	}

	if ( node != NULL  && endp.getWalkable() && (*node == endp))
	{
		int s2 = Node3D::SIZE / 2; 
		Node3D *adjNode = node;

		path.clear();
		while ( *adjNode != startp )
		{
			if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
			{
				path_plan_status = path_plan_is_stopped;
				return 0;
			}

			if ( *adjNode != startp )
			{
				adjNode->setPath( true );
				path.push_back( adjNode->point );
			}
			if ( adjNode->parent == NULL )
			{
				break ;
			}
			adjNode = (Node3D*)(adjNode->parent);
		}

		path.push_back( adjNode->point );
		FILE *fp; 
		if ((fp = fopen(outputfile2_3DAStar.c_str(), "w"))!= NULL) //全局变量5
		{
			for ( int i = path.size()-1 ; i >=0  ; --i )
			{
				if (progress_status == is_stopped) //循环检测标志位progress_status，这一步必须要有，postquitmessage()无法立即结束外层for循环
				{
					path_plan_status = path_plan_is_stopped;
					return 0;
				}

				char str[100];
				path[i].tostring(str);    

				//输出可行的路径绕过的点的坐标
				path[i].realx=(path[i].xPos-0.5)*0.4+minValue[0];
				path[i].realy=(path[i].xPos-0.5)*0.4+minValue[1];
				path[i].realz=(path[i].xPos-0.5)*0.4+minValue[2];
				fprintf(fp, "%5.2f%5.2f%5.2f\n", path[i].realx, path[i].realy, path[i].realz);
			}
			fclose(fp);
		}

		path_plan_status = path_accessible;
		return 0;
	}
	else
	{
		path_plan_status = no_path_accessible;
		return 0;
	}
	/////////////////////
}

void Star::searchchNode(Node3D * adjNode  , Node3D * current)
{
	bool xZDiagonal = ( adjNode->point.xPos != current->point.xPos && adjNode->point.zPos != current->point.zPos );
	bool xYDiagonal = ( adjNode->point.xPos != current->point.xPos && adjNode->point.yPos != current->point.yPos );
	bool yZDiagonal = ( adjNode->point.yPos != current->point.yPos && adjNode->point.zPos != current->point.zPos );

	bool corner = false;

	Node3D * tmp = current;

	/// 如果是顶点，可以抄近到  ,不需要此判断。
	if( 0 && (xZDiagonal || xYDiagonal || yZDiagonal) ) 
	{

		int xDirection = (adjNode->point.xPos - current->point.xPos);
		int yDirection = (adjNode->point.yPos - current->point.yPos);
		int zDirection = (adjNode->point.zPos - current->point.zPos);

		/// 如果是2步的走
		if ( abs( xDirection) == 2 || abs(xZDiagonal) == 2 || abs(zDirection)== 2 )
		{
			corner = true;
			int x1 = ( current->point.xPos + adjNode->point.xPos) /2 ;
			int y1 = ( current->point.yPos + adjNode->point.yPos) /2 ;
			int z1 = ( current->point.zPos + adjNode->point.zPos) /2 ;

			if ( nodes[z1][y1][x1].getWalkable() &&  !findItem(open,adjNode) && !findItem(close,adjNode) )
			{
				// corner = false;
				// tmp->parent = &nodes[z1][y1][x1];
			}
		}

		//corner = false;//( xZCorner1 || xZCorner2 || xYCorner1 || xYCorner2 || yZCorner1 || yZCorner2);
	}

	if(  adjNode->getWalkable() &&  !findItem(open,adjNode) && !findItem(close,adjNode) && !corner)
	{
		adjNode->parent = tmp;
		adjNode->g = current->g + (xZDiagonal || xYDiagonal || yZDiagonal) ? 14 : 2; //计算g 和 h  //这里是 + 源代码中是等号

		if (xYDiagonal)
			adjNode->g += 5; //add for different zPos!!!such that the UAV can fly on the same plane.

		int difX = endp.point.xPos - adjNode->point.xPos;
		int difY = endp.point.yPos - adjNode->point.yPos;
		int difZ = endp.point.zPos - adjNode->point.zPos;

		if( difX < 0 )		difX = difX * -1 ;
		if( difY < 0 )		difY = difY * -1 ;
		if( difZ < 0 )      difZ = difZ * -1 ;

		adjNode->h = (difX + difY + difZ ) * 10;
		adjNode->f = adjNode->g + adjNode->h ;

		//open[adjNode] = adjNode->f ;
		open.insert(  pair<Node3D*,int>( adjNode , adjNode->f ) );
	}
	else if  ( adjNode->getWalkable() && !findItem(close,adjNode) && !corner ) 
	{
		int g = current->g  + ( xZDiagonal || xYDiagonal || yZDiagonal ) ? 14 : 10;
		if (xYDiagonal)
			g += 5; //add for different zPos!!!such that the UAV can fly on the same plane.

		if ( g < adjNode->g )
		{
			adjNode->g = g;
			adjNode->parent = current;
		}
	}
}

