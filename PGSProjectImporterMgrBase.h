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
#pragma once

#include "Plugins\PGSuperIEPlugin.h"
#include <EAF\EAFUtilities.h>
#include <set>

class CPGSProjectImporterMgrBase
{
public:
   CPGSProjectImporterMgrBase();
   virtual ~CPGSProjectImporterMgrBase();

   bool LoadImporters();
   void UnloadImporters();
   CollectionIndexType GetImporterCount() const;
   void GetImporter(CollectionIndexType idx,IPGSProjectImporter** ppImporter);
   void GetImporter(const CLSID& clsid,IPGSProjectImporter** ppImporter);
   void AddImporter(const CLSID& clsid,IPGSProjectImporter* pImporter);
   void RemoveImporter(const CLSID& clsid);

   virtual LPCTSTR GetAppName() = 0;

protected:
   virtual CATID GetProjectImporterCATID() = 0;

private:
   struct Record
   { 
      CLSID clsid; CComPtr<IPGSProjectImporter> Importer;
      Record() {}
      Record(const Record& other) 
      {
         clsid    = other.clsid;
         Importer = other.Importer;
      }
      Record& operator=(const Record& other)
      {
         clsid    = other.clsid;
         Importer = other.Importer;
         return *this;
      }
      bool operator<(const Record& other) const
      {
         return clsid < other.clsid;
      }
   };

   std::set<Record> m_ImporterRecords;
};
