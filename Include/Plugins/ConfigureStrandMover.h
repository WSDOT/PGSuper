///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

interface IShape;

DEFINE_GUID(CLSID_StrandMoverImpl, 0x5D6AFD91, 0x84F4, 0x4755, 0x9A, 0xF7, 0xB7, 0x60, 0x11, 0x4A, 0x45, 0x51);
struct __declspec(uuid("{5D6AFD91-84F4-4755-9AF7-B760114A4551}")) StramdMoverImpl;

// IStrandMover is defined in the GenericBridgeTools project. A default implementation
// is defined in Beams
// {49D89070-3BC9-4b30-97E1-496E0715F636}
DEFINE_GUID(IID_IConfigureStrandMover, 
   0x49D89070, 0x3BC9, 0x4b30, 0x97, 0xE1, 0x49, 0x6E, 0x07, 0x15, 0xF6, 0x36);
struct __declspec(uuid("{49D89070-3BC9-4b30-97E1-496E0715F636}")) IConfigureStrandMover;
interface IConfigureStrandMover : IUnknown
{
   // offset limits and default increments for harped strands
   virtual HRESULT SetHarpedStrandOffsetBounds(Float64 topGirderElevation, Float64 Hg,
      Float64 topStartElevationBoundary, Float64 botStartElevationBoundary,
      Float64 topHp1ElevationBoundary, Float64 botHp1ElevationBoundary,
      Float64 topHp2ElevationBoundary, Float64 botHp2ElevationBoundary,
      Float64 topEndElevationBoundary, Float64 botEndElevationBoundary,
      Float64 endIncrement, Float64 hpIncrement) = 0;
   
   // remove all regions
   virtual HRESULT ClearAll() = 0;
   
   // Add a shape that is to be tested for PointInShape, and arcSlope (x/y) that a strand
   // point is to be moved along
   virtual HRESULT AddRegion(IShape* shape, Float64 arcSlope) = 0;

   // methods to help debugging
   virtual HRESULT GetRegionCount(CollectionIndexType* pNum) const = 0;
   virtual HRESULT GetRegion(CollectionIndexType index, IShape** shape, Float64* arcSlope) const = 0;
};
