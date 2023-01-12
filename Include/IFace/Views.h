///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

class CEAFMenu;

interface IEAFViewController;
interface IBridgeModelViewController;
interface IGirderModelViewController;
interface ILoadsViewController;


// {40E55658-360F-47d8-86DB-0AF21974BA30}
DEFINE_GUID(IID_IViews, 
0x40e55658, 0x360f, 0x47d8, 0x86, 0xdb, 0xa, 0xf2, 0x19, 0x74, 0xba, 0x30);
interface IViews : public IUnknown
{
   virtual void CreateBridgeModelView(IBridgeModelViewController** ppViewController=nullptr) = 0;
   virtual void CreateGirderView(const CGirderKey& girderKey, IGirderModelViewController** ppViewController = nullptr) = 0;
   virtual void CreateLoadsView(ILoadsViewController** ppViewController=nullptr) = 0;
   virtual void CreateLibraryEditorView() = 0;
   virtual void CreateReportView(CollectionIndexType rptIdx,BOOL bPromptForSpec=TRUE) = 0;
   virtual void CreateGraphView(CollectionIndexType graphIdx, IEAFViewController** ppViewController = nullptr) = 0;
   virtual void CreateGraphView(LPCTSTR lpszGraph, IEAFViewController** ppViewController = nullptr) = 0;

   virtual void BuildReportMenu(CEAFMenu* pMenu, bool bQuickReport) = 0;
   virtual void BuildGraphMenu(CEAFMenu* pMenu) = 0;

   virtual long GetBridgeModelEditorViewKey() = 0;
   virtual long GetGirderModelEditorViewKey() = 0;
   virtual long GetLibraryEditorViewKey() = 0;
   virtual long GetReportViewKey() = 0;
   virtual long GetGraphingViewKey() = 0;
   virtual long GetLoadsViewKey() = 0;
};
