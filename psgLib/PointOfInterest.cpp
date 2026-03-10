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

#include "StdAfx.h"
#include <PsgLib\PointOfInterest.h>
#include <PsgLib\GirderLabel.h>

#include <iterator> // for std::back_inserter


#if defined _DEBUG || defined _BETA_VERSION
#define UPDATE_ATTRIBUTES UpdateAttributeString()
#else
#define UPDATE_ATTRIBUTES
#endif

// this defines the temporal order of the referenced attributes
static PoiAttributeType gs_RefAttributes[] = { 
   POI_RELEASED_SEGMENT, 
   POI_LIFT_SEGMENT, 
   POI_STORAGE_SEGMENT,
   POI_HAUL_SEGMENT, 
   POI_ERECTED_SEGMENT, 
   POI_SPAN 
};

static IndexType gs_nRefAttributes = sizeof(gs_RefAttributes)/sizeof(gs_RefAttributes[0]);

#if defined _DEBUG || defined _BETA_VERSION

static LPCTSTR gs_strRefAttributes[] = { 
   _T("POI_RELEASED_SEGMENT"), 
   _T("POI_LIFT_SEGMENT"), 
   _T("POI_STORAGE_SEGMENT"),
   _T("POI_HAUL_SEGMENT"), 
   _T("POI_ERECTED_SEGMENT"), 
   _T("POI_SPAN") 
};
#endif

static LPCTSTR  gs_str10thPointLabel_Release[] = {_T("err"),_T("0.0Lr"),_T("0.1Lr"),_T("0.2Lr"),_T("0.3Lr"),_T("0.4Lr"),_T("0.5Lr"),_T("0.6Lr"),_T("0.7Lr"),_T("0.8Lr"),_T("0.9Lr"),_T("1.0Lr")};
static LPCTSTR  gs_str10thPointLabel_Lift[]    = {_T("err"),_T("0.0Ll"),_T("0.1Ll"),_T("0.2Ll"),_T("0.3Ll"),_T("0.4Ll"),_T("0.5Ll"),_T("0.6Ll"),_T("0.7Ll"),_T("0.8Ll"),_T("0.9Ll"),_T("1.0Ll")};
static LPCTSTR  gs_str10thPointLabel_Storage[] = {_T("err"),_T("0.0Lst"),_T("0.1Lst"),_T("0.2Lst"),_T("0.3Lst"),_T("0.4Lst"),_T("0.5Lst"),_T("0.6Lst"),_T("0.7Lst"),_T("0.8Lst"),_T("0.9Lst"),_T("1.0Lst")};
static LPCTSTR  gs_str10thPointLabel_Haul[]    = {_T("err"),_T("0.0Lh"),_T("0.1Lh"),_T("0.2Lh"),_T("0.3Lh"),_T("0.4Lh"),_T("0.5Lh"),_T("0.6Lh"),_T("0.7Lh"),_T("0.8Lh"),_T("0.9Lh"),_T("1.0Lh")};
static LPCTSTR  gs_str10thPointLabel_Erect[]   = {_T("err"),_T("0.0Le"),_T("0.1Le"),_T("0.2Le"),_T("0.3Le"),_T("0.4Le"),_T("0.5Le"),_T("0.6Le"),_T("0.7Le"),_T("0.8Le"),_T("0.9Le"),_T("1.0Le")};
static LPCTSTR  gs_str10thPointLabel_Span[]    = {_T("err"),_T("0.0Ls"),_T("0.1Ls"),_T("0.2Ls"),_T("0.3Ls"),_T("0.4Ls"),_T("0.5Ls"),_T("0.6Ls"),_T("0.7Ls"),_T("0.8Ls"),_T("0.9Ls"),_T("1.0Ls")};
static LPCTSTR* gs_str10thPointLabel[] = 
{
   gs_str10thPointLabel_Release,
   gs_str10thPointLabel_Lift,
   gs_str10thPointLabel_Storage,
   gs_str10thPointLabel_Haul,
   gs_str10thPointLabel_Erect,
   gs_str10thPointLabel_Span,
};

static LPCTSTR  gs_str10thPointLabelWithMarkup_Release[] = {_T("err"),_T("0.0L<sub>r</sub>"),_T("0.1L<sub>r</sub>"),_T("0.2L<sub>r</sub>"),_T("0.3L<sub>r</sub>"),_T("0.4L<sub>r</sub>"),_T("0.5L<sub>r</sub>"),_T("0.6L<sub>r</sub>"),_T("0.7L<sub>r</sub>"),_T("0.8L<sub>r</sub>"),_T("0.9L<sub>r</sub>"),_T("1.0L<sub>r</sub>")};
static LPCTSTR  gs_str10thPointLabelWithMarkup_Lift[]    = {_T("err"),_T("0.0L<sub>l</sub>"),_T("0.1L<sub>l</sub>"),_T("0.2L<sub>l</sub>"),_T("0.3L<sub>l</sub>"),_T("0.4L<sub>l</sub>"),_T("0.5L<sub>l</sub>"),_T("0.6L<sub>l</sub>"),_T("0.7L<sub>l</sub>"),_T("0.8L<sub>l</sub>"),_T("0.9L<sub>l</sub>"),_T("1.0L<sub>l</sub>")};
static LPCTSTR  gs_str10thPointLabelWithMarkup_Storage[] = {_T("err"),_T("0.0L<sub>st</sub>"),_T("0.1L<sub>st</sub>"),_T("0.2L<sub>st</sub>"),_T("0.3L<sub>st</sub>"),_T("0.4L<sub>st</sub>"),_T("0.5L<sub>st</sub>"),_T("0.6L<sub>st</sub>"),_T("0.7L<sub>st</sub>"),_T("0.8L<sub>st</sub>"),_T("0.9L<sub>st</sub>"),_T("1.0L<sub>st</sub>")};
static LPCTSTR  gs_str10thPointLabelWithMarkup_Haul[]    = {_T("err"),_T("0.0L<sub>h</sub>"),_T("0.1L<sub>h</sub>"),_T("0.2L<sub>h</sub>"),_T("0.3L<sub>h</sub>"),_T("0.4L<sub>h</sub>"),_T("0.5L<sub>h</sub>"),_T("0.6L<sub>h</sub>"),_T("0.7L<sub>h</sub>"),_T("0.8L<sub>h</sub>"),_T("0.9L<sub>h</sub>"),_T("1.0L<sub>h</sub>")};
static LPCTSTR  gs_str10thPointLabelWithMarkup_Erect[]   = {_T("err"),_T("0.0L<sub>e</sub>"),_T("0.1L<sub>e</sub>"),_T("0.2L<sub>e</sub>"),_T("0.3L<sub>e</sub>"),_T("0.4L<sub>e</sub>"),_T("0.5L<sub>e</sub>"),_T("0.6L<sub>e</sub>"),_T("0.7L<sub>e</sub>"),_T("0.8L<sub>e</sub>"),_T("0.9L<sub>e</sub>"),_T("1.0L<sub>e</sub>")};
static LPCTSTR  gs_str10thPointLabelWithMarkup_Span[]    = {_T("err"),_T("0.0L<sub>s</sub>"),_T("0.1L<sub>s</sub>"),_T("0.2L<sub>s</sub>"),_T("0.3L<sub>s</sub>"),_T("0.4L<sub>s</sub>"),_T("0.5L<sub>s</sub>"),_T("0.6L<sub>s</sub>"),_T("0.7L<sub>s</sub>"),_T("0.8L<sub>s</sub>"),_T("0.9L<sub>s</sub>"),_T("1.0L<sub>s</sub>")};
static LPCTSTR* gs_str10thPointLabelWithMarkup[] =
{
   gs_str10thPointLabelWithMarkup_Release,
   gs_str10thPointLabelWithMarkup_Lift,
   gs_str10thPointLabelWithMarkup_Storage,
   gs_str10thPointLabelWithMarkup_Haul,
   gs_str10thPointLabelWithMarkup_Erect,
   gs_str10thPointLabelWithMarkup_Span,
};

