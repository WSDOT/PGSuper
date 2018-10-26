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

#include "stdafx.h"
#include "TxDOTOptionalDesignUtilities.h"
#include "TxDOTOptionalDesignDocTemplate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// utility data structures for managing TOGA template file
struct TemplateFile
{
   std::_tstring FileTitle;
   std::_tstring FilePath;
};

struct TemplateFolder
{
   std::_tstring Title; // name of folder and icon file associated with it
   std::vector<TemplateFile> Files;

   bool operator==(const TemplateFolder& rOther) const
   { 
      return Title == rOther.Title;
   }

   bool operator<(const TemplateFolder& other) const
   {
      return Title < other.Title;
   }
};

// Use set to have sorted collection of unique folders
typedef std::set<TemplateFolder> TemplateFolderCollection;
typedef TemplateFolderCollection::iterator TemplateFolderIterator;
typedef TemplateFolderCollection::const_iterator TemplateFolderConstIterator;


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
   HICON hIcon = AfxGetApp()->LoadIcon(IDR_TXDOTOPTIONALDESIGN);
   m_TemplateGroup.SetIcon(hIcon);
   m_TemplateGroup.AddItem( new CEAFTemplateItem(this,strItemName,NULL,hIcon) );

   // Location of template folders
   CString strWorkgroupFolderName = GetTOGAFolder();

   m_TemplateGroup.Clear();

   if ( strWorkgroupFolderName.GetLength() != 0 )
   {
      //CEAFTemplateGroup* pGroup = new CEAFTemplateGroup();
      //pGroup->SetGroupName(_T("Templates"));
      //m_TemplateGroup.AddGroup(pGroup);
      FindInFolder(strWorkgroupFolderName,&m_TemplateGroup,hIcon);

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

void CTxDOTOptionalDesignDocTemplate::FindTemplateFiles(LPCTSTR strPath,CEAFTemplateGroup* pGroup,HICON origIcon)
{
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_TEMPLATE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());

   // load template files information into a data structure of folders. then we can create folder structure
   TemplateFolderCollection Folders;
   
   // Create our folder list by parsing template files
   CFileFind finder;
   CString strTemplateFileSpec = strPath + CString(_T("\\*.")) + strTemplateSuffix;
   BOOL bHasTemplateFiles = finder.FindFile(strTemplateFileSpec);
   while ( bHasTemplateFiles )
   {
      bHasTemplateFiles = finder.FindNextFile();

      CString templateFile = finder.GetFilePath();

      CString girderEntry, leftConnEntry, rightConnEntry, projectCriteriaEntry, folderName;
      if(::DoParseTemplateFile(templateFile, girderEntry, leftConnEntry, rightConnEntry, projectCriteriaEntry, folderName))
      {
         // Attempt to insert folder. Doesn't matter if insterted or not, all we want is an iterator
         TemplateFolder tfolder;
         tfolder.Title = folderName;
         std::pair<TemplateFolderIterator, bool> itfolder = Folders.insert(tfolder);

         TemplateFile file;
         file.FilePath = templateFile;
         file.FileTitle = finder.GetFileTitle();

         itfolder.first->Files.push_back(file);
      }
      else
      {
         ATLASSERT(false); // problem parsing a template file. Probably need a better way to handle this error
      }
   }

   // We have our templates arranged in folders. Now create our objects and icons
   TemplateFolderIterator itfolder = Folders.begin();
   while(itfolder != Folders.end())
   {
      const TemplateFolder& rfolder = *itfolder;

      HICON fileIcon = origIcon;

      CString strIconFile = CString(strPath) + _T("\\") + rfolder.Title.c_str() + _T(".ico");
      HICON hIcon = (HICON)::LoadImage(NULL,strIconFile,IMAGE_ICON,0,0,LR_LOADFROMFILE);
      if ( hIcon )
         fileIcon = hIcon;
      else
         ATLASSERT(false);

      CEAFTemplateGroup* pNewGroup = new CEAFTemplateGroup();
      pNewGroup->SetGroupName(rfolder.Title.c_str());
      pNewGroup->SetIcon(fileIcon);
      pGroup->AddGroup(pNewGroup);

      std::vector<TemplateFile>::const_iterator itfiles = rfolder.Files.begin();
      while(itfiles != rfolder.Files.end())
      {
         const TemplateFile rfile = *itfiles;

         CEAFTemplateItem* pItem = new CEAFTemplateItem(this,rfile.FileTitle.c_str(),rfile.FilePath.c_str(),fileIcon);
         pNewGroup->AddItem(pItem);

         itfiles++;
      }

      itfolder++;
   }
}
