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
#include "PGSpliceProjectImporterPluginApp.h"
#include "PGSpliceDoc.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include "PGSpliceImportPluginDocTemplate.h"
#include "PGSpliceCommandLineInfo.h"

const CRuntimeClass* CPGSpliceProjectImporterPluginApp::GetDocTemplateRuntimeClass()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return RUNTIME_CLASS(CPGSpliceImportPluginDocTemplate);
}

CString CPGSpliceProjectImporterPluginApp::GetTemplateFileExtension()
{ 
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSPLICE_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}

CATID CPGSpliceProjectImporterPluginApp::GetProjectImporterCATID()
{
   return CATID_PGSpliceProjectImporter;
}

UINT CPGSpliceProjectImporterPluginApp::GetMenuResourceID()
{
   return IDR_PGSPLICE;
}

CPGSImportPluginDocTemplateBase* CPGSpliceProjectImporterPluginApp::CreateDocTemplate()
{
   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());

   CPGSpliceImportPluginDocTemplate* pDocTemplate = new CPGSpliceImportPluginDocTemplate(
		IDR_PGSPLICEPROJECTIMPORTER,
        callback,
		RUNTIME_CLASS(CPGSpliceDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   return pDocTemplate;
}

CEAFCommandLineInfo* CPGSpliceProjectImporterPluginApp::CreateCommandLineInfo() const
{
   return new CPGSpliceProjectImporterCommandLineInfo();
}

CString CPGSpliceProjectImporterPluginApp::GetUsageMessage()
{
   CPGSpliceProjectImporterCommandLineInfo pgsCmdInfo;
   return pgsCmdInfo.GetUsageMessage();
}
