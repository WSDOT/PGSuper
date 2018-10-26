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
#include "PGSuperImportPluginDocTemplate.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CPGSuperImportPluginDocTemplate,CCountedMultiDocTemplate)

CPGSuperImportPluginDocTemplate::CPGSuperImportPluginDocTemplate(UINT nIDResource,
 																 CRuntimeClass* pDocClass,
 																 CRuntimeClass* pFrameClass,
 																 CRuntimeClass* pViewClass,
																 int maxViewCount)
: CCountedMultiDocTemplate(nIDResource,pDocClass,pFrameClass,pViewClass,maxViewCount)
{
}

CPGSuperDoc* CPGSuperImportPluginDocTemplate::Import(UINT nID)
{
   // This is stolen from CMultiDocTemplate::OpenDocumentFile
   // and tweaked so that CPGSuperDoc::Import is called insted of CDocument::OnNewDocument

	CPGSuperDoc* pDocument = (CPGSuperDoc*)CreateNewDocument();
   ASSERT_KINDOF(CPGSuperDoc,pDocument);

	if (pDocument == NULL)
	{
		TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return NULL;
	}
	ASSERT_VALID(pDocument);

	BOOL bAutoDelete = pDocument->m_bAutoDelete;
	pDocument->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
	CFrameWnd* pFrame = CreateNewFrame(pDocument, NULL);
	pDocument->m_bAutoDelete = bAutoDelete;
	if (pFrame == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDocument;       // explicit delete on error
		return NULL;
	}
	ASSERT_VALID(pFrame);

	// create a new document - with default document name
	SetDefaultTitle(pDocument);


   // NOTE: OnImportDocument holds events, but doesn't release them
   //       It doesn't release them because they will fire before the views
   //       are created and bad things (ie. crash) happens.
   //       We have to release the events below.
	if (!pDocument->OnImportDocument(nID) )
	{
		// user has be alerted to what failed in OnNewDocument
		TRACE0("CDocument::OnNewDocument returned FALSE.\n");
		pFrame->DestroyWindow();
		return NULL;
	}

	// it worked, now bump untitled count
	m_nUntitledCount++;

	InitialUpdateFrame(pFrame, pDocument, TRUE);

   // NOTE: Now that the views are created, we can release the events
   //       held by the document class
   CComPtr<IBroker> broker;
   pDocument->GetBroker(&broker);
   GET_IFACE2(broker,IEvents,pEvents);
   GET_IFACE2(broker,IUIEvents,pUIEvents);
   pEvents->FirePendingEvents(); 
   pUIEvents->HoldEvents(false); // stop holding events, but don't fire then either

   return pDocument;
}