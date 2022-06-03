///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#pragma once

#include <MFCTools\Exceptions.h>

class CAnalysisResult
{
public:
   CAnalysisResult(LPCTSTR lpszFile,long line) : m_File(lpszFile),m_Line(line),m_Result(S_OK){};
   CAnalysisResult(LPCTSTR lpszFile,long line,HRESULT hr) : m_File(lpszFile),m_Line(line),m_Result(hr) {ProcessHResult();}

   HRESULT operator=(HRESULT hr) { m_Result = hr; return ProcessHResult(); }

   operator HRESULT() { return m_Result; }

private:
   HRESULT ProcessHResult()
   {
      if ( FAILED(m_Result) )
      {
         ATLASSERT(false); // attention grabber
         CString strMsg;
         strMsg.Format(_T("An error occurred during the girder structural analysis (%d)\n%s, Line %d"),m_Result,m_File,m_Line);
         THROW_UNWIND(strMsg,-1);
      }
      return m_Result;
   }
   HRESULT m_Result;
   CString m_File;
   long m_Line;
};