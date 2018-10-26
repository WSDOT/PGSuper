///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin_i.h"
#include "PGSpliceComponentInfo.h"
#include "resource.h"
#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

HRESULT CPGSpliceComponentInfo::FinalConstruct()
{
   return S_OK;
}

void CPGSpliceComponentInfo::FinalRelease()
{
}

BOOL CPGSpliceComponentInfo::Init(CEAFApp* pApp)
{
   return TRUE;
}

void CPGSpliceComponentInfo::Terminate()
{
}

CString CPGSpliceComponentInfo::GetName()
{
   return _T("PGSplice");
}

CString CPGSpliceComponentInfo::GetDescription()
{
   CString strExe = EAFGetApp()->m_pszExeName;
   strExe += _T(".exe");

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   CString strVersion = verInfo.GetFileVersionAsString();
   CString strCopyright = verInfo.GetLegalCopyright();

   CString strDesc;
   strDesc.Format(_T("Precast-Posttensioned Spliced Girder Bridge Design, Analysis, and Rating\nVersion %s\n%s"),strVersion,strCopyright);
   return strDesc;
}

HICON CPGSpliceComponentInfo::GetIcon()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetApp()->LoadIcon(IDR_PGSPLICE);
}

bool CPGSpliceComponentInfo::HasMoreInfo()
{
   return false;
}

void CPGSpliceComponentInfo::OnMoreInfo()
{
}
