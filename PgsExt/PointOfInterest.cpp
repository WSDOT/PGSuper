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

static PoiAttributeType gs_RefAttributes[] = { 
   POI_RELEASED_SEGMENT, 
   POI_LIFT_SEGMENT, 
   POI_STORAGE_SEGMENT,
   POI_HAUL_SEGMENT, 
   POI_ERECTED_SEGMENT, 
   POI_SPAN 
};

static IndexType gs_nRefAttributes = sizeof(gs_RefAttributes)/sizeof(gs_RefAttributes[0]);

#if defined _DEBUG

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
m_bCanMerge(true),
m_Attributes(0)
{
   memset(m_RefAttributes,0,sizeof(m_RefAttributes));
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
m_bCanMerge(true),
m_Attributes(0)
{
   memset(m_RefAttributes,0,sizeof(m_RefAttributes));

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
m_bCanMerge(true),
m_Attributes(0)
{
   memset(m_RefAttributes,0,sizeof(m_RefAttributes));

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
m_bCanMerge(true),
m_Attributes(0)
{
   memset(m_RefAttributes,0,sizeof(m_RefAttributes));

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
   if ( AtSamePlace(rOther) && IsTenthPoint(POI_SPAN) == 11 && rOther.IsTenthPoint(POI_SPAN) == 1 )
   {
      // POIs are at exactly the same location and this poi is at the end of a span and 
      // the other poi is at the start of a span (1.0L/0.0L)
      // this poi is before (less than) the other poi
      return true;
   }

   if ( m_SegmentKey < rOther.m_SegmentKey )
   {
      return true;
   }

   if ( rOther.m_SegmentKey < m_SegmentKey )
   {
      return false;
   }

   if (IsZero(m_Xpoi-rOther.m_Xpoi))
   {
      return false;
   }

   if ( m_Xpoi < rOther.m_Xpoi )
   {
      return true;
   }

   if ( rOther.m_Xpoi < m_Xpoi )
   {
      return false;
   }

   return false;
}

bool pgsPointOfInterest::operator==(const pgsPointOfInterest& rOther) const
{
   if ( m_ID == rOther.m_ID && AtSamePlace(rOther) )
   {
      return true;
   }
   else
   {
      return false;
   }
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
}

void pgsPointOfInterest::SetDistFromStart(Float64 Xpoi)
{
   if ( !IsEqual(m_Xpoi,Xpoi) )
   {
      // once the POI is moved, the other points are no longer
      // valid
      m_Xpoi = Xpoi;
      
      m_bHasSegmentPathCoordinate = false;
      m_Xsp = -1;
      
      m_bHasGirderCoordinate = false;
      m_Xg = -1;
      
      m_bHasGirderPathCoordinate = false;
      m_Xgp = -1;
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
      for ( IndexType i = 0; i < gs_nRefAttributes; i++ )
      {
         m_RefAttributes[i] = 0;
      }
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
      sysFlags<PoiAttributeType>::Clear(&m_Attributes,attrib);
   }
   else
   {
      sysFlags<PoiAttributeType>::Clear(&m_RefAttributes[index],attrib);
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
      if ( sysFlags<PoiAttributeType>::IsSet(attrib,gs_RefAttributes[index]) )
      {
         poiRef |= gs_RefAttributes[index];
      }
   }

   return poiRef;
}

bool pgsPointOfInterest::CanMerge() const
{
   return m_bCanMerge;
}

void pgsPointOfInterest::CanMerge(bool bCanMerge)
{
   m_bCanMerge = bCanMerge;
}

bool pgsPointOfInterest::IsReferenceAttribute(PoiAttributeType attrib)
{
   return GetIndex(attrib) == INVALID_INDEX ? false : true;
}

bool pgsPointOfInterest::MergeAttributes(const pgsPointOfInterest& rOther)
{
   m_Attributes |= rOther.m_Attributes;
   for ( int i = 0; i < 6; i++ )
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
      return sysFlags<PoiAttributeType>::IsSet(m_Attributes,attribute);
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
         sysFlags<PoiAttributeType>::Clear(&refAttrib,targetReference);
         sysFlags<PoiAttributeType>::Clear(&attrib,   targetReference);
         return sysFlags<PoiAttributeType>::IsSet(refAttrib,attrib);
      }
   }
}

bool pgsPointOfInterest::HasAttributes() const
{
   if ( m_Attributes != 0 )
   {
      return true;
   }

   for ( int i = 0; i < 6; i++ )
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
   if ( m_SegmentKey.IsEqual(other.m_SegmentKey) &&
        IsZero( m_Xpoi - other.m_Xpoi, ms_Tol ) )
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
   IndexType index = GetIndex(GetReference(reference));
   if ( index == INVALID_INDEX )
   {
      return false;
   }
   else
   {
      return sysFlags<PoiAttributeType>::IsSet( m_RefAttributes[index], POI_5L );
   }
}

bool pgsPointOfInterest::IsAtH() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_H );
}

