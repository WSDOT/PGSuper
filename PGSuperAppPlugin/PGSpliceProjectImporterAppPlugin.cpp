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

#include "stdafx.h"
#include "PGSpliceProjectImporterAppPlugin.h"
#include "PGSpliceDoc.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include "PGSpliceImportPluginDocTemplate.h"
#include "PGSpliceCommandLineInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



HRESULT CPGSpliceProjectImporterAppPlugin::FinalConstruct()
{
   m_MyCmdTarget.m_pMyAppPlugin = this;
   return OnFinalConstruct(); // CPGSAppPluginBase
}

void CPGSpliceProjectImporterAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CPGSAppPluginBase
}
const CRuntimeClass* CPGSpliceProjectImporterAppPlugin::GetDocTemplateRuntimeClass()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return RUNTIME_CLASS(CPGSpliceImportPluginDocTemplate);
}

CString CPGSpliceProjectImporterAppPlugin::GetTemplateFileExtension()
{ 
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSPLICE_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}

CATID CPGSpliceProjectImporterAppPlugin::GetProjectImporterCATID()
{
   return CATID_PGSpliceProjectImporter;
}

UINT CPGSpliceProjectImporterAppPlugin::GetMenuResourceID()
{
   return IDR_PGSPLICE;
}

CPGSImportPluginDocTemplateBase* CPGSpliceProjectImporterAppPlugin::CreateDocTemplate()
{
   CPGSpliceImportPluginDocTemplate* pDocTemplate = new CPGSpliceImportPluginDocTemplate(
		IDR_PGSPLICEPROJECTIMPORTER,
      this,
		RUNTIME_CLASS(CPGSpliceDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   return pDocTemplate;
}

CEAFCommandLineInfo* CPGSpliceProjectImporterAppPlugin::CreateCommandLineInfo() const
{
   return new CPGSpliceProjectImporterCommandLineInfo();
}

CString CPGSpliceProjectImporterAppPlugin::GetUsageMessage()
{
   CPGSpliceProjectImporterCommandLineInfo pgsCmdInfo;
   return pgsCmdInfo.GetUsageMessage();
}