/****************************************************************************
CLASS
   pgsPointOfInterest
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPointOfInterest::pgsPointOfInterest():
m_ID(INVALID_ID),
m_Xpoi(-1),
m_bHasSegmentPathCoordinate(false),
m_Xsp(-1),
m_bHasGirderCoordinate(false),
m_Xg(-1),
m_bHasGirderPathCoordinate(false),
m_Xgp(-1),
m_bHasGirderlineCoordinate(false),
m_Xgl(-1),
m_SpanIdx(INVALID_INDEX),
m_Xspan(-1),
m_bHasSpanPoint(false),
m_DeckCastingRegionIdx(INVALID_INDEX),
m_bHasDeckCastingRegion(false),
m_Attributes(0)
{
   m_RefAttributes.fill(0);
   UPDATE_ATTRIBUTES;
}

pgsPointOfInterest::pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart,PoiAttributeType attrib) :
m_ID(INVALID_ID),
m_SegmentKey(segmentKey),
m_Xpoi(distFromStart),
m_bHasSegmentPathCoordinate(false),
m_Xsp(-1),
m_bHasGirderCoordinate(false),
m_Xg(-1),
m_bHasGirderPathCoordinate(false),
m_Xgp(-1),
m_bHasGirderlineCoordinate(false),
m_Xgl(-1),
m_SpanIdx(INVALID_INDEX),
m_Xspan(-1),
m_bHasSpanPoint(false),
m_DeckCastingRegionIdx(INVALID_INDEX),
m_bHasDeckCastingRegion(false),
m_Attributes(0)
{
   m_RefAttributes.fill(0);

   IndexType index = GetIndex(GetReference(attrib));
   if ( index == INVALID_INDEX )
   {
      m_Attributes = attrib;
   }
   else
   {
      m_RefAttributes[index] = attrib;
   }

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

pgsPointOfInterest::pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart,Float64 Xsp,Float64 Xg,Float64 Xgp,PoiAttributeType attrib) :
m_ID(INVALID_ID),
m_SegmentKey(segmentKey),
m_Xpoi(distFromStart),
m_bHasSegmentPathCoordinate(true),
m_Xsp(Xsp),
m_bHasGirderCoordinate(true),
m_Xg(Xg),
m_bHasGirderPathCoordinate(true),
m_Xgp(Xgp),
m_bHasGirderlineCoordinate(false),
m_Xgl(-1),
m_SpanIdx(INVALID_INDEX),
m_Xspan(-1),
m_bHasSpanPoint(false),
m_DeckCastingRegionIdx(INVALID_INDEX),
m_bHasDeckCastingRegion(false),
m_Attributes(0)
{
   m_RefAttributes.fill(0);

   IndexType index = GetIndex(GetReference(attrib));
   if ( index == INVALID_INDEX )
   {
      m_Attributes = attrib;
   }
   else
   {
      m_RefAttributes[index] = attrib;
   }

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

pgsPointOfInterest::pgsPointOfInterest(const pgsPointOfInterest& rOther) :
m_ID(INVALID_ID),
m_Xpoi(-1),
m_bHasSegmentPathCoordinate(false),
m_Xsp(-1),
m_bHasGirderCoordinate(false),
m_Xg(-1),
m_bHasGirderPathCoordinate(false),
m_Xgp(-1),
m_bHasGirderlineCoordinate(false),
m_Xgl(-1),
m_SpanIdx(INVALID_INDEX),
m_Xspan(-1),
m_bHasSpanPoint(false),
m_DeckCastingRegionIdx(INVALID_INDEX),
m_bHasDeckCastingRegion(false),
m_Attributes(0)
{
   m_RefAttributes.fill(0);

   MakeCopy(rOther);
}

pgsPointOfInterest::~pgsPointOfInterest()
{
}

pgsPointOfInterest& pgsPointOfInterest::operator= (const pgsPointOfInterest& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   ASSERTVALID;
   return *this;
}

bool pgsPointOfInterest::operator<=(const pgsPointOfInterest& rOther) const
{
   return *this < rOther || *this == rOther;
}

bool pgsPointOfInterest::operator<(const pgsPointOfInterest& rOther) const
{
   // comparing ID alone isn't enough. two POI with the same ID can
   // have different attributes. the attributes factor into this inequality
   //if (m_ID == rOther.m_ID && m_ID != INVALID_ID)
   //{
   //   // these are exactly the same POI
   //   return false;
   //}

   if (AtSamePlace(rOther))
   {
      // POIs are at the same place. Check the attributes to see if one is before the other

      if (HasAttribute(POI_SECTCHANGE_LEFTFACE) && rOther.HasAttribute(POI_SECTCHANGE_RIGHTFACE))
      {
         // At abrupt section changes there will be 2 poi (if the factory modeled them correctly)
         // this typically occurs at the terminus of end blocks
         // The poi with POI_SECTCHANGE_LEFTFACE comes before the poi with POI_SECTCHANGE_RIGHTFACE
         return true;
      }
      else if (HasAttribute(POI_SECTCHANGE_RIGHTFACE) && rOther.HasAttribute(POI_SECTCHANGE_LEFTFACE))
      {
         return false;
      }

      if (HasAttribute(POI_CASTING_BOUNDARY_END) && rOther.HasAttribute(POI_CASTING_BOUNDARY_START))
      {
         // At abrupt changes in deck casting sections there will be 2 poi
         // The poi with POI_CASTING_BOUNDARY_END comes before the poi with POI_CASTING_BOUNDARY_START
         // e.g. the poi on the left is the one where the region to the left is ending and the poi on 
         // the right is the one where the next region is starting
         return true;
      }
      else if (HasAttribute(POI_CASTING_BOUNDARY_START) && rOther.HasAttribute(POI_CASTING_BOUNDARY_END))
      {
         return false;
      }

      // Check span ends
      Uint16 thisSpanTenth = IsTenthPoint(POI_SPAN);
      Uint16 otherSpanTenth = rOther.IsTenthPoint(POI_SPAN);
      bool thisSpanCantilever = HasAttribute(POI_SPAN | POI_CANTILEVER);
      bool otherSpanCantilever = rOther.HasAttribute(POI_SPAN | POI_CANTILEVER);

      if ( (thisSpanTenth == 11 && otherSpanTenth == 1) || // this poi is at the end of a span and the other is at the start of the span. This poi is less (Span i, 1.0L < Span i+1, 0.0L)
           (thisSpanCantilever  && otherSpanTenth == 1) || // this poi is on the cantilever side and the other poi is at 0.0L
           (thisSpanTenth == 11 && otherSpanCantilever) ) // this poi is at 1.0L and the other is on the cantilever side
      {
         return true;
      }
      else if ( (thisSpanTenth == 1 && otherSpanTenth == 11) || // this poi is at the start of the span and the other is at the end of the span. This poi is more (Span i, 1.0L < Span i+1, 0.0L)
                (thisSpanTenth == 1 && otherSpanCantilever) || // this poi is at 0.0L and the other poi is on the cantilever side
                (thisSpanCantilever && otherSpanTenth == 11)) // this poi is on the cantilever side and the other poi is at 1.0L
      {
         return false;
      }
      else
      {
         // In erected model
         Uint16 thisErectedTenth = IsTenthPoint(POI_ERECTED_SEGMENT);
         Uint16 otherErectedTenth = rOther.IsTenthPoint(POI_ERECTED_SEGMENT);
         bool thisErectedCantilever = HasAttribute(POI_ERECTED_SEGMENT | POI_CANTILEVER);
         bool otherErectedCantilever = rOther.HasAttribute(POI_ERECTED_SEGMENT | POI_CANTILEVER);

         if ( (thisErectedCantilever  && otherErectedTenth == 1) || // this poi is on the cantilever side and the other poi is at 0.0L
              (thisErectedTenth == 11 && otherErectedCantilever) ) // this poi is at 1.0L and the other poi is on the cantilever side
         {
            return true;
         }
         else if ((thisErectedTenth == 1 && otherErectedCantilever) || // this poi is at 0.0L and the other poi is on the cantilever side
                  (thisErectedCantilever && otherErectedTenth == 11) ) // this poi is on the cantilever side and the other poi is at 1.0L
         {
            return false;
         }
         else
         {
            // In storage model
            Uint16 thisStorageTenth = IsTenthPoint(POI_STORAGE_SEGMENT);
            Uint16 otherStorageTenth = rOther.IsTenthPoint(POI_STORAGE_SEGMENT);
            bool thisStorageCantilever = HasAttribute(POI_STORAGE_SEGMENT | POI_CANTILEVER);
            bool otherStorageCantilever = rOther.HasAttribute(POI_STORAGE_SEGMENT | POI_CANTILEVER);

            if ( (thisStorageCantilever   && otherStorageTenth == 1) ||
                 (otherStorageTenth == 11 && otherStorageCantilever) )
            {
               return true;
            }
            else if ((otherStorageTenth == 1 && thisStorageCantilever) || // this poi is at 0.0L and the other poi is on the cantilever side
                     (thisStorageCantilever  && otherStorageTenth == 11)) // this poi is on the cantilever side and the other poi is at 1.0L
            {
               return false;
            }
         }
      }

      // all other things being equal, compare the ID
      return m_ID < rOther.m_ID;
   }
   else
   {
      // POIs are not at the same location
      // compare segment keys and location within the segment if equal
      if (m_SegmentKey < rOther.m_SegmentKey)
      {
         // this segment is before the other
         return true;
      }
      else if (rOther.m_SegmentKey < m_SegmentKey)
      {
         // this segment is after the other
         return false;
      }
      
      // segments are the same, check locations

      if (m_Xpoi < rOther.m_Xpoi)
      {
         return true;
      }
      else if (rOther.m_Xpoi < m_Xpoi)
      {
         return false;
      }

      ATLASSERT(false); // POIs are at the same location... should never get here
   }

   return false;
}

bool pgsPointOfInterest::operator==(const pgsPointOfInterest& rOther) const
{
   if (this == &rOther)
   {
      // POIs are equal if they are the same object
      return true;
   }

   // POIs are equal if they are not less than each other
   // return !(*this < rOther) && !(rOther < *this);
   return !std::less<>()(*this, rOther) && !std::less<>()(rOther, *this);
}

bool pgsPointOfInterest::operator!=(const pgsPointOfInterest& rOther) const
{
   return !(*this == rOther);
}

void pgsPointOfInterest::SetLocation(const CSegmentKey& segmentKey,Float64 Xpoi,Float64 Xsp,Float64 Xg,Float64 Xgp)
{
   m_SegmentKey = segmentKey;
   m_Xpoi = Xpoi;

   m_bHasSegmentPathCoordinate = true;
   m_Xsp = Xsp;

   m_bHasGirderCoordinate = true;
   m_Xg = Xg;

   m_bHasGirderPathCoordinate = true;
   m_Xgp = Xgp;

   m_bHasGirderlineCoordinate = false;
   m_Xgl = -1;

   m_bHasSpanPoint = false;
   m_SpanIdx = INVALID_INDEX;
   m_Xspan = -1;

   m_bHasDeckCastingRegion = false;
   m_DeckCastingRegionIdx = INVALID_INDEX;
}

void pgsPointOfInterest::SetLocation(const CSegmentKey& segmentKey,Float64 Xpoi)
{
   m_SegmentKey = segmentKey;
   m_Xpoi = Xpoi;

   m_bHasSegmentPathCoordinate = false;
   m_Xsp = -1;

   m_bHasGirderCoordinate = false;
   m_Xg = -1;

   m_bHasGirderPathCoordinate = false;
   m_Xgp = -1;

   m_bHasGirderlineCoordinate = false;
   m_Xgl = -1;

   m_bHasSpanPoint = false;
   m_SpanIdx = INVALID_INDEX;
   m_Xspan = -1;

   m_bHasDeckCastingRegion = false;
   m_DeckCastingRegionIdx = INVALID_INDEX;
}

void pgsPointOfInterest::Offset(Float64 delta)
{
   m_Xpoi += delta;

   if ( m_bHasSegmentPathCoordinate )
   {
      m_Xsp += delta;
   }

   if ( m_bHasGirderCoordinate )
   {
      m_Xg += delta;
   }

   if ( m_bHasGirderPathCoordinate )
   {
      m_Xgp += delta;
   }

   if (m_bHasGirderlineCoordinate)
   {
      m_Xgl += delta;
   }

   m_bHasSpanPoint = false;
   m_SpanIdx = INVALID_INDEX;
   m_Xspan = -1;

   m_bHasDeckCastingRegion = false;
   m_DeckCastingRegionIdx = INVALID_INDEX;
}

void pgsPointOfInterest::SetDistFromStart(Float64 Xpoi,bool bRetainAttributes)
{
   // once the POI is moved, the other points are no longer valid
   m_Xpoi = Xpoi;
      
   m_bHasSegmentPathCoordinate = false;
   m_Xsp = -1;
      
   m_bHasGirderCoordinate = false;
   m_Xg = -1;
      
   m_bHasGirderPathCoordinate = false;
   m_Xgp = -1;

   m_bHasGirderlineCoordinate = false;
   m_Xgl = -1;

   m_bHasSpanPoint = false;
   m_SpanIdx = INVALID_INDEX;
   m_Xspan = -1;

   m_bHasDeckCastingRegion = false;
   m_DeckCastingRegionIdx = INVALID_INDEX;

   if (!bRetainAttributes)
   {
      m_RefAttributes.fill(0);
      m_Attributes = 0;

#if defined _DEBUG || defined _BETA_VERSION
      m_strAttributes = _T("");
#endif // _DEBUG
   }
}

void pgsPointOfInterest::SetSegmentPathCoordinate(Float64 Xsp)
{
   m_bHasSegmentPathCoordinate = true;
   m_Xsp = Xsp;
}

Float64 pgsPointOfInterest::GetSegmentPathCoordinate() const
{
   ATLASSERT(m_bHasSegmentPathCoordinate); // if this fires, segment coordinate was never set
   return m_Xsp;
}

bool pgsPointOfInterest::HasSegmentPathCoordinate() const
{
   return m_bHasSegmentPathCoordinate;
}

void pgsPointOfInterest::SetGirderCoordinate(Float64 Xg)
{
   m_bHasGirderCoordinate = true;
   m_Xg = Xg;
}

Float64 pgsPointOfInterest::GetGirderCoordinate() const
{
   ATLASSERT(m_bHasGirderCoordinate); // if this fires, girder coordinate was never set
   return m_Xg;
}

bool pgsPointOfInterest::HasGirderCoordinate() const
{
   return m_bHasGirderCoordinate;
}

void pgsPointOfInterest::SetGirderPathCoordinate(Float64 Xgp)
{
   m_bHasGirderPathCoordinate = true;
   m_Xgp = Xgp;
}

Float64 pgsPointOfInterest::GetGirderPathCoordinate() const
{
   ATLASSERT(m_bHasGirderPathCoordinate); // if this fires, girder path coordinate was never set
   return m_Xgp;
}

bool pgsPointOfInterest::HasGirderPathCoordinate() const
{
   return m_bHasGirderPathCoordinate;
}


void pgsPointOfInterest::SetGirderlineCoordinate(Float64 Xgl)
{
   m_bHasGirderlineCoordinate = true;
   m_Xgl = Xgl;
}

Float64 pgsPointOfInterest::GetGirderlineCoordinate() const
{
   ATLASSERT(m_bHasGirderlineCoordinate); // if this fires, girder line coordinate was never set
   return m_Xgl;
}

bool pgsPointOfInterest::HasGirderlineCoordinate() const
{
   return m_bHasGirderlineCoordinate;
}

void pgsPointOfInterest::SetSpanPoint(SpanIndexType spanIdx,Float64 Xspan)
{
   m_SpanIdx = spanIdx;
   m_Xspan = Xspan;
   m_bHasSpanPoint = true;
}

void pgsPointOfInterest::GetSpanPoint(SpanIndexType* pSpanIdx,Float64* pXspan) const
{
   ATLASSERT(m_bHasSpanPoint);
   *pSpanIdx = m_SpanIdx;
   *pXspan = m_Xspan;
}

bool pgsPointOfInterest::HasSpanPoint() const
{
   return m_bHasSpanPoint;
}

void pgsPointOfInterest::SetDeckCastingRegion(IndexType deckCastingRegionIdx)
{
   m_DeckCastingRegionIdx = deckCastingRegionIdx;
   m_bHasDeckCastingRegion = true;
}

IndexType pgsPointOfInterest::GetDeckCastingRegion() const
{
   ATLASSERT(m_bHasDeckCastingRegion);
   return m_DeckCastingRegionIdx;
}

bool pgsPointOfInterest::HasDeckCastingRegion() const
{
   return m_bHasDeckCastingRegion;
}

void pgsPointOfInterest::ClearAttributes()
{
   m_Attributes = 0;
   m_RefAttributes.fill(0);
   UPDATE_ATTRIBUTES;
}

void pgsPointOfInterest::SetNonReferencedAttributes(PoiAttributeType attrib)
{
   ATLASSERT(GetIndex(GetReference(attrib)) == INVALID_INDEX);
   m_Attributes = attrib;

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

PoiAttributeType pgsPointOfInterest::GetNonReferencedAttributes() const
{
   return m_Attributes;
}

void pgsPointOfInterest::SetReferencedAttributes(PoiAttributeType attrib)
{
   PoiAttributeType refAttrib = GetReference(attrib);
   IndexType index = GetIndex(refAttrib);
   if ( attrib == 0 )
   {
      m_RefAttributes.fill(0);
   }
   else if ( refAttrib == attrib )
   {
      // attrib contains only one of the reference bits which means
      // we are clearing the attributes for that reference
      m_RefAttributes[index] = 0;
   }
   else
   {
      m_RefAttributes[index] = attrib;
   }

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

PoiAttributeType pgsPointOfInterest::GetReferencedAttributes(PoiAttributeType refAttribute) const
{
   IndexType index = GetIndex(GetReference(refAttribute));
   if ( index == INVALID_INDEX )
   {
      return 0;
   }
   else
   {
      return m_RefAttributes[index];
   }
}

void pgsPointOfInterest::RemoveAttributes(PoiAttributeType attrib)
{
   IndexType index = GetIndex(GetReference(attrib));
   if ( index == INVALID_INDEX )
   {
      WBFL::System::Flags<PoiAttributeType>::Clear(&m_Attributes,attrib);
   }
   else
   {
      WBFL::System::Flags<PoiAttributeType>::Clear(&m_RefAttributes[index],attrib);
   }

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

IndexType pgsPointOfInterest::GetIndex(PoiAttributeType refAttribute)
{
   for ( IndexType index = 0; index < gs_nRefAttributes; index++ )
   {
      if ( refAttribute == gs_RefAttributes[index] )
      {
         return index;
      }
   }

   return INVALID_INDEX;
}

PoiAttributeType pgsPointOfInterest::GetReference(PoiAttributeType attrib)
{
   PoiAttributeType poiRef = 0;
   for ( IndexType index = 0; index < gs_nRefAttributes; index++ )
   {
      if ( WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,gs_RefAttributes[index]) )
      {
         poiRef |= gs_RefAttributes[index];
      }
   }

   return poiRef;
}

bool pgsPointOfInterest::IsReferenceAttribute(PoiAttributeType attrib)
{
   return GetIndex(attrib) == INVALID_INDEX ? false : true;
}

bool pgsPointOfInterest::MergeAttributes(const pgsPointOfInterest& rOther)
{
   if (!AtSamePlace(rOther))
   {
      // POIs aren't at the same place, so they can't be merged
      return false;
   }

   bool bIs10thPoint = (0 < IsTenthPoint(gs_RefAttributes[0/*GetIndex(POI_RELEASED_SEGMENT)*/]) || // if this poi is a 10th point for any reference, it is a 10th point
                        0 < IsTenthPoint(gs_RefAttributes[1/*GetIndex(POI_LIFT_SEGMENT)*/])     ||
                        0 < IsTenthPoint(gs_RefAttributes[2/*GetIndex(POI_STORAGE_SEGMENT)*/])  ||
                        0 < IsTenthPoint(gs_RefAttributes[3/*GetIndex(POI_HAUL_SEGMENT)*/])     ||
                        0 < IsTenthPoint(gs_RefAttributes[4/*GetIndex(POI_ERECTED_SEGMENT)*/])  ||
                        0 < IsTenthPoint(gs_RefAttributes[5/*GetIndex(POI_SPAN)*/]));

   bool bIsOther10thPoint = (0 < rOther.IsTenthPoint(gs_RefAttributes[0/*GetIndex(POI_RELEASED_SEGMENT)*/]) || // if this poi is a 10th point for any reference, it is a 10th point
                             0 < rOther.IsTenthPoint(gs_RefAttributes[1/*GetIndex(POI_LIFT_SEGMENT)*/]) ||
                             0 < rOther.IsTenthPoint(gs_RefAttributes[2/*GetIndex(POI_STORAGE_SEGMENT)*/]) ||
                             0 < rOther.IsTenthPoint(gs_RefAttributes[3/*GetIndex(POI_HAUL_SEGMENT)*/]) ||
                             0 < rOther.IsTenthPoint(gs_RefAttributes[4/*GetIndex(POI_ERECTED_SEGMENT)*/]) ||
                             0 < rOther.IsTenthPoint(gs_RefAttributes[5/*GetIndex(POI_SPAN)*/]));

   // rOther cannot be merged into this POI if it has any of the following 
   // attributes because merging would cause rOther to move slightly
   if ( (rOther.HasAttribute(POI_SUPPORTS) ||
      rOther.HasAttribute(POI_START_FACE) ||
      rOther.HasAttribute(POI_END_FACE) ||
      rOther.HasAttribute(POI_STIRRUP_ZONE) ||
      rOther.HasAttribute(POI_H) ||
      rOther.HasAttribute(POI_15H) ||
      rOther.HasAttribute(POI_CLOSURE))
      &&
      (!bIs10thPoint && !bIsOther10thPoint) // merging can happen if both POI are 10th points
      )
   {
      return false;
   }

   if ((HasAttribute(POI_SECTCHANGE_LEFTFACE) && rOther.HasAttribute(POI_SECTCHANGE_RIGHTFACE)) || (HasAttribute(POI_SECTCHANGE_RIGHTFACE) && rOther.HasAttribute(POI_SECTCHANGE_LEFTFACE)))
   {
      // left and right faces cannot be merged together. They are on different sides of an abrupt change
      return false;
   }

   if ( (HasAttribute(POI_DUCT_START) && rOther.HasAttribute(POI_DUCT_END)) || (HasAttribute(POI_DUCT_END) && rOther.HasAttribute(POI_DUCT_START)))
   {
      // POIs for end points of ducts cannot be merged. The POIs are in different ducts
      return false;
   }

   if ((HasAttribute(POI_CASTING_BOUNDARY_START) && rOther.HasAttribute(POI_CASTING_BOUNDARY_END)) || (HasAttribute(POI_CASTING_BOUNDARY_END) && rOther.HasAttribute(POI_CASTING_BOUNDARY_START)))
   {
      // POIs for end points of deck casting regions cannot be merged. The POIs are in different casting regions
      return false;
   }

   if ((HasAttribute(POI_FACEOFSUPPORT) && rOther.HasAttribute(POI_ERECTED_SEGMENT | POI_CANTILEVER)) ||
      (HasAttribute(POI_ERECTED_SEGMENT | POI_CANTILEVER) && rOther.HasAttribute(POI_FACEOFSUPPORT))
      )
   {
      // cantilever points and face of support can't be merged (face of support is never on the cantilever)
      return false;
   }

   // If poi is for a concentrated load, diaphragm, or harp point, it does not merge into this POI unless
   // this poi is a 10th point, concentrated load, diaphragm or hapring point
   bool bHasAttrubutes = HasAttribute(POI_CONCLOAD | POI_DIAPHRAGM | POI_HARPINGPOINT);
   bool bOtherHasAttrubutes = rOther.HasAttribute(POI_CONCLOAD | POI_DIAPHRAGM | POI_HARPINGPOINT);
   if (bOtherHasAttrubutes && !(bIs10thPoint || bHasAttrubutes))
   {
      return false;
   }

   for (int i = 0; i < gs_nRefAttributes; i++)
   {
      if ((GetReferencedAttributes(gs_RefAttributes[i]) != 0 && rOther.GetReferencedAttributes(gs_RefAttributes[i]) != 0) && // both poi have the same referenced attribute -AND-
         ((HasAttribute(gs_RefAttributes[i] | POI_CANTILEVER) && !rOther.HasAttribute(gs_RefAttributes[i] | POI_CANTILEVER)) || // one has the cantilever attribute and the other does not
         (!HasAttribute(gs_RefAttributes[i] | POI_CANTILEVER) && rOther.HasAttribute(gs_RefAttributes[i] | POI_CANTILEVER))
            )
         )
      {
         // can't merge an "in-span" point with a cantilever poi... they are on opposite sides of a support
         return false;
      }
   }

   if (GetReferencedAttributes(POI_SPAN) != 0 && rOther.GetReferencedAttributes(POI_SPAN) != 0)
   {
      // if the two poi's that are being merged are both span POI, then we have a common
      // point in adjacent spans. these can't be merged.

      // we want (Span i-1, 1.0L) and (Span i, 0.0L) as seperate POIs
      // because a POI can't be in two spans
      return false;
   }

   // if the two POI's that are being merged are at a critical section and a face of support, they can
   // be merged, otherwise, they cannot
   if (
      (HasAttribute(POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2) && !rOther.HasAttribute(POI_FACEOFSUPPORT)) ||
      (rOther.HasAttribute(POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2) && !HasAttribute(POI_FACEOFSUPPORT))
      )
   { 
      return false;
   }

   // if we get this far, rOther can be merged into this POI
   m_Attributes |= rOther.m_Attributes;
   for ( int i = 0; i < gs_nRefAttributes; i++ )
   {
      m_RefAttributes[i] |= rOther.m_RefAttributes[i];
   }

   UPDATE_ATTRIBUTES;
   return true;
}

