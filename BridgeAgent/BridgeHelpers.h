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

#ifndef INCLUDED_BRIDGEHELPERS_H_
#define INCLUDED_BRIDGEHELPERS_H_

#include <PgsExt\Keys.h>

HRESULT GetSuperstructureMember(IGenericBridge* pBridge,const CGirderKey& girderKey,ISuperstructureMember* *ssmbr);
HRESULT GetSegment(IGenericBridge* pBridge,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** segment);
HRESULT GetGirder(IGenericBridge* pBridge,const CSegmentKey& segmentKey,IPrecastGirder** girder);

// Returns the ID of a pier layout line in the Bridge Geometry model given a pier index
PierIDType GetPierLineID(PierIndexType pierIdx);
PierIndexType GetPierIndex(PierIDType pierLineID);

// Returns the ID of a temporary support layout line in the Bridge Geometry model given a temporary support index
// (Don't confuse this with the ID of a temporary support object in the CBridgeDescription model)
SupportIDType GetTempSupportLineID(SupportIndexType tsIdx);
SupportIndexType GetTempSupportIndex(SupportIDType tsLineID);

// Returns the ID of a superstructure member in the Generic Bridge Model
GirderIDType GetSuperstructureMemberID(GroupIndexType grpIdx,GirderIndexType gdrIdx);

// Returns a segment key given a girder id. (reverses GetSuperstructureMemberID)
CSegmentKey GetSegmentKey(GirderIDType gdrID);

// Returns the ID of a segment layout line in the Bridge Geometry model given a girder/segment index pair
LineIDType GetGirderSegmentLineID(GroupIndexType grpIdx,GirderIndexType gdrIdx,SegmentIndexType segIdx);
LineIDType GetGirderSegmentLineID(const CSegmentKey& segmentKey);

// Returns the ID of a girder layout line in the Bridge Geometry model given a span/girder index pair
LineIDType GetGirderLineID(const CSpanKey& spanKey);

// Gets the superstructure member ID for precast girder and for the girder to the left and right of it
void GetSuperstructureMemberIDs(GroupIndexType grpIdx,GirderIndexType gdrIdx,GirderIDType* pLeftID,GirderIDType* pThisID,GirderIDType* pRightID);

// Gets the superstructure member ID for precast girder and for the girder to the left and right of it
void GetAdjacentSuperstructureMemberIDs(const CSegmentKey& segmentKey,GirderIDType* pLeftID,GirderIDType* pThisID,GirderIDType* pRightID);

void GetAdjacentGirderKeys(const CSegmentKey& segmentKey,CSegmentKey* pLeftKey,CSegmentKey* pRightKey);

#endif // INCLUDED_BRIDGEHELPERS_H_