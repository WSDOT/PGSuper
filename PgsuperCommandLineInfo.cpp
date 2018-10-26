///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
   CPGSuperCommandLineInfo
****************************************************************************/
#include "PGSuperAppPlugin\stdafx.h"
#include "PgsuperCommandLineInfo.h"
#include <Iface\Test1250.h>

#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPGSuperCommandLineInfo::CPGSuperCommandLineInfo() :
CEAFCommandLineInfo(),
m_bDo1250Test(false),
m_SubdomainId(0),
m_bSetUpdateLibrary(false)
{
   m_Count=0;
}

CPGSuperCommandLineInfo::~CPGSuperCommandLineInfo()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPGSuperCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
   m_Count++;

   bool bMyParameter = false;

   CString strParam(lpszParam);
   if ( bFlag )
   {
      // Parameter is a flag (-flag or /flag)
      if ( strParam.CompareNoCase(_T("TestR")) == 0 )
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
   }

   if ( !bMyParameter )
     CEAFCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
}

CString CPGSuperCommandLineInfo::GetUsageMessage()
{
   CString strMsg;
   strMsg.Format(_T("PGSuper filename.pgs\nPGSuper /TestR filename.pgs\nPGSuper /Test[n] filename.pgs\n\nPGSuper extensions may offer additional command line options. Refer to the user documentation for details."));
   return strMsg;
}

CString CPGSuperCommandLineInfo::GetErrorMessage()
{
   CString strMsg;
   strMsg.Format(_T("PGSuper was started with invalid command line options. Valid command line options are:\n%s\n%s\n\n%s"),
      _T("PGSuper filename.pgs"),
      _T("PGSuper /TestR filename.pgs"),
      _T("PGSuper /SetLib=ServerName:PublisherName"),
      _T("See Command Line Options in the PGSuper User Guide for more information"));
   return strMsg;
}