bool pgsPointOfInterest::HasAttribute(PoiAttributeType attribute) const
{
   PoiAttributeType targetReference = GetReference(attribute);
   IndexType index = GetIndex(targetReference);
   if ( index == INVALID_INDEX )
   {
      return WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes,attribute);
   }
   else
   {
      PoiAttributeType refAttrib = m_RefAttributes[index];
      if ( targetReference == attribute )
      {
         // attribute is just a reference attribute only (like POI_ERECTED_SEGMENT)
         // if the refAttrib isn't zero then it matches
         return (refAttrib == 0 ? false : true);
      }
      else
      {
         PoiAttributeType attrib    = attribute;
         WBFL::System::Flags<PoiAttributeType>::Clear(&refAttrib,targetReference);
         WBFL::System::Flags<PoiAttributeType>::Clear(&attrib,   targetReference);
         return WBFL::System::Flags<PoiAttributeType>::IsSet(refAttrib,attrib);
      }
   }
}

bool pgsPointOfInterest::HasAttributes() const
{
   if ( m_Attributes != 0 )
   {
      return true;
   }

   for ( int i = 0; i < gs_nRefAttributes; i++ )
   {
      if ( m_RefAttributes[i] != 0 )
      {
         return true;
      }
   }

   return false;
}

bool pgsPointOfInterest::AtSamePlace(const pgsPointOfInterest& other) const
{
   return (m_SegmentKey.IsEqual(other.m_SegmentKey) && IsEqual(m_Xpoi, other.m_Xpoi)) ? true : false;
}

