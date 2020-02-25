///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "PGSuperComponentInfo.h"
#include <MFCTools\VersionInfo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HRESULT CPGSuperComponentInfo::FinalConstruct()
{
   return S_OK;
}

void CPGSuperComponentInfo::FinalRelease()
{
}

BOOL CPGSuperComponentInfo::Init(CPGSuperDoc* pDoc)
{
   return TRUE;
}

void CPGSuperComponentInfo::Terminate()
{
}

CString CPGSuperComponentInfo::GetName() const
{
   return _T("WSDOT PGSuper Extensions");
}

CString CPGSuperComponentInfo::GetDescription() const
{
   CString strDesc;
   strDesc.Format(_T("WSDOT-specific features for PGSuper"));
   return strDesc;
}

HICON CPGSuperComponentInfo::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetApp()->LoadIcon(IDI_WSDOT);
}

bool CPGSuperComponentInfo::HasMoreInfo() const
{
   return false;
}

void CPGSuperComponentInfo::OnMoreInfo() const
{
}
