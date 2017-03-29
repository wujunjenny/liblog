// liblog.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "log.h"
#include "mutex.h"
#include "ini.hpp"
#pragma comment(lib,"glog.lib")
#pragma comment(lib,"gflags.lib")
#pragma comment(lib,"shlwapi.lib")


using namespace std;


namespace log_impl{


	static string logpath;
	static string inifilename;
	static string logname_prefix;
	static bool vfile_has_path=false;
	struct v_log_info
	{
		string filename;
		int line;
		int level;
		v_log_info():line(0),level(0){};
	};

	struct pattern
	{
		string spattern;
		int level;
		pattern(const char* p,int l=0):level(l),spattern(p){};
	};

	static  Mutex mu;
	static  unordered_map<string,v_log_info> v_map;
	static  list<pattern> v_pattern;
	bool SafeFNMatch_(const char* pattern, size_t patt_len,const char* str, size_t str_len);
	int get_module_vlog(const char* key);
	
	void register_vlog(const char* file,int line,int* &regvalue)
	{ 
		MutexLock lock(&log_impl::mu);
		
		string key = log_impl::vfile_has_path ? file : ::PathFindFileNameA(file);
		key+="@";
		key+=to_string((long long)line);
		auto rt = v_map.emplace(make_pair(key,v_log_info()));
		regvalue = &rt.first->second.level;
		rt.first->second.level = get_module_vlog(key.c_str());
	};

	void set_module_vlog(const char* pattern,int vlevel)
	{
		auto itr = v_map.begin();
		for(;itr!=v_map.end();itr++)
		{
			if(SafeFNMatch_(pattern,strlen(pattern),itr->first.c_str(),itr->first.size()))
			{
				//itr->second.level=max(get_module_vlog(itr->first.c_str()),vlevel);
				itr->second.level = get_module_vlog(itr->first.c_str());
			}
		}

	};

	int get_module_vlog(const char* key)
	{
		int level =0;
		int depth = 0;
		auto itr = v_pattern.begin();
		for(;itr!=v_pattern.end();itr++)
		{

			if(SafeFNMatch_(itr->spattern.c_str(),itr->spattern.size(),key,strlen(key)))
			{
				if(itr->spattern.size()> depth)
				{
					depth = itr->spattern.size();
					level = itr->level;
				}
			}
		}
		return level;
	}


	bool SafeFNMatch_(const char* pattern, size_t patt_len,const char* str, size_t str_len) 
	{
		  size_t p = 0;
		  size_t s = 0;
		  while (1) {
			if (p == patt_len  &&  s == str_len) return true;
			if (p == patt_len) return false;
			if (s == str_len) return p+1 == patt_len  &&  pattern[p] == '*';
			if (pattern[p] == str[s]  ||  pattern[p] == '?') {
			  p += 1;
			  s += 1;
			  continue;
			}
			if (pattern[p] == '*') {
			  if (p+1 == patt_len) return true;
			  do {
				if (SafeFNMatch_(pattern+(p+1), patt_len-(p+1), str+s, str_len-s)) {
				  return true;
				}
				s += 1;
			  } while (s != str_len);
			  return false;
			}
			return false;
		  }
	}
	void clear_pattern()
	{
		MutexLock lock(&log_impl::mu);
		v_pattern.clear();
	};

};



int SetVLOGLevel(const char* pattern,int level)
{
	MutexLock lock(&log_impl::mu);
	bool bfound = false;
	for(auto itr = log_impl::v_pattern.begin();itr!=log_impl::v_pattern.end();itr++)
	{
		if(itr->spattern == pattern)
		{
			itr->level = level;
			bfound = true;
		}
	}
	if(!bfound)
	{
		log_impl::pattern p(pattern,level);
		log_impl::v_pattern.push_back(p);
	}
	log_impl::set_module_vlog(pattern,level);
	return 0;
};