bool pgsPointOfInterest::IsHarpingPoint() const
{
   return WBFL::System::Flags<PoiAttributeType>::IsSet( m_Attributes, POI_HARPINGPOINT );
}

bool pgsPointOfInterest::IsConcentratedLoad() const
{
   return WBFL::System::Flags<PoiAttributeType>::IsSet( m_Attributes, POI_CONCLOAD );
}

bool pgsPointOfInterest::IsMidSpan(PoiAttributeType reference) const
{
   IndexType index = GetIndex(GetReference(reference));
   if ( index == INVALID_INDEX )
   {
      return false;
   }
   else
   {
      return WBFL::System::Flags<PoiAttributeType>::IsSet( m_RefAttributes[index], POI_5L );
   }
}

bool pgsPointOfInterest::IsAtH() const
{
   return WBFL::System::Flags<PoiAttributeType>::IsSet( m_Attributes, POI_H );
}

bool pgsPointOfInterest::IsAt15H() const
{
   return WBFL::System::Flags<PoiAttributeType>::IsSet( m_Attributes, POI_15H );
}

Uint16 pgsPointOfInterest::IsTenthPoint(PoiAttributeType refAttribute) const
{
   IndexType index = GetIndex(refAttribute);

   if ( index == INVALID_INDEX )
   {
      return 0; // special code for not a tenth point
   }
   else
   {
      return GetAttributeTenthPoint(m_RefAttributes[index]);
   }
}

