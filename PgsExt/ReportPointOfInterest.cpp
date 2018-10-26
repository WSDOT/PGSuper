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
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

rptPointOfInterest::rptPointOfInterest(const unitLength* pUnitOfMeasure,
                                       Float64 zeroTolerance,
                                       bool bShowUnitTag) :
rptLengthUnitValue(pUnitOfMeasure,zeroTolerance,bShowUnitTag),m_bPrefixAttributes(true),m_bIncludeSpanAndGirder(false)
{
}

rptPointOfInterest::rptPointOfInterest(const rptPointOfInterest& rOther) :
rptLengthUnitValue( rOther )
{
   MakeCopy( rOther );
}

rptPointOfInterest& rptPointOfInterest::operator = (const rptPointOfInterest& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment( rOther );
   }

   return *this;
}

rptReportContent* rptPointOfInterest::CreateClone() const
{
   return new rptPointOfInterest( *this );
}

void rptPointOfInterest::IncludeSpanAndGirder(bool bIncludeSpanAndGirder)
{
   m_bIncludeSpanAndGirder = bIncludeSpanAndGirder;
}

bool rptPointOfInterest::IncludeSpanAndGirder() const
{
   return m_bIncludeSpanAndGirder;
}

rptReportContent& rptPointOfInterest::SetValue(PoiAttributeType reference,const pgsPointOfInterest& poi,Float64 endOffset)
{
   ATLASSERT(sysFlags<PoiAttributeType>::IsSet(reference,POI_RELEASED_SEGMENT) || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_STORAGE_SEGMENT)  || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_LIFT_SEGMENT)     || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_HAUL_SEGMENT)     || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_ERECTED_SEGMENT)  || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_SPAN));

   m_POI = poi;
   m_Reference = reference;
   return rptLengthUnitValue::SetValue( poi.GetDistFromStart() - endOffset );
}

std::_tstring rptPointOfInterest::AsString() const
{
   std::_tstring strAttrib = m_POI.GetAttributes(m_Reference,true);
   std::_tstring strValue = rptLengthUnitValue::AsString();

   std::_tstring str;

   if ( m_bIncludeSpanAndGirder )
   {
      CString str1;

      GroupIndexType   grpIdx = m_POI.GetSegmentKey().groupIndex;
      GirderIndexType  gdrIdx = m_POI.GetSegmentKey().girderIndex;
      SegmentIndexType segIdx = m_POI.GetSegmentKey().segmentIndex;

      str1.Format(_T("Group %d Girder %s Segment %d, "),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));
      str = str1;
   }

   if ( strAttrib.size() == 0 )
   {
      str += strValue;
   }
   else
   {
      if ( m_bPrefixAttributes )
      {
         str += _T("(") + strAttrib + _T(") ") + strValue;
      }
      else
      {
         str += strValue + _T(" (") + strAttrib + _T(")");
      }
   }

   return str;
}

void rptPointOfInterest::MakeCopy(const rptPointOfInterest& rOther)
{
   m_POI                   = rOther.m_POI;
   m_Reference             = rOther.m_Reference;
   m_bPrefixAttributes     = rOther.m_bPrefixAttributes;
   m_bIncludeSpanAndGirder = rOther.m_bIncludeSpanAndGirder;
}

void rptPointOfInterest::MakeAssignment(const rptPointOfInterest& rOther)
{
   rptLengthUnitValue::MakeAssignment( rOther );
   MakeCopy( rOther );
}

void rptPointOfInterest::PrefixAttributes(bool bPrefixAttributes)
{
   m_bPrefixAttributes = bPrefixAttributes;
}

bool rptPointOfInterest::PrefixAttributes() const
{
   return m_bPrefixAttributes;
}
