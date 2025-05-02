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

// TxDOTCadExporter.h : Declaration of the CTxDOTCadExporter

#pragma once

#include <EAF\ComponentObject.h>
#include <Plugins\PGSuperIEPlugin.h>
#include "CLSID.h"
#include "ExportCadData.h" 


// CTxDOTCadExporter

class CTxDOTCadExporter : public WBFL::EAF::ComponentObject,
   public PGS::IDataExporter,
   public PGS::IPluginDocumentation
{
public:
   CTxDOTCadExporter();
   ~CTxDOTCadExporter();

// IDataExporter
public:
   STDMETHOD(Init)(UINT nCmdID) override;
   CString GetMenuText() const override;
   HBITMAP GetBitmapHandle() const override;
   CString GetCommandHintText() const override;
   STDMETHOD(Export)(/*[in]*/std::shared_ptr<WBFL::EAF::Broker> pBroker) override;

// IPluginDocumentation
public:
   CString GetDocumentationSetName() const override;
   STDMETHOD(LoadDocumentationMap)() override;
   std::pair<WBFL::EAF::HelpResult,CString> GetDocumentLocation(UINT nHID) const override;

private:
   HRESULT ExportGirderDesignData(std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::vector<CGirderKey>& girderKeys, exportCADData::cdtExportDataType fileDataType, exportCADData::ctxFileFormatType fileFormat);
   HRESULT ExportHaunchDeflectionData(std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::vector<CGirderKey>& girderKeys, exportCADData::cdtExportDataType fileDataType, exportCADData::ctxFileFormatType fileFormat);

   std::map<UINT,CString> m_HelpTopics;
   CString GetDocumentationURL() const;

   CString GetExcelTemplateFolderLocation() const;

   CString m_strTemplateLocation;
};
