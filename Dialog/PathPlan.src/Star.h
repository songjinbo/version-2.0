#pragma once
#include <map>
#include <vector>
#include "Node3D.h"
#include "..\\DialogDlg.h"

using namespace std;

/// 排序
class MyNodeSort
{
public:
	bool  operator() ( const Node3D * node1 , const Node3D * node2 ) const
	{
		if ( node1 != node2 )
		{
			if ( node1->f == node2->f )
			{	if ( node2->h == node2->h)
			if ( node1->g == node2->g)
			{
				return node1 < node2 ;
			}
			return node1->g < node2->g;
			return node1->h < node2->h;
			}	
			return ( node1->f < node2->f ) ;
		}
		else if ( node1->f > node2->f)
		{
			return true;
		}
		return false;
		
	}
};

class Star
{
public:



	int  xDepth;
    int  yDepth;
    int  zDepth;
	Node3D  ***nodes;
   short	***array;
	Star(void);
	~Star(void);
	
	void  Init();
	void  Display();
	bool  Creatgraph();
	double RandomGenerate(long * idum);

	bool  findpath() 
	{
		return Creatgraph(); 
	}
	bool  Find_path(Node3D * node );
	bool  Path_smooth();
	void  searchchNode(Node3D * adjNode , Node3D * current);
	void  animate();
	long  get_h_value(int p);
	long  get_g_value(int p);

	inline bool findItem( map<Node3D*,int,MyNodeSort> &table , Node3D * node)
	{
		typedef map<Node3D*,int,MyNodeSort>::iterator MAPNODE;
		for ( MAPNODE iter = table.begin() ; iter != table.end() ; iter ++ )
		{
			if ( node->point == iter->first->point )
			{
				return true ;
			}
		}
		return false;
	}
private:

	/// open 和 close表
	map<Node3D*,int,MyNodeSort> open;
	map<Node3D*,int,MyNodeSort> close;
	vector<Point>   path;

	/// 开始 和 结束 坐标
	Node3D       startp;
	Node3D		 endp; 

	Node3D		 GPoint;
};
