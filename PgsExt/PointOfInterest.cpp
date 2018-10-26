///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined _DEBUG
#define UPDATE_ATTRIBUTES UpdateAttributeString()
#else
#define UPDATE_ATTRIBUTES
#endif

Float64 pgsPointOfInterest::ms_Tol = 1.0e-06;

/****************************************************************************
CLASS
   pgsPointOfInterest
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPointOfInterest::pgsPointOfInterest():
m_ID(INVALID_ID),
m_DistFromStart(-1),
m_bHasSegmentCoordinate(false),
m_Xs(-1),
m_bHasGirderCoordinate(false),
m_Xg(-1),
m_bHasGirderPathCoordinate(false),
m_Xgp(-1),
m_bCanMerge(true),
m_Attributes(0)
{
   UPDATE_ATTRIBUTES;
}

pgsPointOfInterest::pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart,PoiAttributeType attrib) :
m_ID(INVALID_ID),
m_SegmentKey(segmentKey),
m_DistFromStart(distFromStart),
m_bHasSegmentCoordinate(false),
m_Xs(-1),
m_bHasGirderCoordinate(false),
m_Xg(-1),
m_bHasGirderPathCoordinate(false),
m_Xgp(-1),
m_bCanMerge(true),
m_Attributes(attrib)
{
   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

pgsPointOfInterest::pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStart,Float64 Xs,Float64 Xg,Float64 Xgp,PoiAttributeType attrib) :
m_ID(INVALID_ID),
m_SegmentKey(segmentKey),
m_DistFromStart(distFromStart),
m_bHasSegmentCoordinate(true),
m_Xs(Xs),
m_bHasGirderCoordinate(true),
m_Xg(Xg),
m_bHasGirderPathCoordinate(true),
m_Xgp(Xgp),
m_bCanMerge(true),
m_Attributes(attrib)
{
   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

pgsPointOfInterest::pgsPointOfInterest(const pgsPointOfInterest& rOther)
{
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

bool pgsPointOfInterest::operator<(const pgsPointOfInterest& rOther) const
{
   if ( m_SegmentKey < rOther.m_SegmentKey )
      return true;

   if ( rOther.m_SegmentKey < m_SegmentKey )
      return false;

   if (IsZero(m_DistFromStart-rOther.m_DistFromStart))
      return false;

   if ( m_DistFromStart < rOther.m_DistFromStart )
      return true;

   if ( rOther.m_DistFromStart < m_DistFromStart )
      return false;

   return false;
}

bool pgsPointOfInterest::operator==(const pgsPointOfInterest& rOther) const
{
   if ( m_ID         == rOther.m_ID         && 
        m_SegmentKey == rOther.m_SegmentKey        &&
        IsEqual(m_DistFromStart,rOther.m_DistFromStart) )
   {
      return true;
   }
   else
   {
      return false;
   }
}

void pgsPointOfInterest::SetLocation(const CSegmentKey& segmentKey,Float64 distFromStart,Float64 Xs,Float64 Xg,Float64 Xgp)
{
   m_SegmentKey = segmentKey;
   m_DistFromStart = distFromStart;
   m_bHasSegmentCoordinate = true;
   m_Xs = Xs;
   m_bHasGirderCoordinate = true;
   m_Xg = Xg;
   m_bHasGirderPathCoordinate = true;
   m_Xgp = Xgp;
}

void pgsPointOfInterest::Offset(Float64 delta)
{
   m_DistFromStart += delta;

   if ( m_bHasSegmentCoordinate )
      m_Xs += delta;

   if ( m_bHasGirderCoordinate )
      m_Xg += delta;

   if ( m_bHasGirderPathCoordinate )
      m_Xgp += delta;
}

void pgsPointOfInterest::SetDistFromStart(Float64 distFromStart)
{
   if ( !IsEqual(m_DistFromStart,distFromStart) )
   {
      // once the POI is moved, the other points are no longer
      // valid
      m_DistFromStart = distFromStart;
      m_bHasSegmentCoordinate = false;
      m_Xs = -1;
      m_bHasGirderCoordinate = false;
      m_Xg = -1;
      m_bHasGirderPathCoordinate = false;
      m_Xgp = -1;
   }
}

void pgsPointOfInterest::SetSegmentCoordinate(Float64 Xs)
{
   ATLASSERT(0 <= Xs);
   m_bHasSegmentCoordinate = true;
   m_Xs = Xs;
}

Float64 pgsPointOfInterest::GetSegmentCoordinate() const
{
   ATLASSERT(m_bHasSegmentCoordinate); // if this fires, segment coordinate was never set
   return m_Xs;
}

bool pgsPointOfInterest::HasSegmentCoordinate() const
{
   return m_bHasSegmentCoordinate;
}

void pgsPointOfInterest::SetGirderCoordinate(Float64 Xg)
{
   ATLASSERT(0 <= Xg);
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
   ATLASSERT(0 <= Xgp);
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

void pgsPointOfInterest::SetAttributes(PoiAttributeType attrib)
{
   m_Attributes = attrib;

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

PoiAttributeType pgsPointOfInterest::GetAttributes() const
{
   return m_Attributes;
}

void pgsPointOfInterest::RemoveAttributes(PoiAttributeType attrib)
{
   sysFlags<PoiAttributeType>::Clear(&m_Attributes,attrib);

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

PoiAttributeType pgsPointOfInterest::GetReference() const
{
   return GetReference(m_Attributes);
}

PoiAttributeType pgsPointOfInterest::GetReference(PoiAttributeType attrib)
{
   PoiAttributeType poiRef = 0;
   if ( sysFlags<PoiAttributeType>::IsSet(attrib,POI_RELEASED_SEGMENT) )
      poiRef |= POI_RELEASED_SEGMENT;
   
   if ( sysFlags<PoiAttributeType>::IsSet(attrib,POI_LIFT_SEGMENT) )
      poiRef |= POI_LIFT_SEGMENT;
   
   if ( sysFlags<PoiAttributeType>::IsSet(attrib,POI_STORAGE_SEGMENT) )
      poiRef |= POI_STORAGE_SEGMENT;
   
   if ( sysFlags<PoiAttributeType>::IsSet(attrib,POI_HAUL_SEGMENT) )
      poiRef |= POI_HAUL_SEGMENT;
   
   if ( sysFlags<PoiAttributeType>::IsSet(attrib,POI_ERECTED_SEGMENT) )
      poiRef |= POI_ERECTED_SEGMENT;
   
   if ( sysFlags<PoiAttributeType>::IsSet(attrib,POI_GIRDER) )
      poiRef |= POI_GIRDER;

   return poiRef;
}

void pgsPointOfInterest::MergeAttributes(const pgsPointOfInterest& rOther)
{
   m_Attributes |= rOther.GetAttributes();
   UPDATE_ATTRIBUTES;
}

bool pgsPointOfInterest::HasAttribute(PoiAttributeType attribute) const
{
   return sysFlags<PoiAttributeType>::IsSet(m_Attributes,attribute);
}

bool pgsPointOfInterest::AtSamePlace(const pgsPointOfInterest& other) const
{
   if ( m_SegmentKey == other.m_SegmentKey &&
        IsZero( m_DistFromStart - other.m_DistFromStart, ms_Tol ) )
   {
      return true;
   }

   return false;
}

void pgsPointOfInterest::SetTolerance(Float64 tol)
{
   ms_Tol = tol;
}

Float64 pgsPointOfInterest::GetTolerance()
{
   return ms_Tol;
}

bool pgsPointOfInterest::IsHarpingPoint() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_HARPINGPOINT );
}

bool pgsPointOfInterest::IsConcentratedLoad() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_CONCLOAD );
}

bool pgsPointOfInterest::IsMidSpan(PoiAttributeType reference) const
{
   ATLASSERT(IsValidReference(reference));
   if ( !sysFlags<PoiAttributeType>::IsSet(m_Attributes,reference) )
      return false;

   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_MIDSPAN );
}

bool pgsPointOfInterest::IsAtH(PoiAttributeType reference) const
{
   ATLASSERT(IsValidReference(reference));
   if ( !sysFlags<PoiAttributeType>::IsSet(m_Attributes,reference) )
      return false;

   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_H );
}

bool pgsPointOfInterest::IsAt15H(PoiAttributeType reference) const
{
   ATLASSERT(IsValidReference(reference));
   if ( !sysFlags<PoiAttributeType>::IsSet(m_Attributes,reference) )
      return false;

   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_15H );
}

Uint16 pgsPointOfInterest::IsTenthPoint(PoiAttributeType reference) const
{
   ATLASSERT(IsValidReference(reference));
   if ( !sysFlags<PoiAttributeType>::IsSet(m_Attributes,reference) )
      return 0;
   
   return GetAttributeTenthPoint(m_Attributes);
}

void pgsPointOfInterest::MakeTenthPoint(PoiAttributeType reference,Uint16 tenthPoint)
{
   ATLASSERT(tenthPoint <= 11);
   SetAttributeTenthPoint(tenthPoint,&m_Attributes);
   sysFlags<PoiAttributeType>::Set(&m_Attributes,reference);
}

void pgsPointOfInterest::MakeCopy(const pgsPointOfInterest& rOther)
{
   m_ID                       = rOther.m_ID;
   m_SegmentKey               = rOther.m_SegmentKey;
   m_DistFromStart            = rOther.m_DistFromStart;
   m_bHasSegmentCoordinate    = rOther.m_bHasSegmentCoordinate;
   m_Xs                       = rOther.m_Xs;
   m_bHasGirderCoordinate     = rOther.m_bHasGirderCoordinate;
   m_Xg                       = rOther.m_Xg;
   m_bHasGirderPathCoordinate = rOther.m_bHasGirderPathCoordinate;
   m_Xgp                      = rOther.m_Xgp;
   m_Attributes               = rOther.m_Attributes;

   UPDATE_ATTRIBUTES;
   ASSERTVALID;
}

void pgsPointOfInterest::MakeAssignment(const pgsPointOfInterest& rOther)
{
   MakeCopy( rOther );
}

void pgsPointOfInterest::SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* pattribute)
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
   *pattribute |= PoiAttributeType(tenthPointAttribute);
}

Uint16 pgsPointOfInterest::GetAttributeTenthPoint(PoiAttributeType attribute)
{
   if ( sysFlags<PoiAttributeType>::IsSet(attribute,POI_0L) )
      return 1;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_1L) )
      return 2;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_2L) )
      return 3;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_3L) )
      return 4;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_4L) )
      return 5;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_5L) )
      return 6;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_6L) )
      return 7;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_7L) )
      return 8;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_8L) )
      return 9;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_9L) )
      return 10;
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_10L) )
      return 11;

   return 0;
}

bool pgsPointOfInterest::IsValidReference(PoiAttributeType reference) const
{
   if (sysFlags<PoiAttributeType>::IsSet(reference,POI_RELEASED_SEGMENT) || 
       sysFlags<PoiAttributeType>::IsSet(reference,POI_LIFT_SEGMENT)     || 
       sysFlags<PoiAttributeType>::IsSet(reference,POI_STORAGE_SEGMENT)  || 
       sysFlags<PoiAttributeType>::IsSet(reference,POI_HAUL_SEGMENT)     || 
       sysFlags<PoiAttributeType>::IsSet(reference,POI_ERECTED_SEGMENT)  || 
       sysFlags<PoiAttributeType>::IsSet(reference,POI_GIRDER)           ||
       reference == 0
      )
   {
      return true;
   }

   return false;
}

#if defined _DEBUG
bool pgsPointOfInterest::AssertValid() const
{
   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_H) ||
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_15H) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_MIDSPAN) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_0L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_1L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_2L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_3L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_4L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_5L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_6L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_7L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_8L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_9L) || 
        sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_10L) 
      )
   {
      // must have a valid frame of reference for all of the above attributes
      if ( GetReference() == 0 )
      {
         ATLASSERT(false); // You probably wanted to know that the attributes aren't correct
         return false;
      }
   }

   return true;
}

void pgsPointOfInterest::UpdateAttributeString()
{
   m_strAttributes.clear();
   std::_tostringstream os;

   // figure out POI reference frame
   PoiAttributeType poiReference = GetReference();

   std::_tstring strReference(_T(""));
   
   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_RELEASED_SEGMENT) )
      strReference += _T("POI_RELEASED_SEGMENT | ");
   
   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_LIFT_SEGMENT) )
      strReference += _T("POI_LIFT_SEGMENT | ");
   
   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_STORAGE_SEGMENT) )
      strReference += _T("POI_STORAGE_SEGMENT | ");
   
   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_HAUL_SEGMENT) )
      strReference += _T("POI_HAUL_SEGMENT | ");
   
   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_ERECTED_SEGMENT) )
      strReference += _T("POI_ERECTED_SEGMENT | ");
   
   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_GIRDER) )
      strReference += _T("POI_GIRDER | ");

   if ( poiReference != 0 )
   {
      if ( IsAtH(poiReference) )
         os << _T("(POI_H | ") << strReference << _T(") | ");

      if ( IsAt15H(poiReference) )
         os << _T("(POI_15H | ") << strReference << _T(") | ");

      if ( IsMidSpan(poiReference) )
         os << _T("POI_MIDSPAN | ");

      if ( IsTenthPoint(poiReference) == 1 )
         os << _T("(POI_0L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 2 )
         os << _T("(POI_1L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 3 )
         os << _T("(POI_2L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 4 )
         os << _T("(POI_3L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 5 )
         os << _T("(POI_4L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 6 )
         os << _T("(POI_5L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 7 )
         os << _T("(POI_6L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 8 )
         os << _T("(POI_7L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 9 )
         os << _T("(POI_8L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 10 )
         os << _T("(POI_9L | ") << strReference << _T(") | ");

      if ( IsTenthPoint(poiReference) == 11 )
         os << _T("(POI_10L | ") << strReference << _T(") | ");
   }


   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_PICKPOINT) )
      os << _T("(POI_PICKPOINT | ") << strReference << _T(") | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_BUNKPOINT) )
      os << _T("(POI_BUNKPOINT | ") << strReference << _T(") | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CRITSECTSHEAR1) )
      os << _T("POI_CRITSECTSHEAR1 | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CRITSECTSHEAR2) )
      os << _T("POI_CRITSECTSHEAR2 | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_HARPINGPOINT) )
      os << _T("POI_HARPINGPOINT | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CONCLOAD) )
      os << _T("POI_CONCLOAD | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_PSXFER) )
      os << _T("POI_PSXFER | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_PSDEV) )
      os << _T("POI_PSDEV | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_DEBOND) )
      os << _T("POI_DEBOND | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_DECKBARCUTOFF) )
      os << _T("POI_DECKBARCUTOFF | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_BARCUTOFF) )
      os << _T("POI_BARCUTOFF | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_BARDEVELOP) )
      os << _T("POI_BARDEVELOP | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_FACEOFSUPPORT) )
      os << _T("POI_FACEOFSUPPORT | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CLOSURE) )
      os << _T("POI_CLOSURE | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_SECTCHANGE_TRANSITION) )
      os << _T("POI_SECTCHANGE_TRANSITION | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_SECTCHANGE_RIGHTFACE) )
      os << _T("POI_SECTCHANGE_RIGHTFACE | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_SECTCHANGE_LEFTFACE) )
      os << _T("POI_SECTCHANGE_LEFTFACE | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_TEMPSUPPORT) )
      os << _T("POI_TEMPSUPPORT | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_INTERMEDIATE_PIER) )
      os << _T("POI_INTERMEDIATE_PIER | ");

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_PIER) )
      os << _T("POI_PIER | ");

   os << std::endl;

   m_strAttributes = os.str();
}

#endif // _DEBUG