void pgsPointOfInterest::MakeTenthPoint(PoiAttributeType refAttribute,Uint16 tenthPoint)
{
   IndexType index = GetIndex(refAttribute);
   ATLASSERT(index != INVALID_INDEX);
   ATLASSERT(tenthPoint <= 11);
   SetAttributeTenthPoint(tenthPoint,&m_RefAttributes[index]);
   WBFL::System::Flags<PoiAttributeType>::Set(&m_RefAttributes[index],refAttribute);

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

void pgsPointOfInterest::MakeCopy(const pgsPointOfInterest& rOther)
{
   m_ID                        = rOther.m_ID;
   m_SegmentKey                = rOther.m_SegmentKey;
   m_Xpoi                      = rOther.m_Xpoi;
   m_bHasSegmentPathCoordinate = rOther.m_bHasSegmentPathCoordinate;
   m_Xsp                       = rOther.m_Xsp;
   m_bHasGirderCoordinate      = rOther.m_bHasGirderCoordinate;
   m_Xg                        = rOther.m_Xg;
   m_bHasGirderPathCoordinate  = rOther.m_bHasGirderPathCoordinate;
   m_Xgp                       = rOther.m_Xgp;
   m_bHasGirderlineCoordinate  = rOther.m_bHasGirderlineCoordinate;
   m_Xgl                       = rOther.m_Xgl;
   m_bHasSpanPoint             = rOther.m_bHasSpanPoint;
   m_SpanIdx                   = rOther.m_SpanIdx;
   m_Xspan                     = rOther.m_Xspan;
   m_bHasDeckCastingRegion     = rOther.m_bHasDeckCastingRegion;
   m_DeckCastingRegionIdx      = rOther.m_DeckCastingRegionIdx;

   m_Attributes = rOther.m_Attributes;
   for ( int i = 0; i < gs_nRefAttributes; i++ )
   {
      m_RefAttributes[i] = rOther.m_RefAttributes[i];
   }

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

void pgsPointOfInterest::MakeAssignment(const pgsPointOfInterest& rOther)
{
   MakeCopy( rOther );
}

void pgsPointOfInterest::SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* pAttribute)
{
   ATLASSERT(tenthPoint <= 11);
   PoiAttributeType tenthPointAttribute = 0;
   switch( tenthPoint )
   {
   case 1:  tenthPointAttribute = POI_0L; break;
   case 2:  tenthPointAttribute = POI_1L; break;
   case 3:  tenthPointAttribute = POI_2L; break;
   case 4:  tenthPointAttribute = POI_3L; break;
   case 5:  tenthPointAttribute = POI_4L; break;
   case 6:  tenthPointAttribute = POI_5L; break;
   case 7:  tenthPointAttribute = POI_6L; break;
   case 8:  tenthPointAttribute = POI_7L; break;
   case 9:  tenthPointAttribute = POI_8L; break;
   case 10: tenthPointAttribute = POI_9L; break;
   case 11: tenthPointAttribute = POI_10L; break;
   default:
      ATLASSERT(false);
   }
   *pAttribute |= tenthPointAttribute;
}

Uint16 pgsPointOfInterest::GetAttributeTenthPoint(PoiAttributeType attribute)
{
   if ( WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_0L) )
   {
      return 1;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_1L) )
   {
      return 2;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_2L) )
   {
      return 3;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_3L) )
   {
      return 4;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_4L) )
   {
      return 5;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_5L) )
   {
      return 6;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_6L) )
   {
      return 7;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_7L) )
   {
      return 8;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_8L) )
   {
      return 9;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_9L) )
   {
      return 10;
   }
   else if (WBFL::System::Flags<PoiAttributeType>::IsSet(attribute,POI_10L) )
   {
      return 11;
   }

   return 0;
}

