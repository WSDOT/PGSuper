///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

interface IRoadway;
interface IBridge;
class CBridgeModelViewChildFrame;

namespace WBFL
{
   namespace DManip
   {
      class iPointDisplayObject;
   };
};

// pure virtual class for determining cut location
class iCutLocation
{
public:
   // Cut location coordinate system depends on implementation
   virtual Float64 GetCurrentCutLocation() = 0;
   virtual void CutAt(Float64 X) = 0;
   virtual void CutAtNext() = 0;
   virtual void CutAtPrev() = 0;
   virtual void ShowCutDlg() = 0;
   virtual void GetCutRange(Float64* pMin, Float64* pMax) = 0;
};

class iSectionCutDrawStrategy
{
public:
   virtual void SetColor(COLORREF color) = 0;
	virtual void Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, IBroker* pBroker,const CGirderKey& girderKey, iCutLocation* pCutLoc) = 0;

   // Xgl is in the Girderline Coordinate System
   virtual pgsPointOfInterest GetCutPOI(Float64 Xgl) const = 0;
};

class iBridgeSectionCutDrawStrategy
{
public:
   virtual void SetColor(COLORREF color) = 0;
	virtual void Init(CBridgeModelViewChildFrame* pFrame,std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, IRoadway* pRoadway, IBridge* pBridge, iCutLocation* pCutLoc) = 0;
};
