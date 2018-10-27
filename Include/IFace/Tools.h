///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_IFACE_TOOLS_H_
#define INCLUDED_IFACE_TOOLS_H_

#include <WBFLCore.h>
#include <AgentTools.h>
#include <string>

#define VALIDATE_AND_CHECK_TO_LEVEL(lvl,bfn,cfn) \
if ( level >= lvl && m_Level < lvl ) \
{ \
   if ( !##bfn() ) \
      return lvl-1; \
\
   m_Level = lvl; \
\
   cfn(); \
}

#define VALIDATE_TO_LEVEL(lvl,fn) \
if ( level >= lvl && m_Level < lvl ) \
{ \
   if ( !##fn() ) \
      return lvl-1; \
\
   m_Level = lvl; \
}

#define VALIDATE_TO_LEVEL_EX(lvl,fn,progress) \
if ( level >= lvl && m_Level < lvl ) \
{ \
   if ( !##fn((progress)) ) \
      return lvl-1; \
\
   m_Level = lvl; \
}

#if defined ENABLE_LOGGING

#if !defined INCLUDED_SYSTEM_LOGDUMPCONTEXT_H_
#include <System\LogDumpContext.h>
#endif

#define DECLARE_LOGFILE \
dbgLogDumpContext m_Log;

#else

#define DECLARE_LOGFILE

#endif 


#if defined ENABLE_LOGGING

#define CREATE_LOGFILE(name) \
   ILogFile* __pLogFile__; \
   DWORD __dwCookie__; \
   HRESULT _xxHRxx_ = ::CoCreateInstance( CLSID_SysAgent, 0, CLSCTX_INPROC_SERVER,IID_ILogFile,(void**)&__pLogFile__); \
   ATLASSERT(SUCCEEDED(_xxHRxx_)); \
   __pLogFile__->Open( std::_tstring(std::_tstring(_T(##name))+std::_tstring(_T(".log"))).c_str(), &__dwCookie__ ); \
   m_Log.SetLog( __pLogFile__, __dwCookie__ ); \
   __pLogFile__->Release(); \
   __pLogFile__ = 0; \
   LOG(std::_tstring(_T(##name))+std::_tstring(_T(" Log Opened")))

#define LOG(x) m_Log << x << endl
#define LOGX(x) m_Log << _T(__FILE__) << _T(" ") << _T("(") << __LINE__ << _T(") ") << x << endl

#define LOG_EXECUTION_TIME(_x_) \
   { sysTime startTime; \
   _x_; \
   sysTime endTime; \
   LOG((endTime.Seconds() - startTime.Seconds()) << _T(" sec : ") << _T(#_x_)); }

#define CLOSE_LOGFILE m_Log.SetLog(nullptr,0)

#else

#define CREATE_LOGFILE(name) 
#define LOG(x)
#define LOGX(x)
#define CLOSE_LOGFILE
#define LOG_EXECUTION_TIME(_x_) _x_

#endif


#if defined ENABLE_LOGGING

#define SHARED_LOGFILE dbgLogDumpContext&
#define LOGFILE m_Log
#define DECLARE_SHARED_LOGFILE SHARED_LOGFILE LOGFILE

#else

#define SHARED_LOGFILE void*
#define LOGFILE _m_dummy_void_pointer
#define DECLARE_SHARED_LOGFILE SHARED_LOGFILE LOGFILE

#endif



#if defined ENABLE_LOGGING
#define LOGGER m_Log
#else
#define LOGGER nullptr
#endif

#endif // INCLUDED_IFACE_TOOLS_H_