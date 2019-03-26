///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

/****************************************************************************
CLASS
   CPGSBaseCommandLineInfo
****************************************************************************/
#include <PgsExt\PgsExtLib.h>
#include <PgsExt\BaseCommandLineInfo.h>
#include <Iface\Test1250.h>

#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPGSBaseCommandLineInfo::CPGSBaseCommandLineInfo() :
CEAFCommandLineInfo(),
m_bDo1250Test(false),
m_SubdomainId(0),
m_bSetUpdateLibrary(false)
{
   m_Count=0;
}

CPGSBaseCommandLineInfo::~CPGSBaseCommandLineInfo()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPGSBaseCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
   m_Count++;

   bool bMyParameter = false;

   CString strParam(lpszParam);
   if ( bFlag )
   {
      // Parameter is a flag (-flag or /flag)
      if ( strParam.Left(3).CompareNoCase(_T("App")) == 0 && strParam.Right(strParam.GetLength() - 4).CompareNoCase(GetAppName()) == 0)
      {
         ATLASSERT(strParam.Right(strParam.GetLength() - 4).CompareNoCase(GetAppName()) == 0); 

         // the /App=<appname> option was used on the command line directing command line
         // options to us.
         // We don't have to do anything, just acknowledge it is our parameter
         m_bCommandLineMode = false; // unless other options are given, we don't want to run the command line and exit
         bMyParameter = true;
         m_Count--; // pretend the option was never used
      }
      else if ( strParam.CompareNoCase(_T("TestR")) == 0 )
      {
         // Run the full 12-50 regression tests suite
         m_SubdomainId = RUN_REGRESSION;
         m_bDo1250Test      = true;
         m_bCommandLineMode = true;
         bMyParameter       = true;
      }
      else if ( strParam.Left(4).CompareNoCase(_T("Test")) == 0 )
      {
         // Could be a sub-domain 12-50 test

         // remove the "test" and get the sub-domain ID
         strParam = strParam.Right(strParam.GetLength() - 4);
         CComVariant var(strParam);
         if ( SUCCEEDED(var.ChangeType(VT_I4)) )
         {
            m_SubdomainId = var.lVal;

            m_bDo1250Test      = true;
            m_bCommandLineMode = true;
            bMyParameter       = true;
         }
      }
      else if ( strParam.Left(6).CompareNoCase(_T("SetLib")) == 0 )
      {
         // We have gone away from the terminology "Library and Templates" in favor of the more general
         // term "Configuration". We will still process the SetLib command for backwards compatibility
         // reseaons.

         // Set to server/library and update
         // parse server and publisher name
         int nch = strParam.GetLength();
         CString rght = strParam.Right(nch-7);

         int curPos = 0;
         m_CatalogServerName = rght.Tokenize(_T(":"), curPos);
         if (m_CatalogServerName.IsEmpty())
         {
            m_bError = TRUE;
         }

         m_PublisherName = rght.Tokenize(_T(":"), curPos);
         if (m_PublisherName.IsEmpty())
         {
            m_bError = TRUE;
         }

         if (m_bError==TRUE)
         {
            AfxMessageBox(_T("Error parsing SetLib command - correct format is /SetLib=ServerName:PublisherName"));
            return;
         }

         m_bSetUpdateLibrary = true;
         m_bCommandLineMode = true;
         bMyParameter       = true;
      }
      else if ( strParam.Left(13).CompareNoCase(_T("Configuration")) == 0 )
      {
         // Set to server/library and update
         // parse server and publisher name
         int nch = strParam.GetLength();
         CString rght = strParam.Right(nch-14);

         int curPos = 0;
         m_CatalogServerName = rght.Tokenize(_T(":"), curPos);
         if (m_CatalogServerName.IsEmpty())
         {
            m_bError = TRUE;
         }

         m_PublisherName = rght.Tokenize(_T(":"), curPos);
         if (m_PublisherName.IsEmpty())
         {
            m_bError = TRUE;
         }

         if (m_bError==TRUE)
         {
            AfxMessageBox(_T("Error parsing Configuration command - correct format is /Configuration=ServerName:PublisherName"));
            return;
         }

         m_bSetUpdateLibrary = true;
         m_bCommandLineMode = true;
         bMyParameter       = true;
      }
   }

   if ( !bMyParameter )
   {
     CEAFCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
   }
}

CString CPGSBaseCommandLineInfo::GetUsageMessage()
{
   CString strOption1(_T("/TestR - Generates NCHRP 12-50 test results for all problem domains"));
   CString strOption2(_T("/Test[n] - Generates NCHRP 12-50 test results for a specified problem domain. Substitute the problem domain ID for n"));
   CString strOption3(_T("/Configuration=ServerName:PublisherName - Sets the application configuration"));
   CString strOption4(_T("Application extension may offer additional command line options. Refer to the user documentation for details."));
   
   CString strMsg;
   strMsg.Format(_T("%s\n%s\n%s\n%s"),strOption1,strOption2,strOption3,strOption4);
   return strMsg;
}

CString CPGSBaseCommandLineInfo::GetErrorMessage()
{
   CString strMsg;
   strMsg.Format(_T("%s was started with invalid command line options."),GetAppName());
   return strMsg;
}
