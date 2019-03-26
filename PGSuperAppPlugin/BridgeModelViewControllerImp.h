///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include "BridgeModelViewController.h"
#include "BridgeModelViewChildFrame.h"

// {E467D61D-4927-4B32-AE15-61EE421DEEA4}
DEFINE_GUID(CLSID_BridgeModelViewController,
   0xe467d61d, 0x4927, 0x4b32, 0xae, 0x15, 0x61, 0xee, 0x42, 0x1d, 0xee, 0xa4);

class ATL_NO_VTABLE CBridgeModelViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CBridgeModelViewController, &CLSID_BridgeModelViewController>,
   public IBridgeModelViewController
{
public:
   CBridgeModelViewController();
   virtual ~CBridgeModelViewController();

   void Init(CBridgeModelViewChildFrame* pFrame,IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CBridgeModelViewController)
      COM_INTERFACE_ENTRY(IBridgeModelViewController)
      COM_INTERFACE_ENTRY(IEAFViewController)
   END_COM_MAP()

// IEAFViewController
public:
   virtual bool IsOpen() const override;

   // Closes the view
   virtual void Close() override;

   // implement later....
   //virtual void Move(INT x, INT y) = 0;
   //virtual void Size(INT x, INT y) = 0;
   //virtual void GetSize() = 0;
   //virtual void GetPosition() = 0;
   //virtual void GetState() = 0;
   virtual void Minimize() override;
   virtual void Maximize() override;
   virtual void Restore() override;

// IBridgeModelViewController
public:
   // Other methods... Print(bUseDefault/bPrompt), SelectGirder, SelectDeck, SelectAlignment, SelectRailingSystem, Delete, Insert....
   // Set the top/bottom split screen fraction (50/50, 60/40...)
   virtual void GetSpanRange(SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) const override;
   virtual void SetSpanRange(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx) override;
   virtual Float64 GetCutStation() const override;
   virtual void SetCutStation(Float64 station) override;
   virtual void SetViewMode(IBridgeModelViewController::ViewMode mode) override;
   virtual IBridgeModelViewController::ViewMode GetViewMode() const override;
   virtual void NorthUp(bool bNorthUp) override;
   virtual bool NorthUp() const override;
   virtual void ShowLabels(bool bShowLabels) override;
   virtual bool ShowLabels() const override;
   virtual void ShowDimensions(bool bShowDimensions) override;
   virtual bool ShowDimensions() const override;
   virtual void ShowBridge(bool bShowBridge) override;
   virtual bool ShowBridge() const override;
   virtual void Schematic(bool bSchematic) override;
   virtual bool Schematic() const override;

private:
   CBridgeModelViewChildFrame* m_pFrame;
   CComPtr<IEAFViewController> m_pStdController;
};

