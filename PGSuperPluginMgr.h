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
#ifndef INCLUDED_PGSUPERIEPLUGINMGR_H_
#define INCLUDED_PGSUPERIEPLUGINMGR_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Plugins\PGSuperIEPlugin.h"

class CPGSuperPluginMgr
{
public:
   CPGSuperPluginMgr();

   bool LoadPlugins();
   void UnloadPlugins();
   Uint32 GetImporterCount();
   Uint32 GetExporterCount();
   void GetPGSuperImporter(Uint32 key,bool bByIndex,IPGSuperImporter** ppImporter);
   void GetPGSuperExporter(Uint32 key,bool bByIndex,IPGSuperExporter** ppExporter);
   UINT GetPGSuperImporterCommand(Uint32 idx);
   UINT GetPGSuperExporterCommand(Uint32 idx);

private:
   typedef std::pair<Uint32,CComPtr<IPGSuperImporter> > ImporterRecord;
   typedef std::pair<Uint32,CComPtr<IPGSuperExporter> > ExporterRecord;
   std::vector<ImporterRecord> m_ImporterPlugins;
   std::vector<ExporterRecord> m_ExporterPlugins;
};

#endif // !defined(INCLUDED_PGSUPERIEPLUGINMGR_H_)
