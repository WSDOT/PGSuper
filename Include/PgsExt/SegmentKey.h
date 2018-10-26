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

#include <PgsExt\PgsExtExp.h>
#include <PGSuperTypes.h>
#include <WBFLCore.h>

// A key to identify a girder.
// Girders are defined by their group and index
class PGSEXTCLASS CGirderKey
{
public:
   CGirderKey(GroupIndexType grpIdx,GirderIndexType gdrIdx) : groupIndex(grpIdx), girderIndex(gdrIdx) {}
   CGirderKey() : groupIndex(INVALID_INDEX), girderIndex(INVALID_INDEX) {}
   CGirderKey(const CGirderKey& other) : groupIndex(other.groupIndex), girderIndex(other.girderIndex) {}
   CGirderKey& operator=(const CGirderKey& other) { groupIndex = other.groupIndex; girderIndex = other.girderIndex; return *this;}

   bool IsEqual(const CGirderKey& other) const
   {
      if ( (groupIndex   == other.groupIndex)  && 
           (girderIndex  == other.girderIndex) )
      {
         return true;
      }

      return false;
   }

   bool operator==(const CGirderKey& other) const
   {
      if ( (groupIndex   == other.groupIndex   || groupIndex   == ALL_GROUPS)  && 
           (girderIndex  == other.girderIndex  || girderIndex  == ALL_GIRDERS) )
      {
         return true;
      }

      return false;
   }

   bool operator!=(const CGirderKey& other) const
   {
      return !operator==(other);
   }

   bool operator<(const CGirderKey& other) const
   {
      if ( groupIndex < other.groupIndex )
         return true;

      if ( other.groupIndex < groupIndex )
         return false;

      return girderIndex < other.girderIndex;
   }

   GroupIndexType   groupIndex;   // identifies a group of girders
   GirderIndexType  girderIndex;  // identifies a girder within a group

	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress)
   {
      pStrSave->BeginUnit(_T("GirderKey"),1.0);
      pStrSave->put_Property(_T("GroupIndex"),CComVariant(groupIndex));
      pStrSave->put_Property(_T("GirderIndex"),CComVariant(girderIndex));
      pStrSave->EndUnit();

      return S_OK;
   }

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
   {
      CComVariant var;
      var.vt = VT_I8;
      pStrLoad->BeginUnit(_T("GirderKey"));
      
      pStrLoad->get_Property(_T("GroupIndex"),&var);
      groupIndex = var.iVal;
      
      pStrLoad->get_Property(_T("GirderIndex"),&var);
      girderIndex = var.iVal;

      pStrLoad->EndUnit();

      return S_OK;
   }
};

// Key to identify a precast segment or a closure joint segment
class PGSEXTCLASS CSegmentKey : public CGirderKey
{
public:
   CSegmentKey(GroupIndexType grpIdx,GirderIndexType gdrIdx,SegmentIndexType segIdx) : CGirderKey(grpIdx,gdrIdx), segmentIndex(segIdx) {}
   CSegmentKey() : segmentIndex(INVALID_INDEX) {}
   CSegmentKey(const CSegmentKey& other) : CGirderKey(other), segmentIndex(other.segmentIndex) {}
   CSegmentKey(const CGirderKey& other,SegmentIndexType segIdx) : CGirderKey(other), segmentIndex(segIdx) {}
   CSegmentKey& operator=(const CSegmentKey& other) { groupIndex = other.groupIndex; girderIndex = other.girderIndex; segmentIndex = other.segmentIndex; return *this;}

   bool IsEqual(const CSegmentKey& other) const
   {
      if ( (groupIndex   == other.groupIndex)  && 
           (girderIndex  == other.girderIndex) && 
           (segmentIndex == other.segmentIndex) )
      {
         return true;
      }

      return false;
   }

   bool operator==(const CSegmentKey& other) const
   {
      if ( (groupIndex   == other.groupIndex   || groupIndex   == ALL_GROUPS)  && 
           (girderIndex  == other.girderIndex  || girderIndex  == ALL_GIRDERS) && 
           (segmentIndex == other.segmentIndex || segmentIndex == ALL_SEGMENTS) )
      {
         return true;
      }

      return false;
   }

