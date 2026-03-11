///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
// PGSuperExporter.h : Declaration of the CPGSuperExporter

#pragma once

#include <Plugins/PGSuperIEPlugin.h>
#include <PsgLib\BridgeDescription2.h>
#include <IFace\AnalysisResults.h>
#include "KDOTExporter.hxx"
#include <EAF\ComponentObject.h>

class IShapes;
class LiveLoadLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CKDOTDataExporter
class CKDOTDataExporter : public WBFL::EAF::ComponentObject,
   public PGS::IDataExporter,
   public PGS::IPluginDocumentation
{
public:
   CKDOTDataExporter();

   CBitmap m_bmpLogo;

// IDataExporter
public:
   HRESULT Init(UINT nCmdID) override;
   CString GetMenuText() const override;
   HBITMAP GetBitmapHandle() const override;
   CString GetCommandHintText() const override;
   HRESULT Export(std::shared_ptr<WBFL::EAF::Broker> pBroker) override;

// IPGSuperDocumentation
public:
   CString GetDocumentationSetName() const override;
   STDMETHOD(LoadDocumentationMap)() override;
   std::pair<WBFL::EAF::HelpResult,CString> GetDocumentLocation(UINT nHID) const override;

private:
   Float64 m_BearingHeight;
   HRESULT Export(std::shared_ptr<WBFL::EAF::Broker> pBroker,CString& strFileName, const std::vector<CGirderKey>& girderKeys);

   std::map<UINT,CString> m_HelpTopics;
   CString GetDocumentationURL() const;
};
