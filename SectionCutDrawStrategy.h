///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\Keys.h>
#include <PgsExt\PointOfInterest.h>

interface iPointDisplayObject;
interface IRoadway;
interface IBridge;
class CBridgeModelViewChildFrame;

// pure virtual class for determining cut location along girder
// no need for com here
class iCutLocation
{
public:
   // Cut locations are in the Girderline Coordinate System
   virtual Float64 GetCurrentCutLocation() = 0;
   virtual void CutAt(Float64 Xgl) = 0;
   virtual void CutAtNext() = 0;
   virtual void CutAtPrev() = 0;
   virtual void ShowCutDlg() = 0;
   virtual Float64 GetMinCutLocation() = 0;
   virtual Float64 GetMaxCutLocation() = 0;
};

// {C56878AE-5504-4f1a-A060-F2C56991663D}
DEFINE_GUID(IID_iSectionCutDrawStrategy, 
0xc56878ae, 0x5504, 0x4f1a, 0xa0, 0x60, 0xf2, 0xc5, 0x69, 0x91, 0x66, 0x3d);

interface iSectionCutDrawStrategy : public IUnknown
{
   STDMETHOD_(void,SetColor)(COLORREF color) PURE;
	STDMETHOD_(void,Init)(iPointDisplayObject* pDO, IBroker* pBroker,const CGirderKey& girderKey, iCutLocation* pCutLoc) PURE;

   // Xgl is in the Girderline Coodinate System
   STDMETHOD_(pgsPointOfInterest,GetCutPOI)(Float64 Xgl) PURE;
};

// {2CDCA9C4-A9A3-4c75-B7B1-ED9E1E308203}
DEFINE_GUID(IID_iBridgeSectionCutDrawStrategy, 
0x2cdca9c4, 0xa9a3, 0x4c75, 0xb7, 0xb1, 0xed, 0x9e, 0x1e, 0x30, 0x82, 0x3);

interface iBridgeSectionCutDrawStrategy : public IUnknown
{
   STDMETHOD_(void,SetColor)(COLORREF color) PURE;
	STDMETHOD_(void,Init)(CBridgeModelViewChildFrame* pFrame,iPointDisplayObject* pDO, IRoadway* pRoadway, IBridge* pBridge, iCutLocation* pCutLoc) PURE;
};
//
//// {F238C162-A106-4056-975A-20D3E4002512}
//DEFINE_GUID(IID_iSectionCutEvents, 
//0xf238c162, 0xa106, 0x4056, 0x97, 0x5a, 0x20, 0xd3, 0xe4, 0x0, 0x25, 0x12);
//
//interface iSectionCutEvents : public IUnknown
//{
//};
