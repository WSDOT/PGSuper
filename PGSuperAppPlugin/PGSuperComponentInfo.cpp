///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "PGSuperComponentInfo.h"
#include "resource.h"
#include <MFCTools\VersionInfo.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

HRESULT CPGSuperComponentInfo::FinalConstruct()
{
   return S_OK;
}

void CPGSuperComponentInfo::FinalRelease()
{
}

BOOL CPGSuperComponentInfo::Init(CEAFApp* pApp)
{
   return TRUE;
}

void CPGSuperComponentInfo::Terminate()
{
}

CString CPGSuperComponentInfo::GetName()
{
   return "PGSuper";
}

CString CPGSuperComponentInfo::GetDescription()
{
   CString strExe = EAFGetApp()->m_pszExeName;
   strExe += ".exe";

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   CString strVersion = verInfo.GetFileVersionAsString();
   CString strCopyright = verInfo.GetLegalCopyright();

   CString strDesc;
   strDesc.Format("Precast-Prestressed Girder Bridge Design, Analysis, and Rating\nVersion %s\n%s",strVersion,strCopyright);
   return strDesc;
}

HICON CPGSuperComponentInfo::GetIcon()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetApp()->LoadIcon(IDI_PGSUPER);
}

bool CPGSuperComponentInfo::HasMoreInfo()
{
   return false;
}

void CPGSuperComponentInfo::OnMoreInfo()
{
}
