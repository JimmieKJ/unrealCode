// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSlateD3DTextureManager;
class FSlateD3D11RenderingPolicy;



class GetTime
{
private:
	GetTime() {}
public:
	~GetTime() {}

static 	GetTime* GetGlobalInstance()
	{
		if (NULL==g_GetTime)
		{
			g_GetTime = new GetTime();
			return g_GetTime;
		}
		else
		{
			return g_GetTime;
		}
	}
public:
	 double getExactTime()
	{
		return mTime;
	}

	void  SetExactTime()
	{

	}
	static GetTime*  g_GetTime;
private:
	double mTime;
};