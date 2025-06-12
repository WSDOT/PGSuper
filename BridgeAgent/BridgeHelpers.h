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

#ifndef INCLUDED_BRIDGEHELPERS_H_
#define INCLUDED_BRIDGEHELPERS_H_

#include <PsgLib\Keys.h>

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
GirderIDType GetSuperstructureMemberID(const CGirderKey& girderKey);

// Returns a segment key given a girder id. (reverses GetSuperstructureMemberID)
CSegmentKey GetSegmentKey(GirderIDType gdrID);

// Returns the ID of a segment layout line in the Bridge Geometry model given a girder/segment index pair
IDType GetGirderSegmentLineID(GroupIndexType grpIdx,GirderIndexType gdrIdx,SegmentIndexType segIdx);
IDType GetGirderSegmentLineID(const CSegmentKey& segmentKey);

// Returns the ID of a girder layout line in the Bridge Geometry model given a span/girder index pair
IDType GetGirderLineID(const CSpanKey& spanKey);

// Gets the superstructure member ID for a girder and for the girder to the left and right of it
// Can be called prior to bridge model validation
void GetSuperstructureMemberIDs(GroupIndexType grpIdx, GirderIndexType gdrIdx, GirderIndexType nGirders, GirderIDType* pLeftID, GirderIDType* pThisID, GirderIDType* pRightID);

// Gets the superstructure member ID for a girder and for the girder to the left and right of it
void GetAdjacentSuperstructureMemberIDs(const CGirderKey& girderKey,GirderIDType* pLeftID,GirderIDType* pThisID,GirderIDType* pRightID);

// Gets the segment keys for segments that are in the same position, but left and right of the supplied segment
void GetAdjacentSegmentKeys(const CSegmentKey& segmentKey,CSegmentKey* pLeftKey,CSegmentKey* pRightKey);

#endif // INCLUDED_BRIDGEHELPERS_H_