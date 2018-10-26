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

#include "stdafx.h"
#include "PGSuperDocManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSuperDocManager::CPGSuperDocManager() :
CDocManager()
{
}

CPGSuperDocManager::~CPGSuperDocManager()
{
}

void CPGSuperDocManager::OnImport(UINT nID)
{
   ASSERT(2 <= m_templateList.GetCount() );

   POSITION pos = m_templateList.GetHeadPosition();
   CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos); // this is the template at the head of the list
   
   ASSERT( pos != NULL);
   pTemplate = (CDocTemplate*)m_templateList.GetNext(pos); // this is the 2nd template in the list (the one we want)

   ASSERT( pTemplate != NULL );

   CPGSuperImportPluginDocTemplate* pPluginTemplate = (CPGSuperImportPluginDocTemplate*)pTemplate;
   ASSERT_KINDOF(CPGSuperImportPluginDocTemplate,pPluginTemplate);

   pPluginTemplate->Import(nID);
}

void CPGSuperDocManager::OnFileNew()
{
   // Replacing the functionality of the document manager
   //
   // Because there are three templates registered with the document manager
   // it will open a dialog to ask the user which template to use when
   // creating the new document.
   //
   // We want either the 1st or 3rd templates based on
   // the new file mode

   //CDocManager::OnFileNew();
	if (m_templateList.IsEmpty())
	{
		TRACE0("Error: no document templates registered with CWinApp.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return;
	}

   POSITION position = m_templateList.GetHeadPosition();

	CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(position);
	ASSERT(pTemplate != NULL);
	ASSERT_KINDOF(CDocTemplate, pTemplate);

   pTemplate->OpenDocumentFile(NULL);
}
