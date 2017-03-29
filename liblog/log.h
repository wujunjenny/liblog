#pragma once

#if defined (_WINDOWS_) || defined (_WINDOWS) || defined (WIN32) || defined (WIN64)
	#ifndef GLOG_NO_ABBREVIATED_SEVERITIES
	#define GLOG_NO_ABBREVIATED_SEVERITIES
	#endif
#endif

#include <gflags\gflags.h>
#include <glog\logging.h>
#include <Shlwapi.h>



#ifndef LOG_DLL_DECL
# define LOG_API_DECL  __declspec(dllimport)
#else
# define LOG_API_DECL  __declspec(dllexport)
#endif



#ifdef VLOG_IS_ON
#undef VLOG_IS_ON
#endif

#include <string>
#include <unordered_map>
#include <list>

namespace log_impl{


	int get_module_vlog(const char* key);	
	void LOG_API_DECL register_vlog(const char* file,int line,int* &regvalue);
	void set_module_vlog(const char* pattern,int vlevel);
	int get_module_vlog(const char* key);
	bool LOG_API_DECL SafeFNMatch_(const char* pattern, size_t patt_len,const char* str, size_t str_len);
}


bool LOG_API_DECL initlogpathname(const char* name = nullptr,const char* path = nullptr);

bool LOG_API_DECL initlog(const char* ini_fname = nullptr);
LOG_API_DECL void flushlog();
LOG_API_DECL void vreginfodump();
int LOG_API_DECL SetVLOGLevel(const char* pattern,int level);
LOG_API_DECL void StackTrace(int deep);
LOG_API_DECL void reloadcfg();
#define VLOG_IS_ON(verboselevel) \
	([](const char* file,int line,int level)->bool{	\
		static int* p_level = nullptr;		\
		if(p_level==nullptr)				\
		{ log_impl::register_vlog(file,line,p_level);	\
		}									\
		return *p_level >= level;}			\
	)(__FILE__,__LINE__,verboselevel)


