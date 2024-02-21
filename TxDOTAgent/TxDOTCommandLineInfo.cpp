///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
   CTxDOTCommandLineInfo
****************************************************************************/
#include "stdafx.h"
#include "TxDOTCommandLineInfo.h"
#include <Iface\Test1250.h>

#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTxDOTCommandLineInfo::CTxDOTCommandLineInfo() :
CPGSBaseCommandLineInfo(),
m_TxSpan(INVALID_INDEX),
m_TxGirder(INVALID_INDEX),
m_DoAppendToFile(false),
m_DoTogaTest(false)
{
   m_Count=0;
}

CTxDOTCommandLineInfo::~CTxDOTCommandLineInfo()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CTxDOTCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
   bool bMyParameter = false;

   CString strParam(lpszParam);
   if ( bFlag )
   {
      // Parameter is a flag (-flag or /flag)
      if ( m_Count != 0 )
      {
         // Not necessarily an error... might be a different extension's command line option
         //// the flag must be the first and only flag on the command line
         //m_bError = TRUE;
         //m_strErrorMsg = GetUsageString();
      }
      else if (strParam.CompareNoCase(_T("TxTOGA")) == 0)
      {
         // TOGA test file
         m_DoTogaTest = true;
         m_bCommandLineMode = true;
         m_bShowSplash = FALSE;
         bMyParameter = true;
      }
   }
   else
   {
      // Not a flag, just a regular parameter
      if ( m_Count == 0 && !bLast )
      {
         // first parameter must be a flag
         m_bError = true;
         m_strErrorMsg.Format(_T("The first parameter must be a flag.\n%s"),GetUsageString());
      }
      else if ( m_DoTogaTest )
      {
         if ( m_Count == 2 )
         {
            // output file name
            m_TxOutputFile = lpszParam;
            bMyParameter = true;
         }
      }
   }

   if ( !bMyParameter )
     CPGSBaseCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);

   m_Count++;
}

CString CTxDOTCommandLineInfo::GetUsageMessage()
{
   return CString(_T("/TxTOGA filename.toga outputfile"));
}

CString CTxDOTCommandLineInfo::GetUsageString()
{
   return CString(_T("Valid parameters are /TxTOGA filename.toga outputfile"));
}

LPCTSTR CTxDOTCommandLineInfo::GetAppName() const
{
   return _T("TOGA");
}
