///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "TxDOTOptionalDesignUtilities.h"
#include "TxDOTOptionalDesignDocTemplate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignDocTemplate,CEAFDocTemplate)

CTxDOTOptionalDesignDocTemplate::CTxDOTOptionalDesignDocTemplate(UINT nIDResource,
                                                                 IEAFCommandCallback* pCallback,
                                                                 CRuntimeClass* pDocClass,
                                                                 CRuntimeClass* pFrameClass,
                                                                 CRuntimeClass* pViewClass,
                                                                 HMENU hSharedMenu,
                                                                 int maxViewCount)
: CEAFDocTemplate(nIDResource,pCallback,pDocClass,pFrameClass,pViewClass,hSharedMenu,maxViewCount)
{
 
   LoadTemplateInformation();
}

CString CTxDOTOptionalDesignDocTemplate::GetTemplateGroupItemDescription(const CEAFTemplateItem* pItem) const
{
   CString strDescription("Create a new TxDOT Optional Girder Analysis Project");
   return strDescription;
}

void CTxDOTOptionalDesignDocTemplate::LoadTemplateInformation()
{
   // this adds the icon to the right side of the New Project dialog
   CString strExtName;
   GetDocString(strExtName,CDocTemplate::filterExt);

   CString strFileName;
   GetDocString(strFileName,CDocTemplate::fileNewName);

   CString strItemName;
   if ( strExtName != _T("") )
      strItemName.Format(_T("%s (%s)"),strFileName,strExtName);
   else
      strItemName = strFileName;

   // top level icon
   HICON hIcon = AfxGetApp()->LoadIcon(IDI_TXDOT);

   m_TemplateGroup.AddItem( new CEAFTemplateItem(strItemName,NULL,hIcon) );

   // Location of template folders
   CString strWorkgroupFolderName = GetTOGAFolder();

   m_TemplateGroup.Clear();

   if ( strWorkgroupFolderName.GetLength() != 0 )
   {
      CEAFTemplateGroup* pGroup = new CEAFTemplateGroup(this);
      pGroup->SetGroupName(_T("Templates"));
      m_TemplateGroup.AddGroup(pGroup);
      FindInFolder(strWorkgroupFolderName,pGroup,hIcon);

      // CEAFSplashScreen::SetText(_T(""));
   }
}

void CTxDOTOptionalDesignDocTemplate::FindInFolder(LPCTSTR strPath,CEAFTemplateGroup* pGroup,HICON defaultIcon)
{
   HICON folderIcon = defaultIcon;

   //CString strMsg;
   //strMsg.Format(_T("Searching for PGSuper Project Templates in %s"),strPath);
   //CEAFSplashScreen::SetText(strMsg);

   CString strIconFile = strPath;
   int i = strIconFile.ReverseFind(_T('\\'));
   strIconFile += strIconFile.Mid(i);
   strIconFile += _T(".ico");
   HICON hIcon = (HICON)::LoadImage(NULL,strIconFile,IMAGE_ICON,0,0,LR_LOADFROMFILE);
   if ( hIcon )
      folderIcon = hIcon;

   FindTemplateFiles(strPath,pGroup,folderIcon); // find template files in this folder
}

void CTxDOTOptionalDesignDocTemplate::FindTemplateFiles(LPCTSTR strPath,CEAFTemplateGroup* pGroup,HICON folderIcon)
{
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_TEMPLATE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());

   CFileFind finder;
   CString strTemplateFileSpec = strPath + CString(_T("\\*.")) + strTemplateSuffix;
   BOOL bHasTemplateFiles = finder.FindFile(strTemplateFileSpec);
   while ( bHasTemplateFiles )
   {
      bHasTemplateFiles = finder.FindNextFile();

      HICON fileIcon = folderIcon;

      CString strIconFile = finder.GetFilePath();
      strIconFile.Replace(strTemplateSuffix,_T("ico"));
      HICON hIcon = (HICON)::LoadImage(NULL,strIconFile,IMAGE_ICON,0,0,LR_LOADFROMFILE);
      if ( hIcon )
         fileIcon = hIcon;

      CEAFTemplateItem* pItem = new CEAFTemplateItem(finder.GetFileTitle(),finder.GetFilePath(),fileIcon);
      pGroup->AddItem(pItem);
   }
}
