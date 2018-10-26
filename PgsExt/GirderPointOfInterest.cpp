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
#include <PgsExt\GirderPointOfInterest.h>
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
      MakeAssignment( rOther );

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
             sysFlags<PoiAttributeType>::IsSet(reference,POI_ERECTED_SEGMENT)  || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_LIFT_SEGMENT)     || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_HAUL_SEGMENT)     || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_GIRDER));

   m_POI = poi;
   m_Reference = reference;
   return rptLengthUnitValue::SetValue( poi.GetDistFromStart() - endOffset );
}

std::_tstring rptPointOfInterest::AsString() const
{
   std::_tstring strAttrib;
   Uint16 nAttributes = 0;
   PoiAttributeType attributes = m_POI.GetAttributes();

   strAttrib = _T("(");

   if ( m_POI.IsHarpingPoint() )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("HP");
      nAttributes++;
   }

   if ( m_POI.IsAtH(m_Reference) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("H");
      nAttributes++;
   }

   if ( m_POI.IsAt15H(m_Reference) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("1.5H");
      nAttributes++;
   }
   
   if ( lrfdVersionMgr::ThirdEdition2004 <= lrfdVersionMgr::GetVersion() )
   {
      if ( m_POI.HasAttribute(POI_CRITSECTSHEAR1) || m_POI.HasAttribute(POI_CRITSECTSHEAR2) )
      {
         if ( 0 < nAttributes )
            strAttrib += _T(", ");

         strAttrib += _T("CS");
         nAttributes++;
      }
   }
   else
   {
      if ( m_POI.HasAttribute(POI_CRITSECTSHEAR1) )
      {
         if ( 0 < nAttributes )
            strAttrib += _T(", ");

         strAttrib += _T("DCS");
         nAttributes++;
      }
      
      if ( m_POI.HasAttribute(POI_CRITSECTSHEAR2) )
      {
         if ( 0 < nAttributes )
            strAttrib += _T(", ");

         strAttrib += _T("PCS");
         nAttributes++;
      }
   }

   if ( m_POI.HasAttribute(POI_PSXFER) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("PSXFR");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_PSDEV) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("Ld");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_DEBOND) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("Debond");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_DECKBARCUTOFF) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("Bar Cutoff");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_BARCUTOFF) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("Bar Cutoff");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_BARDEVELOP) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("Bar Develop.");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_PICKPOINT) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("Pick Point");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_BUNKPOINT) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("Bunk Point");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_FACEOFSUPPORT) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("FoS");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_CLOSURE) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("CP");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_SECTCHANGE_TRANSITION) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("ST");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_INTERMEDIATE_PIER) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("IP");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_STIRRUP_ZONE) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("SZB");
      nAttributes++;
   }

   if ( m_POI.HasAttribute(POI_TEMPSUPPORT) )
   {
      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      strAttrib += _T("ITS");
      nAttributes++;
   }

   Uint16 tenpt = m_POI.IsTenthPoint(m_Reference);
   if (0 < tenpt)
   {
      ATLASSERT(tenpt<12);
      // for the sake of efficiency, dont use a stringstream
      LPCTSTR release_label[]={_T("err"),_T("0.0L<sub>s</sub>"),_T("0.1L<sub>s</sub>"),_T("0.2L<sub>s</sub>"),_T("0.3L<sub>s</sub>"),_T("0.4L<sub>s</sub>"),
         _T("0.5L<sub>s</sub>"),_T("0.6L<sub>s</sub>"),_T("0.7L<sub>s</sub>"),_T("0.8L<sub>s</sub>"),_T("0.9L<sub>s</sub>"),_T("1.0L<sub>s</sub>")};

      LPCTSTR girder_label[]={_T("err"),_T("0.0L<sub>g</sub>"),_T("0.1L<sub>g</sub>"),_T("0.2L<sub>g</sub>"),_T("0.3L<sub>g</sub>"),_T("0.4L<sub>g</sub>"),
         _T("0.5L<sub>g</sub>"),_T("0.6L<sub>g</sub>"),_T("0.7L<sub>g</sub>"),_T("0.8L<sub>g</sub>"),_T("0.9L<sub>g</sub>"),_T("1.0L<sub>g</sub>")};

      if ( 0 < nAttributes )
         strAttrib += _T(", ");

      if ( sysFlags<PoiAttributeType>::IsSet(m_Reference,POI_ERECTED_SEGMENT) )
         strAttrib += std::_tstring(girder_label[tenpt]);
      else
         strAttrib += std::_tstring(release_label[tenpt]);

      nAttributes++;
   }
   strAttrib += _T(")");

   std::_tstring strValue = rptLengthUnitValue::AsString();

   std::_tstring str;

   if ( m_bIncludeSpanAndGirder )
   {
      CString str1;

      GroupIndexType   grpIdx = m_POI.GetSegmentKey().groupIndex;
      GirderIndexType  gdrIdx = m_POI.GetSegmentKey().girderIndex;
      SegmentIndexType segIdx = m_POI.GetSegmentKey().segmentIndex;

      str1.Format(_T("Group %d Girder %s Segment %d, "),LABEL_SPAN(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));
      str = str1;
   }

   if ( nAttributes == 0 )
   {
      str += strValue;
   }
   else
   {
      if ( m_bPrefixAttributes )
         str += strAttrib + _T(" ") + strValue;
      else
         str += strValue + _T(" ") + strAttrib;
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