   bool operator!=(const CSegmentKey& other) const
   {
      return !operator==(other);
   }

   bool operator<(const CSegmentKey& other) const
   {
      if ( groupIndex < other.groupIndex )
         return true;

      if ( other.groupIndex < groupIndex )
         return false;

      if ( girderIndex < other.girderIndex )
         return true;

      if ( other.girderIndex < girderIndex )
         return false;

      return segmentIndex < other.segmentIndex;
   }

   SegmentIndexType segmentIndex; // identifies a segment within a girder

	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress)
   {
      pStrSave->BeginUnit(_T("SegmentKey"),1.0);
      pStrSave->put_Property(_T("GroupIndex"),CComVariant(groupIndex));
      pStrSave->put_Property(_T("GirderIndex"),CComVariant(girderIndex));
      pStrSave->put_Property(_T("SegmentIndex"),CComVariant(segmentIndex));
      pStrSave->EndUnit();

      return S_OK;
   }

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
   {
      CComVariant var;
      var.vt = VT_I8;
      pStrLoad->BeginUnit(_T("SegmentKey"));
      
      pStrLoad->get_Property(_T("GroupIndex"),&var);
      groupIndex = var.iVal;
      
      pStrLoad->get_Property(_T("GirderIndex"),&var);
      girderIndex = var.iVal;
      
      pStrLoad->get_Property(_T("SegmentIndex"),&var);
      segmentIndex = var.iVal;

      pStrLoad->EndUnit();

      return S_OK;
   }

};

typedef CSegmentKey CClosureKey;

class PGSEXTCLASS CSpanGirderKey
{
public:
   CSpanGirderKey(SpanIndexType spanIdx,GirderIndexType gdrIdx) : spanIndex(spanIdx), girderIndex(gdrIdx) {}
   CSpanGirderKey() : spanIndex(INVALID_INDEX), girderIndex(INVALID_INDEX) {}
   CSpanGirderKey(const CSpanGirderKey& other) : spanIndex(other.spanIndex), girderIndex(other.girderIndex) {}
   
   CSpanGirderKey& operator=(const CSpanGirderKey& other) 
   { spanIndex = other.spanIndex; girderIndex = other.girderIndex; return *this;}

   bool IsEqual(const CSpanGirderKey& other) const
   {
      if ( (spanIndex    == other.spanIndex)  && 
           (girderIndex  == other.girderIndex) )
      {
         return true;
      }

      return false;
   }

   bool operator==(const CSpanGirderKey& other) const
   {
      if ( (spanIndex    == other.spanIndex    || spanIndex    == ALL_SPANS)  && 
           (girderIndex  == other.girderIndex  || girderIndex  == ALL_GIRDERS) )
      {
         return true;
      }

      return false;
   }

   bool operator!=(const CSpanGirderKey& other) const
   {
      return !operator==(other);
   }

   bool operator<(const CSpanGirderKey& other) const
   {
      if ( spanIndex < other.spanIndex )
         return true;

      if ( other.spanIndex < spanIndex )
         return false;

      return girderIndex < other.girderIndex;
   }

   SpanIndexType    spanIndex;    // identifies a span
   GirderIndexType  girderIndex;  // identifies a girder in the span

	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress)
   {
      pStrSave->BeginUnit(_T("SpanGirderKey"),1.0);
      pStrSave->put_Property(_T("SpanIndex"),CComVariant(spanIndex));
      pStrSave->put_Property(_T("GirderIndex"),CComVariant(girderIndex));
      pStrSave->EndUnit();

      return S_OK;
   }

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
   {
      CComVariant var;
      var.vt = VT_I8;
      pStrLoad->BeginUnit(_T("SpanGirderKey"));
      
      pStrLoad->get_Property(_T("SpanIndex"),&var);
      spanIndex = var.iVal;
      
      pStrLoad->get_Property(_T("GirderIndex"),&var);
      girderIndex = var.iVal;

      pStrLoad->EndUnit();

      return S_OK;
   }
};