std::_tstring pgsPointOfInterest::GetAttributes(PoiAttributeType reference,bool bIncludeMarkup) const
{
   std::_tstring strAttrib;
   if ( !HasAttributes() )
   {
      return strAttrib;
   }

   int nAttributes = 0;

   if ( IsHarpingPoint() )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("HP");
      nAttributes++;
   }

   if ( IsAtH() )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("H");
      nAttributes++;
   }

   if ( IsAt15H() )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("1.5H");
      nAttributes++;
   }
   
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      if ( HasAttribute(POI_CRITSECTSHEAR1) || HasAttribute(POI_CRITSECTSHEAR2) )
      {
         if ( 0 < nAttributes )
         {
            strAttrib += _T(", ");
         }

         strAttrib += _T("CS");
         nAttributes++;
      }
   }
   else
   {
      if ( HasAttribute(POI_CRITSECTSHEAR1) )
      {
         if ( 0 < nAttributes )
         {
            strAttrib += _T(", ");
         }

         strAttrib += _T("DCS");
         nAttributes++;
      }
      
      if ( HasAttribute(POI_CRITSECTSHEAR2) )
      {
         if ( 0 < nAttributes )
         {
            strAttrib += _T(", ");
         }

         strAttrib += _T("PCS");
         nAttributes++;
      }
   }

   if ( HasAttribute(POI_DIAPHRAGM) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("Diaphragm");
      nAttributes++;
   }

   if ( HasAttribute(POI_PSXFER) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("PSXFR");
      nAttributes++;
   }

   if ( HasAttribute(POI_PSDEV) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("Ld");
      nAttributes++;
   }

   if ( HasAttribute(POI_DEBOND) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("Debond");
      nAttributes++;
   }

   if ( HasAttribute(POI_DECKBARCUTOFF) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("Deck Bar Cutoff");
      nAttributes++;
   }

   if ( HasAttribute(POI_BARCUTOFF) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("Bar Cutoff");
      nAttributes++;
   }

   if ( HasAttribute(POI_BARDEVELOP) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("Bar Develop.");
      nAttributes++;
   }

   if ( HasAttribute(POI_FACEOFSUPPORT) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("FoS");
      nAttributes++;
   }

   if ( HasAttribute(POI_CLOSURE) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("CJ");
      nAttributes++;
   }

   if ( HasAttribute(POI_SECTCHANGE_TRANSITION) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("ST");
      nAttributes++;
   }

   if ( HasAttribute(POI_SECTCHANGE_RIGHTFACE) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("STRF");
      nAttributes++;
   }

   if ( HasAttribute(POI_SECTCHANGE_LEFTFACE) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("STLF");
      nAttributes++;
   }

   if (HasAttribute(POI_DUCT_START))
   {
      if (0 < nAttributes)
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("DS");
      nAttributes++;
   }

   if (HasAttribute(POI_DUCT_END))
   {
      if (0 < nAttributes)
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("DE");
      nAttributes++;
   }

   if (HasAttribute(POI_CASTING_BOUNDARY_START))
   {
      if (0 < nAttributes)
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("SDCR");
      nAttributes++;
   }

   if (HasAttribute(POI_CASTING_BOUNDARY_END))
   {
      if (0 < nAttributes)
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("EDCR");
      nAttributes++;
   }

   if ( HasAttribute(POI_INTERMEDIATE_PIER) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("IP");
      nAttributes++;
   }

   if ( HasAttribute(POI_STIRRUP_ZONE) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("SZB");
      nAttributes++;
   }

   if ( HasAttribute(POI_INTERMEDIATE_TEMPSUPPORT) )
   {
      if ( 0 < nAttributes )
      {
         strAttrib += _T(", ");
      }

      strAttrib += _T("ITS");
      nAttributes++;
   }

   std::vector<PoiAttributeType> references;
   if ( reference == 0 )
   {
      references.push_back(POI_RELEASED_SEGMENT);
      references.push_back(POI_LIFT_SEGMENT);
      references.push_back(POI_STORAGE_SEGMENT);
      references.push_back(POI_HAUL_SEGMENT);
      references.push_back(POI_ERECTED_SEGMENT);
      references.push_back(POI_SPAN);
   }
   else
   {
      references.push_back(reference);
   }

   // for the sake of efficiency, dont use a stringstream
   std::vector<PoiAttributeType>::iterator iter(references.begin());
   std::vector<PoiAttributeType>::iterator end(references.end());
   for ( ; iter != end; iter++ )
   {
      PoiAttributeType refAttribute = *iter;

      if ( HasAttribute(refAttribute | POI_PICKPOINT) )
      {
         if ( 0 < nAttributes )
         {
            strAttrib += _T(", ");
         }

         strAttrib += _T("Pick Point");
         nAttributes++;
      }

      if ( HasAttribute(refAttribute | POI_BUNKPOINT) )
      {
         if ( 0 < nAttributes )
         {
            strAttrib += _T(", ");
         }

         strAttrib += _T("Bunk Point");
         nAttributes++;
      }

      IndexType index = GetIndex(refAttribute);

      Uint16 tenpt = IsTenthPoint(refAttribute);
      if (0 < tenpt)
      {
         ATLASSERT(tenpt < 12);

         if ( 0 < nAttributes )
         {
            strAttrib += _T(", ");
         }

         if ( bIncludeMarkup )
         {
            strAttrib += gs_str10thPointLabelWithMarkup[index][tenpt];
         }
         else
         {
            strAttrib += gs_str10thPointLabel[index][tenpt];
         }

         nAttributes++;
      }
   }

   return strAttrib;
}