bool pgsPointOfInterest::IsAt15H() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_15H );
}

Uint16 pgsPointOfInterest::IsTenthPoint(PoiAttributeType refAttribute) const
{
   IndexType index = GetIndex(refAttribute);
   ATLASSERT(index != INVALID_INDEX);

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
   sysFlags<PoiAttributeType>::Set(&m_RefAttributes[index],refAttribute);

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
   m_bCanMerge                 = rOther.m_bCanMerge;
   m_Attributes                = rOther.m_Attributes;

   for ( int i = 0; i < 6; i++ )
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
   if ( sysFlags<PoiAttributeType>::IsSet(attribute,POI_0L) )
   {
      return 1;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_1L) )
   {
      return 2;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_2L) )
   {
      return 3;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_3L) )
   {
      return 4;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_4L) )
   {
      return 5;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_5L) )
   {
      return 6;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_6L) )
   {
      return 7;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_7L) )
   {
      return 8;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_8L) )
   {
      return 9;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_9L) )
   {
      return 10;
   }
   else if (sysFlags<PoiAttributeType>::IsSet(attribute,POI_10L) )
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
   
   if ( lrfdVersionMgr::ThirdEdition2004 <= lrfdVersionMgr::GetVersion() )
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

      strAttrib += _T("Bar Cutoff");
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
   LPCTSTR release_label_markup[]={_T("err"),_T("0.0L<sub>s</sub>"),_T("0.1L<sub>s</sub>"),_T("0.2L<sub>s</sub>"),_T("0.3L<sub>s</sub>"),_T("0.4L<sub>s</sub>"),
      _T("0.5L<sub>s</sub>"),_T("0.6L<sub>s</sub>"),_T("0.7L<sub>s</sub>"),_T("0.8L<sub>s</sub>"),_T("0.9L<sub>s</sub>"),_T("1.0L<sub>s</sub>")};

   LPCTSTR release_label[]={_T("err"),_T("0.0Ls"),_T("0.1Ls"),_T("0.2Ls"),_T("0.3Ls"),_T("0.4Ls"),
      _T("0.5Ls"),_T("0.6Ls"),_T("0.7Ls"),_T("0.8Ls"),_T("0.9Ls"),_T("1.0Ls")};

   LPCTSTR girder_label_markup[]={_T("err"),_T("0.0L<sub>g</sub>"),_T("0.1L<sub>g</sub>"),_T("0.2L<sub>g</sub>"),_T("0.3L<sub>g</sub>"),_T("0.4L<sub>g</sub>"),
      _T("0.5L<sub>g</sub>"),_T("0.6L<sub>g</sub>"),_T("0.7L<sub>g</sub>"),_T("0.8L<sub>g</sub>"),_T("0.9L<sub>g</sub>"),_T("1.0L<sub>g</sub>")};

   LPCTSTR girder_label[]={_T("err"),_T("0.0Lg"),_T("0.1Lg"),_T("0.2Lg"),_T("0.3Lg"),_T("0.4Lg"),
      _T("0.5Lg"),_T("0.6Lg"),_T("0.7Lg"),_T("0.8Lg"),_T("0.9Lg"),_T("1.0Lg")};

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

