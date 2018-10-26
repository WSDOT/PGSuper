///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

/////////////////////////////////////////////////////////////////////////////////////////
// User interface extension callback interfaces are defined in this header file. 
//
// These are prototype interfaces. That means they are subject to change. It is unclear
// as to exactly which events extenions need to receive and how they are to be broadcast.
//
// Be warned that these interfaces may change in the future and your extension agents
// are likely to break.
//

// Extend the Edit Pier Dialog
interface IEditPierData
{
   virtual pgsTypes::PierConnectionType GetConnectionType() = 0;
   virtual GirderIndexType GetGirderCount(pgsTypes::PierFaceType face) = 0;
};

interface IEditPierCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pPierData) = 0;
   virtual void DestroyPropertyPage(INT_PTR result,CPropertyPage* pPage,IEditPierData* pPierData) = 0;
};

// Extend the Edit Span Dialog
interface IEditSpanData
{
   virtual pgsTypes::PierConnectionType GetConnectionType(pgsTypes::MemberEndType end) = 0;
   virtual GirderIndexType GetGirderCount() = 0;
};

interface IEditSpanCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditSpanData* pSpanData) = 0;
   virtual void DestroyPropertyPage(INT_PTR result,CPropertyPage* pPage,IEditSpanData* pSpanData) = 0;
};

// Extend the Edit Girder Dialog
interface IEditGirderData
{
   virtual void EGDummy() = 0;
};

interface IEditGirderCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditGirderData* pGirderData) = 0;
   virtual void DestroyPropertyPage(INT_PTR result,CPropertyPage* pPage,IEditGirderData* pGirderData) = 0;
};

// Extend the Edit Bridge Dialog
interface IEditBridgeData
{
   virtual void EBDummy() = 0;
};

interface IEditBridgeCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditBridgeData* pBridgeData) = 0;
   virtual void DestroyPropertyPage(INT_PTR result,CPropertyPage* pPage,IEditBridgeData* pBridgeData) = 0;
};


/////////////////////////////////////////////////////////
// IExtendUI
// Use this interface to register callbacks for user interface extensions

// {F477FBFC-2C57-42bf-8FB5-A32296087B64}
DEFINE_GUID(IID_IExtendUI, 
0xf477fbfc, 0x2c57, 0x42bf, 0x8f, 0xb5, 0xa3, 0x22, 0x96, 0x8, 0x7b, 0x64);
struct __declspec(uuid("{F477FBFC-2C57-42bf-8FB5-A32296087B64}")) IExtendUI;
interface IExtendUI : IUnknown
{
   virtual IDType RegisterEditPierCallback(IEditPierCallback* pCallback) = 0;
   virtual IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback) = 0;
   virtual IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback) = 0;
   virtual IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback) = 0;

   virtual bool UnregisterEditPierCallback(IDType ID) = 0;
   virtual bool UnregisterEditSpanCallback(IDType ID) = 0;
   virtual bool UnregisterEditGirderCallback(IDType ID) = 0;
   virtual bool UnregisterEditBridgeCallback(IDType ID) = 0;
};