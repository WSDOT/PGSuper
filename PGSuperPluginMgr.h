///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#ifndef INCLUDED_PGSUPERIEPLUGINMGR_H_
#define INCLUDED_PGSUPERIEPLUGINMGR_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Plugins\PGSuperIEPlugin.h"

class CPGSuperPluginMgrBase
{
public:
   CPGSuperPluginMgrBase();

   bool LoadPlugins();
   void UnloadPlugins();
   CollectionIndexType GetImporterCount();
   CollectionIndexType GetExporterCount();
   void GetImporter(CollectionIndexType key,bool bByIndex,IPGSDataImporter** ppImporter);
   void GetExporter(CollectionIndexType key,bool bByIndex,IPGSDataExporter** ppExporter);
   UINT GetImporterCommand(CollectionIndexType idx);
   UINT GetExporterCommand(CollectionIndexType idx);
   const CBitmap* GetImporterBitmap(CollectionIndexType idx);
   const CBitmap* GetExporterBitmap(CollectionIndexType idx);
   void LoadDocumentationMaps();
   eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nHID,CString& strURL);

protected:
   virtual CATID GetImporterCATID() = 0;
   virtual CATID GetExporterCATID() = 0;

private:
   template <class T> 
   struct Record
   { UINT commandID; CBitmap Bitmap; CComPtr<T> Plugin; 
   Record() {}
   Record(const Record& other) 
   {
      Bitmap.Detach();

      CBitmap* pBmp = const_cast<CBitmap*>(&(other.Bitmap));
      Bitmap.Attach(pBmp->Detach());

      commandID = other.commandID;
      Plugin = other.Plugin;
   }
   Record& operator=(const Record& other)
   {
      Bitmap.Detach();

      CBitmap* pBmp = const_cast<CBitmap*>(&(other.Bitmap));
      Bitmap.Attach(pBmp->Detach());

      commandID = other.commandID;
      Plugin = other.Plugin;
      return *this;
   }
   };

   typedef Record<IPGSDataImporter> ImporterRecord;
   typedef Record<IPGSDataExporter> ExporterRecord;

   std::vector<ImporterRecord> m_ImporterPlugins;
   std::vector<ExporterRecord> m_ExporterPlugins;
};

class CPGSuperPluginMgr : public CPGSuperPluginMgrBase
{
protected:
   virtual CATID GetImporterCATID() {return CATID_PGSuperDataImporter;}
   virtual CATID GetExporterCATID() {return CATID_PGSuperDataExporter;}
};

class CPGSplicePluginMgr : public CPGSuperPluginMgrBase
{
protected:
   virtual CATID GetImporterCATID() {return CATID_PGSpliceDataImporter;}
   virtual CATID GetExporterCATID() {return CATID_PGSpliceDataExporter;}
};

#endif // !defined(INCLUDED_PGSUPERIEPLUGINMGR_H_)
