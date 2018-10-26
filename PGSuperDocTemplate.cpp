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
#include "resource.h"
#include "PGSuper.h"
#include "PGSuperDocTemplate.h"
#include "PGSuperDoc.h"

#include "PGSuperCatCom.h"
#include "Plugins\PGSuperIEPlugin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CPGSuperDocTemplate,CEAFDocTemplate)

CPGSuperDocTemplate::CPGSuperDocTemplate(UINT nIDResource,
                   CRuntimeClass* pDocClass,
                   CRuntimeClass* pFrameClass,
                   CRuntimeClass* pViewClass,
                   HMENU hSharedMenu,
                   int maxViewCount)
: CEAFDocTemplate(nIDResource,pDocClass,pFrameClass,pViewClass,hSharedMenu,maxViewCount)
{
   // Register the component categories PGSuper needs
   sysComCatMgr::CreateCategory(L"PGSuper Agent",CATID_PGSuperAgent);
   sysComCatMgr::CreateCategory(L"PGSuper Extension Agent",CATID_PGSuperExtensionAgent);
   sysComCatMgr::CreateCategory(L"PGSuper Beam Family",CATID_BeamFamily);
   sysComCatMgr::CreateCategory(L"PGSuper Project Importer Plugin",CATID_PGSuperProjectImporter);
   sysComCatMgr::CreateCategory(L"PGSuper Data Importer Plugin",CATID_PGSuperDataImporter);
   sysComCatMgr::CreateCategory(L"PGSuper Data Exporter Plugin",CATID_PGSuperDataExporter);

   LoadTemplateInformation();
}

CString CPGSuperDocTemplate::GetTemplateGroupItemDescription(const CEAFTemplateItem* pItem) const
{
   CString strDescription;
   strDescription.Format("Create a new PGSuper project using the %s template",pItem->GetName());
   return strDescription;
}

void CPGSuperDocTemplate::LoadTemplateInformation()
{
   CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();

   HICON defaultIcon = pApp->LoadIcon(IDR_PGSUPER_TEMPLATE_ICON);

   CString strWorkgroupFolderName;
   pApp->GetTemplateFolders(strWorkgroupFolderName);

   m_TemplateGroup.Clear();

   if ( strWorkgroupFolderName.GetLength() != 0 )
   {
      CEAFTemplateGroup* pGroup = new CEAFTemplateGroup(this);
      pGroup->SetGroupName("Workgroup Templates");
      m_TemplateGroup.AddGroup(pGroup);
      FindInFolder(strWorkgroupFolderName,pGroup,defaultIcon);

      CEAFSplashScreen::SetText("");
   }
}

void CPGSuperDocTemplate::SetDocStrings(const CString& str)
{
   m_strDocStrings = str;
}

void CPGSuperDocTemplate::FindInFolder(LPCSTR strPath,CEAFTemplateGroup* pGroup,HICON defaultIcon)
{
   HICON folderIcon = defaultIcon;

   CString strMsg;
   strMsg.Format("Searching for PGSuper Project Templates in %s",strPath);
   CEAFSplashScreen::SetText(strMsg);

   CString strIconFile = strPath;
   int i = strIconFile.ReverseFind('\\');
   strIconFile += strIconFile.Mid(i);
   strIconFile += ".ico";
   HICON hIcon = (HICON)::LoadImage(NULL,strIconFile,IMAGE_ICON,0,0,LR_LOADFROMFILE);
   if ( hIcon )
      folderIcon = hIcon;

   FindTemplateFiles(strPath,pGroup,folderIcon); // find template files in this folder

   // Drill down into sub-folders
   CString strDirectoryName = strPath + CString("\\*.*");
   CFileFind finder;
   BOOL bHasFiles = finder.FindFile(strDirectoryName);
   while ( bHasFiles )
   {
      bHasFiles = finder.FindNextFile();
      if (finder.IsDirectory() && !finder.IsDots())
      {
         // sub-directory found
         CEAFTemplateGroup* pNewGroup = new CEAFTemplateGroup(this);
         pNewGroup->SetGroupName(finder.GetFileTitle());

         FindInFolder(finder.GetFilePath(),pNewGroup,folderIcon);
         if ( pNewGroup->GetItemCount() != 0 || pNewGroup->GetGroupCount() != 0)
            pGroup->AddGroup(pNewGroup);
         else
            delete pNewGroup;
      }
   }
}

void CPGSuperDocTemplate::FindTemplateFiles(LPCSTR strPath,CEAFTemplateGroup* pGroup,HICON folderIcon)
{
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());

   CFileFind finder;
   CString strTemplateFileSpec = strPath + CString("\\*.") + strTemplateSuffix;
   BOOL bHasTemplateFiles = finder.FindFile(strTemplateFileSpec);
   while ( bHasTemplateFiles )
   {
      bHasTemplateFiles = finder.FindNextFile();

      HICON fileIcon = folderIcon;

      CString strIconFile = finder.GetFilePath();
      strIconFile.Replace(strTemplateSuffix,"ico");
      HICON hIcon = (HICON)::LoadImage(NULL,strIconFile,IMAGE_ICON,0,0,LR_LOADFROMFILE);
      if ( hIcon )
         fileIcon = hIcon;

      CEAFTemplateItem* pItem = new CEAFTemplateItem(finder.GetFileTitle(),finder.GetFilePath(),fileIcon);
      pGroup->AddItem(pItem);
   }
}
