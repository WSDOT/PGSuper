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
#include "KDOTExport.h"
#include "KDOTComponentInfo.h"
#include "resource.h"
#include <MFCTools\VersionInfo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HRESULT CKDOTComponentInfo::FinalConstruct()
{
   return S_OK;
}

void CKDOTComponentInfo::FinalRelease()
{
}

BOOL CKDOTComponentInfo::Init(CPGSuperDoc* pDoc)
{
   return TRUE;
}

void CKDOTComponentInfo::Terminate()
{
}

CString CKDOTComponentInfo::GetName() const
{
   return _T("KDOT PGSuper Exporter");
}

CString CKDOTComponentInfo::GetDescription() const
{
   CString strDesc;
   strDesc.Format(_T("KDOT-specific features for PGSuper"));
   return strDesc;
}

HICON CKDOTComponentInfo::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetApp()->LoadIcon(IDI_KDOT);
}

bool CKDOTComponentInfo::HasMoreInfo() const
{
   return false;
}

void CKDOTComponentInfo::OnMoreInfo() const
{
}
