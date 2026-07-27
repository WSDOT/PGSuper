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
#include <FEA2D\XFEA2D.h>

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

// WBFL::FEA2D::XFEA2D (like every WBFL::System::XBase) only signals that
// the library couldn't complete the operation - it carries no notion of
// severity. It's the client's job to decide whether that's fatal or not,
// and a girder structural analysis failure has always been treated as
// non-fatal here (the app reports the error and keeps running), the same
// way CAnalysisResult above used THROW_UNWIND rather than THROW_SHUTDOWN
// for a failed Fem2d COM call.
//
// Wrap any single FEA2D call (Model::ComputeXxx, etc.) in this macro to
// catch WBFL::FEA2D::XFEA2D and re-throw it as a CXUnwind, stamped with
// *this* call site's file/line - XFEA2D's own file/line point inside
// FEA2D's implementation (wherever THROW_FEA2D fired), which isn't useful
// for knowing which of the many PGSuper call sites was responsible.
#define FEA2D_ANALYSIS_RESULT(expr) \
   try \
   { \
      expr; \
   } \
   catch (const WBFL::FEA2D::XFEA2D& fea2d_ex) \
   { \
      CString strMsg; \
      strMsg.Format(_T("An error occurred during the girder structural analysis\n%s\n%s, Line %d"), fea2d_ex.GetErrorMessage().c_str(), _T(__FILE__), __LINE__); \
      THROW_UNWIND(strMsg, fea2d_ex.GetReason()); \
   }