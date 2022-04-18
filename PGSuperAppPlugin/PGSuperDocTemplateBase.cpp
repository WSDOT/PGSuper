///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "PGSuperDocTemplateBase.h"
#include "PGSuperBaseAppPlugin.h"

#include "PGSuperCatCom.h"
#include "Plugins\PGSuperIEPlugin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CPGSuperDocTemplateBase,CEAFDocTemplate)

CPGSuperDocTemplateBase::CPGSuperDocTemplateBase(UINT nIDResource,
                                         IEAFCommandCallback* pCallback,
                                         CRuntimeClass* pDocClass,
                                         CRuntimeClass* pFrameClass,
                                         CRuntimeClass* pViewClass,
                                         HMENU hSharedMenu,
                                         int maxViewCount)
: CEAFDocTemplate(nIDResource,pCallback,pDocClass,pFrameClass,pViewClass,hSharedMenu,maxViewCount)
{
}

void CPGSuperDocTemplateBase::SetPlugin(IEAFAppPlugin* pPlugin)
{
   CEAFDocTemplate::SetPlugin(pPlugin);
   LoadTemplateInformation();
}

CString CPGSuperDocTemplateBase::GetTemplateGroupItemDescription(const CEAFTemplateItem* pItem) const
{
   CString strDescription;
   strDescription.Format(_T("Create a new %s project using the %s template"),GetAppName(),pItem->GetName());
   return strDescription;
}

void CPGSuperDocTemplateBase::LoadTemplateInformation()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   HICON defaultIcon = pApp->LoadIcon(GetTemplateIconResourceID());

   CPGSAppPluginBase* pAppPlugin = dynamic_cast<CPGSAppPluginBase*>(m_pPlugin);
   CString strWorkgroupFolderName;
   pAppPlugin->GetTemplateFolders(strWorkgroupFolderName);

   m_TemplateGroup.Clear();

   if ( strWorkgroupFolderName.GetLength() != 0 )
   {
      FindInFolder(strWorkgroupFolderName,&m_TemplateGroup,defaultIcon);

      CEAFSplashScreen::SetText(_T(""));
   }

   HICON hAppIcon = pApp->LoadIcon(m_nIDResource);
   m_TemplateGroup.SetIcon(hAppIcon);
}

void CPGSuperDocTemplateBase::SetDocStrings(const CString& str)
{
   m_strDocStrings = str;
}

void CPGSuperDocTemplateBase::FindInFolder(LPCTSTR strPath,CEAFTemplateGroup* pGroup,HICON defaultIcon)
{
   HICON folderIcon = defaultIcon;

   CString strMsg;
   strMsg.Format(_T("Searching for %s Project Templates in %s"),GetAppName(),strPath);
   CEAFSplashScreen::SetText(strMsg);

   CString strIconFile = strPath;
   int i = strIconFile.ReverseFind('\\');

   CString strIconRootName = strIconFile.Mid(i);
   if ( strIconRootName == _T("\\") )
   {
      strIconRootName = _T("Default"); // there isn't foldername to generate the icon file name with, just use "Default"
   }

   strIconFile += strIconRootName;

   strIconFile += _T(".ico");
   HICON hIcon = (HICON)::LoadImage(nullptr,strIconFile,IMAGE_ICON,0,0,LR_LOADFROMFILE);
   if ( hIcon )
   {
      folderIcon = hIcon;
   }

   pGroup->SetIcon(folderIcon);

   FindTemplateFiles(strPath,pGroup,folderIcon); // find template files in this folder

   // Drill down into sub-folders
   CString strDirectoryName = strPath + CString(_T("\\*.*"));
   CFileFind finder;
   BOOL bHasFiles = finder.FindFile(strDirectoryName);
   while ( bHasFiles )
   {
      bHasFiles = finder.FindNextFile();
      if (finder.IsDirectory() && !finder.IsDots())
      {
         // sub-directory found
         std::unique_ptr<CEAFTemplateGroup> pNewGroup = std::make_unique<CEAFTemplateGroup>();
         pNewGroup->SetGroupName(finder.GetFileTitle());

         FindInFolder(finder.GetFilePath(),pNewGroup.get(),folderIcon);
         if ( pNewGroup->GetItemCount() != 0 || pNewGroup->GetGroupCount() != 0)
         {
            pGroup->AddGroup(pNewGroup.release());
         }
         // else.... pNewGroup goes out of scope and releases the allocation
      }
   }
}

void CPGSuperDocTemplateBase::FindTemplateFiles(LPCTSTR strPath,CEAFTemplateGroup* pGroup,HICON folderIcon)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CString strTemplateSuffix(GetTemplateSuffix());

   CFileFind finder;
   CString strTemplateFileSpec = strPath + CString(_T("\\*.")) + strTemplateSuffix;
   BOOL bHasTemplateFiles = finder.FindFile(strTemplateFileSpec);
   while ( bHasTemplateFiles )
   {
      bHasTemplateFiles = finder.FindNextFile();

      HICON fileIcon = folderIcon;

      CString strIconFile = finder.GetFilePath();
      strIconFile.Replace(strTemplateSuffix,_T("ico"));
      HICON hIcon = (HICON)::LoadImage(nullptr,strIconFile,IMAGE_ICON,0,0,LR_LOADFROMFILE);
      if (hIcon)
      {
         fileIcon = hIcon;
      }

      std::unique_ptr<CEAFTemplateItem> pItem = std::make_unique<CEAFTemplateItem>(this,finder.GetFileTitle(),finder.GetFilePath(),fileIcon);
      pGroup->AddItem(pItem.release());
   }
}
