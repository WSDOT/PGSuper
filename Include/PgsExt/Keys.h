///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <PgsExt\PgsExtExp.h>
#include <WBFLCore.h>

// A key to identify a girder.
// Girders are defined by their group and index
class PGSEXTCLASS CGirderKey
{
public:
   CGirderKey(GroupIndexType grpIdx,GirderIndexType gdrIdx);
   CGirderKey();
   CGirderKey(const CGirderKey& other);
   CGirderKey& operator=(const CGirderKey& other);

   bool IsEqual(const CGirderKey& other) const;
   bool operator==(const CGirderKey& other) const;
   bool operator!=(const CGirderKey& other) const;
   bool operator<(const CGirderKey& other) const;

   GroupIndexType   groupIndex;   // identifies a group of girders
   GirderIndexType  girderIndex;  // identifies a girder within a group

	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
};

// Key to identify a precast segment or a closure joint segment
class PGSEXTCLASS CSegmentKey : public CGirderKey
{
public:
   CSegmentKey(GroupIndexType grpIdx,GirderIndexType gdrIdx,SegmentIndexType segIdx);
   CSegmentKey();
   CSegmentKey(const CSegmentKey& other);
   CSegmentKey(const CGirderKey& other,SegmentIndexType segIdx);
   CSegmentKey& operator=(const CSegmentKey& other);

   bool IsEqual(const CSegmentKey& other) const;
   bool operator==(const CSegmentKey& other) const;
   bool operator!=(const CSegmentKey& other) const;
   bool operator<(const CSegmentKey& other) const;

   SegmentIndexType segmentIndex; // identifies a segment within a girder

	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
};

typedef CSegmentKey CClosureKey;

class PGSEXTCLASS CSpanKey
{
public:
   CSpanKey(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   CSpanKey();
   CSpanKey(const CSpanKey& other);
   
   CSpanKey& operator=(const CSpanKey& other);
   bool IsEqual(const CSpanKey& other) const;
   bool operator==(const CSpanKey& other) const;
   bool operator!=(const CSpanKey& other) const;
   bool operator<(const CSpanKey& other) const;

   SpanIndexType    spanIndex;    // identifies a span
   GirderIndexType  girderIndex;  // identifies a girder in the span

	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
};

class PGSEXTCLASS CTendonKey
{
public:
   CTendonKey(const CGirderKey& girderKey,DuctIndexType ductIdx);
   CTendonKey(GirderIDType girderID,DuctIndexType ductIdx);
   CTendonKey(const CTendonKey& other);
   CTendonKey& operator=(const CTendonKey& other);
   bool operator==(const CTendonKey& other) const;
   bool operator<(const CTendonKey& other) const;

   GirderIDType girderID;
   CGirderKey girderKey;
   DuctIndexType ductIdx;
};


#define ASSERT_GIRDER_KEY(_g_) ATLASSERT(_g_.groupIndex != ALL_GROUPS && _g_.girderIndex != ALL_GIRDERS)
#define ASSERT_SEGMENT_KEY(_s_) ATLASSERT(_s_.groupIndex != ALL_GROUPS && _s_.girderIndex != ALL_GIRDERS && _s_.segmentIndex != ALL_SEGMENTS)
#define ASSERT_CLOSURE_KEY(_c_) ASSERT_SEGMENT_KEY(_c_)
#define ASSERT_SPAN_KEY(_sg_) ATLASSERT(_sg_.spanIndex != ALL_SPANS && _sg_.girderIndex != ALL_GIRDERS)
