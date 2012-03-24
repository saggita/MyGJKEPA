#ifndef PROFILER_H
#define PROFILER_H

#include <Common/Math/Array.h>
#include <stdio.h>

class Profiler
{
	public:
		__inline
		Profiler();

		__inline
		void pushBack();
		__inline
		void popBack();
		__inline
		void addData(const char* name, float ms);
		__inline
		void clear();
		__inline
		void printf();
		__inline
		void printf(Array<char*>& txtArray);

	public:
		struct Data
		{
			int m_level;
			char m_name[128];
			float m_ms;
		};

		Array<Data> m_data;
		int m_currentLevel;
};

Profiler::Profiler()
: m_currentLevel(0)
{

}

void Profiler::pushBack()
{
	m_currentLevel ++;
}

void Profiler::popBack()
{
	ADLASSERT( m_currentLevel >= 1 );
	m_currentLevel --;
}

void Profiler::addData(const char* name, float ms)
{
	Data& data = m_data.expandOne();
	data.m_level = m_currentLevel;
	memcpy( data.m_name, name, 128 );
	data.m_ms = ms;
}

void Profiler::clear()
{
	m_data.clear();
}

void Profiler::printf()
{
	for(int i=0; i<48; i++) ::printf("-");
	::printf("\n");

	float totalTime = 0.f;
	for(int i=0; i<m_data.getSize(); i++)
	{
		totalTime += m_data[i].m_ms;
	}
	for(int i=0; i<m_data.getSize(); i++)
	{
		char txt[512];
		sprintf_s(txt,512,">> ");
		for(int j=0; j<m_data[i].m_level; j++) sprintf_s(txt+strlen(txt),512-strlen(txt), "  ");
		sprintf_s(txt+strlen(txt),512-strlen(txt), "%s	: %3.2f", m_data[i].m_name, m_data[i].m_ms);

		::printf("%s\n", txt);
	}
	for(int i=0; i<48; i++) ::printf("-");
	::printf("\n");
}

void Profiler::printf(Array<char*>& txtArray)
{
	float totalTime = 0.f;
	for(int i=0; i<m_data.getSize(); i++)
	{
		totalTime += m_data[i].m_ms;
	}

	{
		char* txt = txtArray.expandOne();
		sprintf_s(txt,512,">> PROFILER OUTPUT [%3.2fms]\n", totalTime);
	}

	for(int i=0; i<m_data.getSize(); i++)
	{
		char* txt = txtArray.expandOne();
		sprintf_s(txt,512,">> ");
		for(int j=0; j<m_data[i].m_level; j++) sprintf_s(txt+strlen(txt),512, "  ");
		sprintf_s(txt+strlen(txt),512, "%s :%3.1f%% (%f)", m_data[i].m_name, m_data[i].m_ms*100.f/totalTime, m_data[i].m_ms);
	}
}

#endif

