///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ThreadManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
   
// predicate object used to check if a thread is done executing
class CThreadChecker
{
public:
   CThreadChecker(bool bWait) : m_bWait(bWait) {}


   bool operator()(CWinThread* pThread)
   {
      // if m_bWait is true, wait until the thread is complete, otherwise return immediately
      // if the thread is still running

      if ( ::WaitForSingleObject(pThread->m_hThread,m_bWait ? INFINITE : 0) == WAIT_OBJECT_0 )
      {
         // WAIT_OBJECT_0 means the thread is done executing
         delete pThread; // delete the thread (we own it... see below where the thread get created)

         // return true because we want the thread object removed from the container
         return true;
      }

      // don't remove the thread object from the container
      return false;
   }
private:
   bool m_bWait;
};

CThreadManager::CThreadManager()
{
}

CThreadManager::~CThreadManager()
{
   // remove all threads we are managing... wait until they are complete
   FlushThreads(true);
}

void CThreadManager::CreateThread(AFX_THREADPROC pfnThreadProc,LPVOID pParam)
{
   FlushThreads(false); // flush any threads that may have finished

   // create the new thread in a suspended state (we want to modify the thread properties)
   CWinThread* pThread = AfxBeginThread(pfnThreadProc,pParam,THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);

   pThread->m_bAutoDelete = FALSE; // we want to delete the thread object. we need it to test if the thread is done

   m_Threads.push_back(pThread); // hang on to the thread object

   pThread->ResumeThread(); // resume execution of the thread
}

void CThreadManager::FlushThreads(bool bWait)
{
   // remove threads from the list if they are complete
   m_Threads.remove_if(CThreadChecker(bWait));
}
