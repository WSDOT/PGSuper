///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

#pragma once

#include "PsgLibLib.h"
class CProjectAgentImp; // Only one privy to private conversion data

enum BearingDefinitionType {btBasic, btDetailed};
enum BearingShape {bsRectangular, bsRound};

// put arbitrary upper limit on number of bearings for UI
#define MAX_BEARING_CNT 48

class PSGLIBCLASS CBearingData2
{
public:
   BearingDefinitionType  DefinitionType; // definition of bearing. Either basic bearing details (e.g. length & width) or more detailed for design (elastomer layers, shims etc)
   BearingShape Shape;
   Float64      Length; // length along girder. used for support width when computing capacities. Before version 4, was SupportWidth in pier class
   Float64      Width;  // width normal to girder
   Uint32       BearingCount; // number of bearings per girder
   Float64      Spacing; // spacing if more than one bearing
   Float64      Height;
   Float64      RecessHeight;  // depth of recess cut into girder bottom
   Float64      RecessLength;  // length of recess cut into girder bottom
   Float64      SolePlateHeight; // thickness of sole plate attached to girder bottom (in recess)

   Float64      ElastomerThickness; // thickness of intermediate elastomer layer i
   Float64      CoverThickness;
   Float64      ShimThickness;
   IndexType    NumIntLayers;

   Float64      ShearDeformationOverride{-1};

   bool         UseExtPlates;
   bool         FixedX;
   bool         FixedY;
   


   CBearingData2();

   CBearingData2& operator=(const CBearingData2& brg);

   // set data to defaults
   void Init();

   // Get net bearing height. Height from bottom of girder to bearing seat = Height - RecessHeight + SolePlateHeight
   Float64 GetNetBearingHeight() const;

	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress) const;

   bool operator==(const CBearingData2& rOther) const; 
   bool operator!=(const CBearingData2& rOther) const;

friend CProjectAgentImp; // Only one privy to private initialization data
private:
   bool bNeedsDefaults; // Default bearing values depend on bridge data and must be initialized after bridge model is first built
};
