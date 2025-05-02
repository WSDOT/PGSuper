///////////////////////////////////////////////////////////////////////
// IEPluginExample
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

// PGSuperProjectImporter.h : Declaration of the CPGSuperProjectImporter

#pragma once

#include <Plugins\PGSuperIEPlugin.h>
#include <EAF\ComponentObject.h>

class CBridgeDescription2;

class CPGSuperProjectImporter : public WBFL::EAF::ComponentObject, 
   public PGS::IProjectImporter
{
public:
   CPGSuperProjectImporter();

   // IProjectImporter
public:
   CString GetItemText() const override;
   HICON GetIcon() const override;
   HRESULT Import(std::shared_ptr<WBFL::EAF::Broker> pBroker) override;
   CLSID GetCLSID() const override;

private:
   void BuildBridge(std::shared_ptr<WBFL::EAF::Broker> pBroker);
   void SetSpecification(std::shared_ptr<WBFL::EAF::Broker> pBroker);
   void InitGirderData(std::shared_ptr<WBFL::EAF::Broker> pBroker);
   void InitTimelineManager(std::shared_ptr<WBFL::EAF::Broker> pBroker, CBridgeDescription2& bridge);

   CBitmap m_Bitmap;
};
