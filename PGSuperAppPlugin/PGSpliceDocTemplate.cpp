///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "PGSpliceDocTemplate.h"
#include "PGSpliceDoc.h"
#include "PGSuperBaseAppPlugin.h"

#include "PGSpliceCatCom.h"
#include "Plugins\PGSuperIEPlugin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CPGSpliceDocTemplate,CPGSuperDocTemplateBase)

CPGSpliceDocTemplate::CPGSpliceDocTemplate(UINT nIDResource,
                                         IEAFCommandCallback* pCallback,
                                         CRuntimeClass* pDocClass,
                                         CRuntimeClass* pFrameClass,
                                         CRuntimeClass* pViewClass,
                                         HMENU hSharedMenu,
                                         int maxViewCount)
: CPGSuperDocTemplateBase(nIDResource,pCallback,pDocClass,pFrameClass,pViewClass,hSharedMenu,maxViewCount)
{
}

CString CPGSpliceDocTemplate::GetAppName() const
{
   return CString(_T("PGSplice"));
}

UINT CPGSpliceDocTemplate::GetTemplateIconResourceID() const
{
   return IDR_PGSPLICE_TEMPLATE_ICON;
}

CString CPGSpliceDocTemplate::GetTemplateSuffix()
{
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSPLICE_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}