bool initlogpathname(const char* name ,const char* path )
{
	using namespace google;
	static char pathname[_MAX_FNAME];
	char* prefix = nullptr;
	if(path == nullptr)
	{
		::GetModuleFileNameA(::GetModuleHandleA(nullptr),pathname,_MAX_FNAME);
	}
	else
	{
		strncpy_s(pathname,path,_MAX_FNAME-1);
	}
	::PathRemoveFileSpecA(pathname);

	FLAGS_log_dir = pathname;

	if(name==nullptr)
	{
		::GetModuleFileNameA(::GetModuleHandleA(nullptr),pathname,_MAX_FNAME);
		prefix = ::PathFindFileNameA(pathname);

	}
	else
	{
		prefix = ::PathFindFileNameA(name);
	}
	
	if(prefix==nullptr)
		google::InitGoogleLogging("errofilename");
	else
		google::InitGoogleLogging(prefix);

	return true;
}


bool initlog(const char* ini_fname)
{
	log_impl::inifilename = ini_fname? ini_fname:"log.ini";
	char pathname[_MAX_FNAME];
	::GetModuleFileNameA(::GetModuleHandleA(nullptr),pathname,_MAX_FNAME);
	log_impl::logname_prefix = ::PathFindFileNameA(pathname);
	::PathRemoveExtensionA(&log_impl::logname_prefix[0]);
	log_impl::logname_prefix = log_impl::logname_prefix.c_str();
	try
	{
		INI::Parser parser(log_impl::inifilename.c_str());

		auto global = parser.top()("global");
		log_impl::logpath = global["log_dir"];
		if(log_impl::logpath.size())
		{
			if(::PathIsRelativeA(log_impl::logpath.c_str()))
			{
				::PathRemoveFileSpecA(pathname);
				::PathAppendA(pathname,log_impl::logpath.c_str());
				log_impl::logpath = pathname;
			}
		}
		else
		{
			::PathRemoveFileSpecA(pathname);
			//::PathRemoveExtensionA();
			log_impl::logpath = pathname;
		}
		LOG(INFO) << "log_dir =[" <<log_impl::logpath<<"]";

		FLAGS_log_dir = log_impl::logpath;

		if(global["log_name_prefix"].size())
			log_impl::logname_prefix = global["log_name_prefix"];

		LOG(INFO) << "log_name_prefix =[" <<log_impl::logname_prefix<<"]";		
		
		google::InitGoogleLogging(&log_impl::logname_prefix[0]);
		
		printf("google::InitGoogleLogging\n");
		LOG(INFO) << "InitGoogleLogging";

		if(global["alsologtostderr"]=="true")
		{
			FLAGS_alsologtostderr = true;
			LOG(INFO) << "alsologtostderr =[" <<FLAGS_alsologtostderr<<"]";		
		}
		else
		{
			FLAGS_alsologtostderr = false;
			LOG(INFO) << "alsologtostderr =[" <<FLAGS_alsologtostderr<<"]";		
		}

		if(global["colorlogtostderr"]=="true")
		{
			FLAGS_colorlogtostderr = true;
			LOG(INFO) << "colorlogtostderr =[" <<FLAGS_colorlogtostderr<<"]";		
		}
		else
		{
			FLAGS_alsologtostderr = false;
			LOG(INFO) << "colorlogtostderr =[" <<FLAGS_colorlogtostderr<<"]";		
		}
		if(global["vfile_path"]=="true")
		{
			log_impl::vfile_has_path = true;
		}

		auto vmodule = parser.top()("vmodule");

		log_impl::clear_pattern();

		for( auto itr = vmodule.ordered_values.begin();itr!=vmodule.ordered_values.end();itr++)
		{
			auto pattern = (*itr)->first;
			auto level = (*itr)->second;
			SetVLOGLevel(pattern.c_str(),atoi(level.c_str()));
			LOG(INFO) << "Set V  [" <<pattern <<"]=["<<level<<"]";		
		}

	}
	catch(std::runtime_error &e)
	{
		LOG(ERROR) << "Load log ini error :"<<e.what();
		return false;
	}
	return true;
}


void flushlog()
{
	google::FlushLogFiles(google::GLOG_INFO);
}

void vreginfodump()
{
	MutexLock lock(&log_impl::mu);
	stringstream s;
	auto itr = log_impl::v_map.begin();
	for(;itr!= log_impl::v_map.end();itr++)
	{
		s<<"["<<itr->first<<"="<<itr->second.level<<"]";
	}
	LOG(INFO)<<"all register vlog "<<s.str();
}

