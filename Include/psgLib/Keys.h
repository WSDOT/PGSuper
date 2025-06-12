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
#include "PGSuperTypes.h"
#include <memory>

class IEAFProgress;

// A key to identify a girder.
// Girders are defined by their group and index
class PSGLIBCLASS CGirderKey
{
public:
   CGirderKey(GroupIndexType grpIdx,GirderIndexType gdrIdx);
   CGirderKey() = default;
   CGirderKey(const CGirderKey& other) = default;
   CGirderKey& operator=(const CGirderKey& other) = default;

   bool IsEqual(const CGirderKey& other) const;
   bool operator==(const CGirderKey& other) const;
   bool operator!=(const CGirderKey& other) const;
   bool operator<(const CGirderKey& other) const;

   GroupIndexType   groupIndex = INVALID_INDEX;   // identifies a group of girders
   GirderIndexType  girderIndex = INVALID_INDEX;  // identifies a girder within a group

	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
};

// Key to identify a precast segment or a closure joint segment
class PSGLIBCLASS CSegmentKey : public CGirderKey
{
public:
   CSegmentKey(GroupIndexType grpIdx,GirderIndexType gdrIdx,SegmentIndexType segIdx);
   CSegmentKey() = default;
   CSegmentKey(const CSegmentKey& other) = default;
   CSegmentKey(const CGirderKey& other,SegmentIndexType segIdx);
   CSegmentKey& operator=(const CSegmentKey& other) = default;

   bool IsEqual(const CSegmentKey& other) const;
   bool operator==(const CSegmentKey& other) const;
   bool operator!=(const CSegmentKey& other) const;
   bool operator<(const CSegmentKey& other) const;

   SegmentIndexType segmentIndex = INVALID_INDEX; // identifies a segment within a girder

	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
};

using CClosureKey = CSegmentKey;

class PSGLIBCLASS CSpanKey
{
public:
   CSpanKey(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   CSpanKey() = default;
   CSpanKey(const CSpanKey& other) = default;
   
   CSpanKey& operator=(const CSpanKey& other) = default;
   bool IsEqual(const CSpanKey& other) const;
   bool operator==(const CSpanKey& other) const;
   bool operator!=(const CSpanKey& other) const;
   bool operator<(const CSpanKey& other) const;

   SpanIndexType    spanIndex = INVALID_INDEX;    // identifies a span
   GirderIndexType  girderIndex = INVALID_INDEX;  // identifies a girder in the span

	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
};

class PSGLIBCLASS CGirderTendonKey
{
public:
   CGirderTendonKey(const CGirderKey& girderKey,DuctIndexType ductIdx);
   CGirderTendonKey(GirderIDType girderID,DuctIndexType ductIdx);
   CGirderTendonKey(const CGirderTendonKey& other) = default;
   CGirderTendonKey& operator=(const CGirderTendonKey& other) = default;
   bool operator==(const CGirderTendonKey& other) const;
   bool operator<(const CGirderTendonKey& other) const;

   GirderIDType girderID = INVALID_ID;
   CGirderKey girderKey;
   DuctIndexType ductIdx = INVALID_INDEX;
};

class PSGLIBCLASS CSegmentTendonKey
{
public:
   CSegmentTendonKey(const CSegmentKey& segmentKey, DuctIndexType ductIdx);
   CSegmentTendonKey(SegmentIDType segmentID, DuctIndexType ductIdx);
   CSegmentTendonKey(const CSegmentTendonKey& other) = default;
   CSegmentTendonKey& operator=(const CSegmentTendonKey& other) = default;
   bool operator==(const CSegmentTendonKey& other) const;
   bool operator<(const CSegmentTendonKey& other) const;

   SegmentIDType segmentID = INVALID_ID;
   CSegmentKey segmentKey;
   DuctIndexType ductIdx = INVALID_INDEX;
};


#define ASSERT_GIRDER_KEY(_g_) ATLASSERT(_g_.groupIndex != ALL_GROUPS && _g_.girderIndex != ALL_GIRDERS)
#define ASSERT_SEGMENT_KEY(_s_) ATLASSERT(_s_.groupIndex != ALL_GROUPS && _s_.girderIndex != ALL_GIRDERS && _s_.segmentIndex != ALL_SEGMENTS)
#define ASSERT_CLOSURE_KEY(_c_) ASSERT_SEGMENT_KEY(_c_)
#define ASSERT_SPAN_KEY(_sg_) ATLASSERT(_sg_.spanIndex != ALL_SPANS && _sg_.girderIndex != ALL_GIRDERS)
