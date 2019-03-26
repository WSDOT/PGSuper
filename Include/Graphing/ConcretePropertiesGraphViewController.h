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

#include <EAF\EAFViewController.h>

// {E5C34BE8-A04B-475E-862F-62630FF984C8}
DEFINE_GUID(IID_IConcretePropertiesGraphViewController ,
   0xe5c34be8, 0xa04b, 0x475e, 0x86, 0x2f, 0x62, 0x63, 0xf, 0xf9, 0x84, 0xc8);
struct __declspec(uuid("{E5C34BE8-A04B-475E-862F-62630FF984C8}")) IConcretePropertiesGraphViewController;

interface IConcretePropertiesGraphViewController : IEAFViewController
{
   enum GraphType {Fc, Ec, Shrinkage, Creep};
   enum ElementType { Segment, Closure, Deck};
   enum XAxisType {TimeLinear, TimeLog, AgeLinear, AgeLog, Interval};

   virtual void SetGraphType(GraphType type) = 0;
   virtual GraphType GetGraphType() const = 0;

   virtual void SetElementType(ElementType type) = 0;
   virtual ElementType GetElementType() const = 0;

   virtual void SetXAxisType(XAxisType type) = 0;
   virtual XAxisType GetXAxisType() const = 0;

   virtual void SetSegment(const CSegmentKey& segmentKey) = 0;
   virtual const CSegmentKey& GetSegment() const = 0;

   virtual void SetClosureJoint(const CClosureKey& closureKey) = 0;
   virtual const CClosureKey& GetClosureJoint() const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;
};