void reloadcfg()
{
	try
	{
		INI::Parser parser(log_impl::inifilename.c_str());

		auto global = parser.top()("global");

		LOG(INFO) << "start reset log option";
		if(global["alsologtostderr"]=="true")
		{
			FLAGS_alsologtostderr = true;
			LOG(INFO) << "alsologtostderr =[" <<FLAGS_alsologtostderr<<"]";		
		}
		else
		{
			FLAGS_alsologtostderr = false;
			LOG(INFO) << "alsologtostderr =[" <<FLAGS_alsologtostderr<<"]";		
		}

		if(global["colorlogtostderr"]=="true")
		{
			FLAGS_colorlogtostderr = true;
			LOG(INFO) << "colorlogtostderr =[" <<FLAGS_colorlogtostderr<<"]";		
		}
		else
		{
			FLAGS_alsologtostderr = false;
			LOG(INFO) << "colorlogtostderr =[" <<FLAGS_colorlogtostderr<<"]";		
		}

		if(global["vfile_path"]=="true")
		{
			log_impl::vfile_has_path = true;
			LOG(INFO)<<"vfile without no path";
		}
		else
		{
			LOG(INFO)<<"vfile has path";
		}

		auto vmodule = parser.top()("vmodule");

		
		log_impl::clear_pattern();
		LOG(INFO)<<"clear old pattern.";

		for( auto itr = vmodule.ordered_values.begin();itr!=vmodule.ordered_values.end();itr++)
		{
			auto pattern = (*itr)->first;
			auto level = (*itr)->second;
			SetVLOGLevel(pattern.c_str(),atoi(level.c_str()));
			LOG(INFO) << "Set V  [" <<pattern <<"]=["<<level<<"]";		
		}

	}
	catch(std::runtime_error &e)
	{
		LOG(ERROR) << "Load log ini error :"<<e.what();
	}
}


#include "dbghelp.h" 
#pragma comment(lib, "dbghelp.lib")


void StackTrace(int deep)
{
    char Buff[sizeof(IMAGEHLP_SYMBOL64)+1024] = {0};  
    unsigned int   i;  
    void         * stack[128];  
    unsigned short frames;  
    IMAGEHLP_SYMBOL64  * symbol;  
    HANDLE         process;  
	std::stringstream ret;
    process = GetCurrentProcess();    
    SymInitialize(process, NULL, TRUE);  
 
    frames = CaptureStackBackTrace(1, 128, stack, NULL);  
    symbol = (IMAGEHLP_SYMBOL64 *)Buff;  
	symbol->MaxNameLength = 1024;  
    symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);  
  
    for (i = 0; i < min((int)frames,deep); i++)  
    {  
		HMODULE	hModule = NULL;  
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (PCTSTR)(DWORD64)(stack[i]), &hModule);  

		ret << std::dec <<i <<": ";
		if(SymGetSymFromAddr64(process, (DWORD64)(stack[i]), 0, symbol))
			ret << symbol->Name;
		else
			ret <<"N/A";
		ret << " - " << std::hex <<std::showbase <<stack[i]<< "-"<<hModule<<"\t";
		
		IMAGEHLP_LINE64 lineInfo;
		memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE64));
		lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD dwLineDisplacement;
		if(SymGetLineFromAddr64(process, (DWORD64)(stack[i]), &dwLineDisplacement, &lineInfo))
		{
			ret << "filename: [" << lineInfo.FileName <<"] line:["<<std::dec<<lineInfo.LineNumber <<"] ";
		}
		else
		{
			ret << "filename: [N/A] line:[N/A] ";
		}

		IMAGEHLP_MODULE64 moduleInfo;
		memset(&moduleInfo, 0, sizeof(IMAGEHLP_MODULE64));

		moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

		// 得到模块名
		//
		if (SymGetModuleInfo64(process, (DWORD64)(stack[i]), &moduleInfo))
		{
			ret << "module:"<< moduleInfo.ModuleName;
		}
		ret << std::endl;
    }
	LOG(INFO) << "stack trace :\n"<< ret.str();
}