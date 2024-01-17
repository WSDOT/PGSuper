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

// PGSuperTemplateManager.h
//

#pragma once

#include <afxinet.h>

// Base class for downloading PGSuper Project Templates from a catalog server

class CPGSuperTemplateManager
{
public:
	CPGSuperTemplateManager(LPCTSTR strExt,CHAR chSep); // chSep is the folder separator chacter

   // Recursively copies PTG template files and associated icon files from the source to the destination
   void GetTemplates(const CString& strSourcePath,const CString& strRootDestinationPath,IProgressMonitor* pProgress);

protected:
   // Create the type of CFileFind object needed for the data source
   virtual CFileFind* CreateFileFinder() = 0;

   // Get the file from the data source
   virtual BOOL GetFile(const CString& strSource,const CString& strDestination) = 0;

private:
   void GetTemplateFiles(const CString& strSourcePath,const CString& strDestinationPath,IProgressMonitor* pProgress);

protected:
   CHAR m_cFolderSeparator;

   CString m_strExt;
};

////////////////////////////////////////////////////////////////
// Template manager for templates stored on a file system such
// as a local disk, network disk, or UNC
class CFileTemplateManager : public CPGSuperTemplateManager
{
public:
   CFileTemplateManager(LPCTSTR strExt);

protected:
   virtual CFileFind* CreateFileFinder() override;
   virtual BOOL GetFile(const CString& strSource,const CString& strDestination) override;
};

////////////////////////////////////////////////////////////////
// Template manager for templates stored on an FTP server
class CFTPTemplateManager : public CPGSuperTemplateManager
{
public:
   CFTPTemplateManager(LPCTSTR strExt,CFtpConnection* pFTP);

protected:
   virtual CFileFind* CreateFileFinder() override;
   virtual BOOL GetFile(const CString& strSource,const CString& strDestination) override;

private:
   CFtpConnection* m_pFTP;
};