#if defined _DEBUG || defined _BETA_VERSION
bool pgsPointOfInterest::AssertValid() const
{
   return true;
}

void pgsPointOfInterest::UpdateAttributeString()
{
   m_strAttributes.clear();
   std::_tostringstream os;

   for (IndexType i = 0; i < gs_nRefAttributes; i++)
   {
      if (m_RefAttributes[i] != 0)
      {
         PoiAttributeType refAttribute = gs_RefAttributes[i];
         std::_tstring strReference = gs_strRefAttributes[i];

         if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_RefAttributes[i], POI_PICKPOINT))
         {
            os << _T("(POI_PICKPOINT | ") << strReference << _T(") | ");
         }

         if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_RefAttributes[i], POI_BUNKPOINT))
         {
            os << _T("(POI_BUNKPOINT | ") << strReference << _T(") | ");
         }

         if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_RefAttributes[i], POI_CANTILEVER))
         {
            os << _T("(POI_CANTILEVER | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 1)
         {
            os << _T("(POI_0L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 2)
         {
            os << _T("(POI_1L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 3)
         {
            os << _T("(POI_2L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 4)
         {
            os << _T("(POI_3L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 5)
         {
            os << _T("(POI_4L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 6)
         {
            os << _T("(POI_5L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 7)
         {
            os << _T("(POI_6L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 8)
         {
            os << _T("(POI_7L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 9)
         {
            os << _T("(POI_8L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 10)
         {
            os << _T("(POI_9L | ") << strReference << _T(") | ");
         }

         if (IsTenthPoint(refAttribute) == 11)
         {
            os << _T("(POI_10L | ") << strReference << _T(") | ");
         }
      } // end of if
   } // next reference type

   if (IsAtH())
   {
      os << _T("(POI_H | ");
   }

   if (IsAt15H())
   {
      os << _T("(POI_15H | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_CRITSECTSHEAR1))
   {
      os << _T("POI_CRITSECTSHEAR1 | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_CRITSECTSHEAR2))
   {
      os << _T("POI_CRITSECTSHEAR2 | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_HARPINGPOINT))
   {
      os << _T("POI_HARPINGPOINT | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_CONCLOAD))
   {
      os << _T("POI_CONCLOAD | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_DIAPHRAGM))
   {
      os << _T("POI_DIAPHRAGM | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_PSXFER))
   {
      os << _T("POI_PSXFER | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_PSDEV))
   {
      os << _T("POI_PSDEV | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_DEBOND))
   {
      os << _T("POI_DEBOND | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_DECKBARCUTOFF))
   {
      os << _T("POI_DECKBARCUTOFF | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_BARCUTOFF))
   {
      os << _T("POI_BARCUTOFF | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_BARDEVELOP))
   {
      os << _T("POI_BARDEVELOP | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_FACEOFSUPPORT))
   {
      os << _T("POI_FACEOFSUPPORT | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_CLOSURE))
   {
      os << _T("POI_CLOSURE | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_SECTCHANGE_TRANSITION))
   {
      os << _T("POI_SECTCHANGE_TRANSITION | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_SECTCHANGE_RIGHTFACE))
   {
      os << _T("POI_SECTCHANGE_RIGHTFACE | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_SECTCHANGE_LEFTFACE))
   {
      os << _T("POI_SECTCHANGE_LEFTFACE | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_DUCT_START))
   {
      os << _T("POI_DUCT_START | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_DUCT_END))
   {
      os << _T("POI_DUCT_END | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_CASTING_BOUNDARY_START))
   {
      os << _T("POI_CASTING_BOUNDARY_START | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_CASTING_BOUNDARY_END))
   {
      os << _T("POI_CASTING_BOUNDARY_END | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_INTERMEDIATE_TEMPSUPPORT))
   {
      os << _T("POI_INTERMEDIATE_TEMPSUPPORT | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_INTERMEDIATE_PIER))
   {
      os << _T("POI_INTERMEDIATE_PIER | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_BOUNDARY_PIER))
   {
      os << _T("POI_BOUNDARY_PIER | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_ABUTMENT))
   {
      os << _T("POI_ABUTMENT | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_STIRRUP_ZONE))
   {
      os << _T("POI_STIRRUP_ZONE | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_START_FACE))
   {
      os << _T("POI_START_FACE | ");
   }

   if (WBFL::System::Flags<PoiAttributeType>::IsSet(m_Attributes, POI_END_FACE))
   {
      os << _T("POI_END_FACE | ");
   }

   os << std::endl;

   m_strAttributes = os.str();

   if (HasAttribute(POI_SECTCHANGE_LEFTFACE) && HasAttribute(POI_SECTCHANGE_RIGHTFACE))
   {
      ATLASSERT(false); // poi can't be both left and right face of a section change
   }
}

#endif // _DEBUG


void MakePoiList(const std::vector<pgsPointOfInterest>& vPoi,PoiList* pPoiList)
{
   pPoiList->clear();
   pPoiList->reserve(vPoi.size());
   std::transform(std::cbegin(vPoi), std::cend(vPoi), std::back_inserter(*pPoiList), [](const auto& poi) {return std::reference_wrapper<const pgsPointOfInterest>(poi); });
}

void MakePoiVector(const PoiList& vPoiList, std::vector<pgsPointOfInterest>* pvPoi)
{
   pvPoi->clear();
   pvPoi->reserve(vPoiList.size());
   std::transform(std::cbegin(vPoiList), std::cend(vPoiList), std::back_inserter(*pvPoi), [](const auto& poi) {return poi.get(); });
}
