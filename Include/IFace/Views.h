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

#pragma once

// {40E55658-360F-47d8-86DB-0AF21974BA30}
DEFINE_GUID(IID_IViews, 
0x40e55658, 0x360f, 0x47d8, 0x86, 0xdb, 0xa, 0xf2, 0x19, 0x74, 0xba, 0x30);
interface IViews : public IUnknown
{
   virtual void CreateGirderView(const CGirderKey& girderKey) = 0;
   virtual void CreateBridgeModelView() = 0;
   virtual void CreateLoadsView() = 0;
   virtual void CreateLibraryEditorView() = 0;
   virtual void CreateReportView(CollectionIndexType rptIdx,bool bPromptForSpec=true) = 0;
   virtual void BuildReportMenu(CEAFMenu* pMenu,bool bQuickReport) = 0;
   virtual void CreateGraphView(CollectionIndexType graphIdx) = 0;
   virtual void BuildGraphMenu(CEAFMenu* pMenu) = 0;
};
