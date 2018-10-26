///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Float64 pgsPointOfInterest::ms_Tol = 1.0e-06;

/****************************************************************************
CLASS
   pgsPointOfInterest
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPointOfInterest::pgsPointOfInterest()
{
   m_ID = -1;
   m_Span = 0;
   m_Girder = 0;
   m_DistFromStart = 0;
   m_Attributes = POI_ALLACTIONS | POI_ALLOUTPUT;
}

pgsPointOfInterest::pgsPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib) :
m_ID(-1),
m_Span(span),
m_Girder(gdr),
m_DistFromStart(distFromStart),
m_Attributes(attrib)
{
   ATLASSERT( !(distFromStart < 0) );  // must be zero or more.

   ASSERTVALID;
}

pgsPointOfInterest::pgsPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib) :
m_ID(-1),
m_Span(span),
m_Girder(gdr),
m_DistFromStart(distFromStart),
m_Attributes(attrib)
{
   ATLASSERT( !(distFromStart < 0) );  // must be zero or more.

   m_Stages.insert(stage);
   ASSERTVALID;
}

pgsPointOfInterest::pgsPointOfInterest(std::set<pgsTypes::Stage> stages,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib) :
m_ID(-1),
m_Span(span),
m_Girder(gdr),
m_DistFromStart(distFromStart),
m_Attributes(attrib),
m_Stages(stages)
{
   ATLASSERT( !(distFromStart < 0) );  // must be zero or more.

   ASSERTVALID;
}

pgsPointOfInterest::pgsPointOfInterest(const pgsPointOfInterest& rOther)
{
   MakeCopy(rOther);
   ASSERTVALID;
}

pgsPointOfInterest::~pgsPointOfInterest()
{
}

//======================== OPERATORS  =======================================
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
   if ( GetSpan() < rOther.GetSpan() )
      return true;
   
   if ( GetSpan() > rOther.GetSpan() )
      return false;

   if ( GetGirder() < rOther.GetGirder() )
      return true;

   if ( GetGirder() > rOther.GetGirder() )
      return false;

   if ( GetDistFromStart() < rOther.GetDistFromStart() && !IsEqual(GetDistFromStart(), rOther.GetDistFromStart()) )
      return true;

   if ( GetDistFromStart() > rOther.GetDistFromStart() && !IsEqual(GetDistFromStart(), rOther.GetDistFromStart()) )
      return false;

   //// if everything else is equal, compare ID's
   //if ( IsEqual( GetDistFromStart(), rOther.GetDistFromStart() ) )
   //   return GetID() < rOther.GetID();

   return false;
}

bool pgsPointOfInterest::operator==(const pgsPointOfInterest& rOther) const
{
   if ( GetID() == rOther.GetID() && GetSpan() == rOther.GetSpan() && GetGirder() == rOther.GetGirder() && IsEqual(GetDistFromStart(),rOther.GetDistFromStart()) )
      return true;
   else
      return false;
}

//======================== OPERATIONS =======================================
void pgsPointOfInterest::SetLocation(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart)
{
   SetSpan(span);
   SetGirder(gdr);
   SetDistFromStart(distFromStart);
}

//======================== ACCESS     =======================================
PoiIDType pgsPointOfInterest::GetID() const
{
   return m_ID;
}

void pgsPointOfInterest::SetSpan(SpanIndexType span)
{
   m_Span = span;
}

SpanIndexType pgsPointOfInterest::GetSpan() const
{
   return m_Span;
}

void pgsPointOfInterest::SetGirder(GirderIndexType gdr)
{
   m_Girder = gdr;
}

GirderIndexType pgsPointOfInterest::GetGirder() const
{
   return m_Girder;
}

void pgsPointOfInterest::SetDistFromStart(Float64 distFromStart)
{
   ATLASSERT( !(distFromStart < 0) );
   m_DistFromStart = distFromStart;
}

Float64 pgsPointOfInterest::GetDistFromStart() const
{
   return m_DistFromStart;
}

void pgsPointOfInterest::SetAttributes(PoiAttributeType attrib)
{
   m_Attributes = attrib;
   ASSERTVALID;
}

PoiAttributeType pgsPointOfInterest::GetAttributes() const
{
   return m_Attributes;
}

bool pgsPointOfInterest::HasAttribute(PoiAttributeType attribute) const
{
   return sysFlags<PoiAttributeType>::IsSet(m_Attributes,attribute);
}

void pgsPointOfInterest::AddStage(pgsTypes::Stage stage)
{
   m_Stages.insert(stage);
}

void pgsPointOfInterest::AddStages(std::set<pgsTypes::Stage> stages)
{
   m_Stages.insert(stages.begin(),stages.end());
}

void pgsPointOfInterest::RemoveStage(pgsTypes::Stage stage)
{
   std::set<pgsTypes::Stage>::iterator found = m_Stages.find(stage);
   if ( found != m_Stages.end() )
   {
      m_Stages.erase(found);
   }
}

bool pgsPointOfInterest::HasStage(pgsTypes::Stage stage) const
{
   std::set<pgsTypes::Stage>::const_iterator found = m_Stages.find(stage);
   return ( found != m_Stages.end() ? true : false );
}

std::set<pgsTypes::Stage> pgsPointOfInterest::GetStages() const
{
   return m_Stages;
}

Uint32 pgsPointOfInterest::GetStageCount() const
{
   return m_Stages.size();
}

void pgsPointOfInterest::SetTolerance(Float64 tol)
{
   ms_Tol = tol;
}

Float64 pgsPointOfInterest::GetTolerance()
{
   return ms_Tol;
}

//======================== INQUIRY    =======================================

bool pgsPointOfInterest::IsFlexureStress() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_FLEXURESTRESS );
}

bool pgsPointOfInterest::IsFlexureCapacity() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_FLEXURECAPACITY );
}

bool pgsPointOfInterest::IsShear() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_SHEAR );
}

bool pgsPointOfInterest::IsDisplacement() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_DISPLACEMENT );
}

bool pgsPointOfInterest::IsHarpingPoint() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_HARPINGPOINT );
}

bool pgsPointOfInterest::IsConcentratedLoad() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_CONCLOAD );
}

bool pgsPointOfInterest::IsMidSpan() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_MIDSPAN );
}

bool pgsPointOfInterest::IsTabular() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_TABULAR );
}

bool pgsPointOfInterest::IsGraphical() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_GRAPHICAL );
}

bool pgsPointOfInterest::IsAtH() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_H );
}

bool pgsPointOfInterest::IsAt15H() const
{
   return sysFlags<PoiAttributeType>::IsSet( m_Attributes, POI_15H );
}

Uint16 pgsPointOfInterest::IsATenthPoint(PoiAttributeType basisType) const
{
   Uint16 tenthPoint = GetAttributeTenthPoint(m_Attributes);
   if ( HasAttribute(basisType) )
      return tenthPoint;
   else
      return 0;
}

void pgsPointOfInterest::MakeTenthPoint(PoiAttributeType basisType,Uint16 tenthPoint)
{
   ATLASSERT(basisType == POI_SPAN_POINT || basisType == POI_GIRDER_POINT);
   ATLASSERT(tenthPoint <= 11);
   m_Attributes |= basisType;
   SetAttributeTenthPoint(tenthPoint,&m_Attributes);
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsPointOfInterest::MakeCopy(const pgsPointOfInterest& rOther)
{
   // Add copy code here...
   m_ID            = rOther.m_ID;
   m_Span          = rOther.m_Span;
   m_Girder        = rOther.m_Girder;
   m_DistFromStart = rOther.m_DistFromStart;
   m_Attributes    = rOther.m_Attributes;
   m_Stages        = rOther.m_Stages;

   ASSERTVALID;
}

void pgsPointOfInterest::MakeAssignment(const pgsPointOfInterest& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================
void pgsPointOfInterest::SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* pattribute)
{
   ATLASSERT(tenthPoint <= 11);
   *pattribute |= PoiAttributeType(tenthPoint);
}

Uint16 pgsPointOfInterest::GetAttributeTenthPoint(PoiAttributeType attribute)
{
   Uint32 low32 = low_Uint32(attribute);
   Uint16 tenth_point = low_Uint16(low32);
   ATLASSERT(tenth_point <= 11);
   return tenth_point;
}

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsPointOfInterest::AssertValid() const
{
   if ( m_DistFromStart < 0 )
      return false;

   return true;
}

void pgsPointOfInterest::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsPointOfInterest" << endl;
   os << "m_ID            = " << m_ID << endl;
   os << "m_Span          = " << m_Span << endl;
   os << "m_Girder        = " << m_Girder << endl;
   os << "m_DistFromStart = " << m_DistFromStart << endl;
   //os << "m_Attributes    = " << m_Attributes << endl;


   if ( IsFlexureCapacity() )
      os << "   FlexureCapacity" << endl;

   if ( IsFlexureStress() )
      os << "   FlexureStress" << endl;

   if ( IsShear() )
      os << "   Shear" << endl;

   if ( IsDisplacement() )
      os << "   Displacement" << endl;

   if ( IsHarpingPoint() )
      os << "   HarpingPoint" << endl;

   if ( IsConcentratedLoad() )
      os << "   ConcentrateLoad" << endl;

   if ( IsMidSpan() )
      os << "   MidSpan" << endl;

   if ( IsAtH() )
      os << "   At H" << endl;

   if ( IsAt15H() )
      os << "   At 1.5H" << endl;

   if ( IsGraphical() )
      os << "   Graphical" << endl;

   if ( IsTabular() )
      os << "   Tabular" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsPointOfInterest::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsPointOfInterest");

   pgsPointOfInterest poi(1,1,10.0);
   TRY_TESTME( poi.GetSpan() == 1 );
   TRY_TESTME( poi.GetGirder() == 1 );
   TRY_TESTME( IsEqual(poi.GetDistFromStart(),10.) );
   TRY_TESTME( poi.IsFlexureCapacity() );
   TRY_TESTME( poi.IsFlexureStress() );
   TRY_TESTME( poi.IsShear() );
   TRY_TESTME( poi.IsDisplacement() );
   TRY_TESTME( !poi.IsHarpingPoint() );
   TRY_TESTME( !poi.IsConcentratedLoad() );
   TRY_TESTME( poi.IsTabular() );
   TRY_TESTME( poi.IsGraphical() );

   TESTME_EPILOG("PointOfInterest");
}
#endif // _UNITTEST


rptPointOfInterest::rptPointOfInterest(const pgsPointOfInterest& poi,
                                       Float64 endOffset,
                                       const unitLength* pUnitOfMeasure,
                                       Float64 zeroTolerance,
                                       bool bShowUnitTag,bool bSpanPOI) :
rptLengthUnitValue( poi.GetDistFromStart()-endOffset , pUnitOfMeasure, zeroTolerance, bShowUnitTag ),
m_Poi(poi),m_bSpanPOI(bSpanPOI),m_bPrefixAttributes(true)
{
}

rptPointOfInterest::rptPointOfInterest(const unitLength* pUnitOfMeasure,
                                       Float64 zeroTolerance,
                                       bool bShowUnitTag,bool bSpanPOI) :
rptLengthUnitValue(pUnitOfMeasure,zeroTolerance,bShowUnitTag),m_bSpanPOI(bSpanPOI),m_bPrefixAttributes(true)
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

void rptPointOfInterest::MakeGirderPoi(bool bGirderPOI)
{
   m_bSpanPOI = !bGirderPOI;
}

void rptPointOfInterest::MakeSpanPoi(bool bSpanPOI)
{
   m_bSpanPOI = bSpanPOI;
}

bool rptPointOfInterest::IsSpanPoi() const
{
   return m_bSpanPOI;
}

bool rptPointOfInterest::IsGirderPoi() const
{
   return !m_bSpanPOI;
}

rptReportContent& rptPointOfInterest::SetValue(const pgsPointOfInterest& poi,Float64 endOffset)
{
   m_Poi = poi;
   return rptLengthUnitValue::SetValue( poi.GetDistFromStart() - endOffset );
}

std::string rptPointOfInterest::AsString() const
{
   std::string strAttrib;
   Uint16 nAttributes = 0;
   PoiAttributeType attributes = m_Poi.GetAttributes();

   strAttrib = "(";

   if ( m_Poi.IsHarpingPoint() )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "HP";
      nAttributes++;
   }

   if ( m_Poi.IsAtH() && ((m_bSpanPOI == m_Poi.HasAttribute(POI_SPAN_POINT)) || (!m_bSpanPOI == m_Poi.HasAttribute(POI_GIRDER_POINT))) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "H";
      nAttributes++;
   }

   if ( m_Poi.IsAt15H() )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "1.5H";
      nAttributes++;
   }
   
   if ( m_Poi.HasAttribute(POI_CRITSECTSHEAR1) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "DCS";
      nAttributes++;
   }
   
   if ( m_Poi.HasAttribute(POI_CRITSECTSHEAR2) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "PCS";
      nAttributes++;
   }

   if ( m_Poi.HasAttribute(POI_PSXFER) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "PSXFR";
      nAttributes++;
   }

   if ( m_Poi.HasAttribute(POI_PSDEV) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "Ld";
      nAttributes++;
   }

   if ( m_Poi.HasAttribute(POI_DEBOND) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "Debond";
      nAttributes++;
   }

   if ( m_Poi.HasAttribute(POI_BARCUTOFF) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "Bar Cutoff";
      nAttributes++;
   }

   if ( m_Poi.HasAttribute(POI_PICKPOINT) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "Pick Point";
      nAttributes++;
   }

   if ( m_Poi.HasAttribute(POI_BUNKPOINT) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "Bunk Point";
      nAttributes++;
   }

   if ( m_Poi.HasAttribute(POI_FACEOFSUPPORT) )
   {
      if ( 0 < nAttributes )
         strAttrib += ", ";

      strAttrib += "FoS";
      nAttributes++;
   }

   //if ( m_Poi.HasAttribute(POI_SECTCHANGE) )
   //{
   //   if ( 0 < nAttributes )
   //      strAttrib += ", ";

   //   strAttrib += "XS";
   //   nAttributes++;
   //}

   Uint16 tenpt = m_Poi.IsATenthPoint(m_bSpanPOI ? POI_SPAN_POINT : POI_GIRDER_POINT);
   if (0 < tenpt)
   {
      CHECK(tenpt<12);
      // for the sake of efficiency, dont use a stringstream
      const char* span_label[]={"err","0.0L<sub>s</sub>","0.1L<sub>s</sub>","0.2L<sub>s</sub>","0.3L<sub>s</sub>","0.4L<sub>s</sub>",
         "0.5L<sub>s</sub>","0.6L<sub>s</sub>","0.7L<sub>s</sub>","0.8L<sub>s</sub>","0.9L<sub>s</sub>","1.0L<sub>s</sub>"};

      const char* girder_label[]={"err","0.0L<sub>g</sub>","0.1L<sub>g</sub>","0.2L<sub>g</sub>","0.3L<sub>g</sub>","0.4L<sub>g</sub>",
         "0.5L<sub>g</sub>","0.6L<sub>g</sub>","0.7L<sub>g</sub>","0.8L<sub>g</sub>","0.9L<sub>g</sub>","1.0L<sub>g</sub>"};


      if ( 0 < nAttributes )
         strAttrib += ", ";

      if ( m_bSpanPOI )
         strAttrib += std::string(span_label[tenpt]);
      else
         strAttrib += std::string(girder_label[tenpt]);

      nAttributes++;
   }
   strAttrib += ")";

   std::string strValue = rptLengthUnitValue::AsString();

   std::string str;
   if ( nAttributes == 0 )
   {
      str = strValue;
   }
   else
   {
      if ( m_bPrefixAttributes )
         str = strAttrib + " " + strValue;
      else
         str = strValue + " " + strAttrib;
   }

   return str;
}

void rptPointOfInterest::MakeCopy(const rptPointOfInterest& rOther)
{
   m_Poi = rOther.m_Poi;
   m_bSpanPOI = rOther.m_bSpanPOI;
   m_bPrefixAttributes = rOther.m_bPrefixAttributes;
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
