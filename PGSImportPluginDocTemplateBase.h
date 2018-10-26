///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#pragma once

#include <EAF\EAFDocTemplate.h>
#include "PGSuperDoc.h"
#include "Plugins\PGSuperIEPlugin.h"
#include "PGSProjectImporterMgrBase.h"

#include "PGSuperProjectImporterMgr.h"

class CPGSImportPluginDocTemplateBase : public CEAFDocTemplate
{
public:
   CPGSImportPluginDocTemplateBase(UINT nIDResource,
                                   IEAFCommandCallback* pCallback,
                                   CRuntimeClass* pDocClass,
                                   CRuntimeClass* pFrameClass,
                                   CRuntimeClass* pViewClass,
                                   HMENU hSharedMenu = NULL,
                                   int maxViewCount = -1);
   
   virtual ~CPGSImportPluginDocTemplateBase();

   virtual CString GetTemplateGroupItemDescription(const CEAFTemplateItem* pItem) const;

	virtual BOOL GetDocString(CString& rString,enum DocStringIndex index) const;

   CPGSProjectImporterMgrBase* GetProjectImporterManager() const;

   virtual CDocTemplate::Confidence MatchDocType(LPCTSTR lpszPathName,	CDocument*& rpDocMatch);

protected:
   virtual BOOL DoOpenDocumentFile(LPCTSTR lpszPathName,BOOL bMakeVisible,CEAFDocument* pDocument,CFrameWnd* pFrame);

   DECLARE_DYNAMIC(CPGSImportPluginDocTemplateBase)

   virtual CPGSProjectImporterMgrBase* CreateProjectImporterMgr() const = 0;
   mutable CPGSProjectImporterMgrBase* m_pProjectImporterMgr; // don't access this directly... use the GetProjectImporterManager method
};