#if defined _DEBUG
bool pgsPointOfInterest::AssertValid() const
{
   return true;
}

void pgsPointOfInterest::UpdateAttributeString()
{
   m_strAttributes.clear();
   std::_tostringstream os;

   for ( IndexType i = 0; i < gs_nRefAttributes; i++ )
   {
      if ( m_RefAttributes[i] != 0 )
      {
         PoiAttributeType refAttribute = gs_RefAttributes[i];
         std::_tstring strReference = gs_strRefAttributes[i];

         if ( sysFlags<PoiAttributeType>::IsSet(m_RefAttributes[i],POI_PICKPOINT) )
         {
            os << _T("(POI_PICKPOINT | ");
         }

         if ( sysFlags<PoiAttributeType>::IsSet(m_RefAttributes[i],POI_BUNKPOINT) )
         {
            os << _T("(POI_BUNKPOINT | ");
         }

         if ( IsTenthPoint(refAttribute) == 1 )
         {
            os << _T("(POI_0L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 2 )
         {
            os << _T("(POI_1L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 3 )
         {
            os << _T("(POI_2L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 4 )
         {
            os << _T("(POI_3L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 5 )
         {
            os << _T("(POI_4L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 6 )
         {
            os << _T("(POI_5L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 7 )
         {
            os << _T("(POI_6L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 8 )
         {
            os << _T("(POI_7L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 9 )
         {
            os << _T("(POI_8L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 10 )
         {
            os << _T("(POI_9L | ") << strReference << _T(") | ");
         }

         if ( IsTenthPoint(refAttribute) == 11 )
         {
            os << _T("(POI_10L | ") << strReference << _T(") | ");
         }
      } // end of if
   } // next reference type

   if ( IsAtH() )
   {
      os << _T("(POI_H | ");
   }

   if ( IsAt15H() )
   {
      os << _T("(POI_15H | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CRITSECTSHEAR1) )
   {
      os << _T("POI_CRITSECTSHEAR1 | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CRITSECTSHEAR2) )
   {
      os << _T("POI_CRITSECTSHEAR2 | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_HARPINGPOINT) )
   {
      os << _T("POI_HARPINGPOINT | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CONCLOAD) )
   {
      os << _T("POI_CONCLOAD | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_PSXFER) )
   {
      os << _T("POI_PSXFER | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_PSDEV) )
   {
      os << _T("POI_PSDEV | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_DEBOND) )
   {
      os << _T("POI_DEBOND | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_DECKBARCUTOFF) )
   {
      os << _T("POI_DECKBARCUTOFF | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_BARCUTOFF) )
   {
      os << _T("POI_BARCUTOFF | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_BARDEVELOP) )
   {
      os << _T("POI_BARDEVELOP | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_FACEOFSUPPORT) )
   {
      os << _T("POI_FACEOFSUPPORT | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_CLOSURE) )
   {
      os << _T("POI_CLOSURE | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_SECTCHANGE_TRANSITION) )
   {
      os << _T("POI_SECTCHANGE_TRANSITION | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_SECTCHANGE_RIGHTFACE) )
   {
      os << _T("POI_SECTCHANGE_RIGHTFACE | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_SECTCHANGE_LEFTFACE) )
   {
      os << _T("POI_SECTCHANGE_LEFTFACE | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_INTERMEDIATE_TEMPSUPPORT) )
   {
      os << _T("POI_INTERMEDIATE_TEMPSUPPORT | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_INTERMEDIATE_PIER) )
   {
      os << _T("POI_INTERMEDIATE_PIER | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_BOUNDARY_PIER) )
   {
      os << _T("POI_BOUNDARY_PIER | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_ABUTMENT) )
   {
      os << _T("POI_ABUTMENT | ");
   }

   if ( sysFlags<PoiAttributeType>::IsSet(m_Attributes,POI_STIRRUP_ZONE) )
   {
      os << _T("POI_STIRRUP_ZONE | ");
   }

   os << std::endl;

   m_strAttributes = os.str();
}

#endif // _DEBUG

