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
#include "PGSuperApp.h"
#include "PGSImportPluginDocTemplateBase.h"
#include "SelectItemDlg.h"

#include <IFace/Tools.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>


IMPLEMENT_DYNAMIC(CPGSProjectImporterTemplateItem,CEAFTemplateItem)


IMPLEMENT_DYNAMIC(CPGSImportPluginDocTemplateBase,CEAFDocTemplate)

CPGSImportPluginDocTemplateBase::CPGSImportPluginDocTemplateBase(UINT nIDResource,
                                                                 std::shared_ptr<WBFL::EAF::ICommandCallback> pCallback,
  																                 CRuntimeClass* pDocClass,
  																                 CRuntimeClass* pFrameClass,
 																                 CRuntimeClass* pViewClass,
                                                                 HMENU hSharedMenu,
                                                                 int maxViewCount) :
CEAFDocTemplate(nIDResource,pCallback,pDocClass,pFrameClass,pViewClass,hSharedMenu,maxViewCount)
{
   USES_CONVERSION;

   m_pProjectImporterMgr = nullptr;
}

CPGSImportPluginDocTemplateBase::~CPGSImportPluginDocTemplateBase()
{
   GetProjectImporterManager()->UnloadImporters();
   delete m_pProjectImporterMgr;
}

CDocTemplate::Confidence CPGSImportPluginDocTemplateBase::MatchDocType(LPCTSTR lpszPathName,CDocument*& rpDocMatch)
{
   // This document template doesn't open any document files from disk
   // return noAttempt so that the document manager does not conclude this is the
   // best match
   return CDocTemplate::noAttempt;
}

BOOL CPGSImportPluginDocTemplateBase::DoOpenDocumentFile(LPCTSTR lpszPathName,BOOL bMakeVisible,CEAFDocument* pDocument,CFrameWnd* pFrame)
{
   CEAFDocTemplate::DoOpenDocumentFile(lpszPathName,bMakeVisible,pDocument,pFrame);

   ASSERT_KINDOF(CPGSProjectImporterTemplateItem,m_pTemplateItem);
   CPGSProjectImporterTemplateItem* pTemplateItem = (CPGSProjectImporterTemplateItem*)m_pTemplateItem;

   // Hold the UI events (release in CPGSDocBase::OnCreateFinalize)
   ASSERT_KINDOF(CEAFBrokerDocument, pDocument);
   auto broker = ((CEAFBrokerDocument*)pDocument)->GetBroker();
   GET_IFACE2(broker,IEvents,pEvents);
   GET_IFACE2(broker,IUIEvents,pUIEvents);
   pEvents->HoldEvents();
   pUIEvents->HoldEvents();

   // do the importing
   // this is where the importer does its thing and build a new bridge model
   if ( FAILED(pTemplateItem->m_Importer->Import(broker)) )
   {
      return FALSE;
   }

   // Can't release events here because the views have not yet been created
   // Events are released in CPGSDocBase::OnCreateFinalize

   return TRUE;
}

CString CPGSImportPluginDocTemplateBase::GetTemplateGroupItemDescription(const CEAFTemplateItem* pItem) const
{
   CString strName = pItem->GetName();
   CString strDescription;
   strDescription.Format(_T("Create a new %s project using the %s importer"),GetProjectImporterManager()->GetAppName(),strName);
   return strDescription;
}

BOOL CPGSImportPluginDocTemplateBase::GetDocString(CString& rString,enum DocStringIndex index) const
{
   if ( index == CDocTemplate::fileNewName )
   {
      rString.Format(_T("%s Project Importers"),GetProjectImporterManager()->GetAppName());
      return TRUE;
   }
   else if ( index == CDocTemplate::filterExt || index == CDocTemplate::filterName )
   {
      rString.Empty();
      return TRUE;
   }

   return CEAFDocTemplate::GetDocString(rString,index);
}

CPGSProjectImporterMgrBase* CPGSImportPluginDocTemplateBase::GetProjectImporterManager() const
{
   if ( m_pProjectImporterMgr == nullptr )
   {
      // The first time the project importer manager is requested, create it and loadd all of the importers
      m_pProjectImporterMgr = CreateProjectImporterMgr();
      m_pProjectImporterMgr->LoadImporters();

      // For each registered importer, add a template item to the template group

      IndexType nImporters = m_pProjectImporterMgr->GetImporterCount();
      for (IndexType idx = 0; idx < nImporters; idx++)
      {
         auto importer = m_pProjectImporterMgr->GetImporter(idx);

         auto strText = importer->GetItemText();
         HICON hIcon = importer->GetIcon();
         auto path = importer->GetTemplateFilePath();
         auto template_item = new CPGSProjectImporterTemplateItem((CEAFDocTemplate*)this, strText, path, hIcon, importer);

         m_TemplateGroup.AddItem( template_item );
      }
   }

   return m_pProjectImporterMgr;
}

CEAFTemplateItem* CPGSImportPluginDocTemplateBase::GetTemplateItem(const CLSID& clsid)
{
   IndexType nItems = m_TemplateGroup.GetItemCount();
   for (IndexType idx = 0; idx < nItems; idx++)
   {
      CEAFTemplateItem* pItem = m_TemplateGroup.GetItem(idx);
      if (pItem->IsKindOf(RUNTIME_CLASS(CPGSProjectImporterTemplateItem)))
      {
         CPGSProjectImporterTemplateItem* pMyItem = (CPGSProjectImporterTemplateItem*)pItem;
         CLSID itemCLSID = pMyItem->m_Importer->GetCLSID();
         if (clsid == itemCLSID)
         {
            return pItem;
         }
      }
   }

   return nullptr;
}
