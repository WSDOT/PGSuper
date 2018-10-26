///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include "WSDOTAgent_i.h"
#include "WSDOTComponentInfo.h"
#include "resource.h"
#include <MFCTools\VersionInfo.h>

HRESULT CWSDOTComponentInfo::FinalConstruct()
{
   return S_OK;
}

void CWSDOTComponentInfo::FinalRelease()
{
}

BOOL CWSDOTComponentInfo::Init(CPGSuperDoc* pDoc)
{
   return TRUE;
}

void CWSDOTComponentInfo::Terminate()
{
}

CString CWSDOTComponentInfo::GetName()
{
   return _T("WSDOT PGSuper Extensions");
}

CString CWSDOTComponentInfo::GetDescription()
{
   CString strDesc;
   strDesc.Format(_T("WSDOT-specific features for PGSuper"));
   return strDesc;
}

HICON CWSDOTComponentInfo::GetIcon()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetApp()->LoadIcon(IDI_WSDOT);
}

bool CWSDOTComponentInfo::HasMoreInfo()
{
   return false;
}

void CWSDOTComponentInfo::OnMoreInfo()
{
}
