#include "StdAfx.h"
#include "Node3D.h"

Node3D::Node3D(void)
{
	f = g =h = 0;
	walkable = true;
	start =  destinantion = path = 0;
	parent = NULL;
}

Node3D::~Node3D(void)
{
}
