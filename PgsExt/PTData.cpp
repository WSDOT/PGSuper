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

/****************************************************************************
CLASS
   CPTData
****************************************************************************/

#include <PgsExt\PTData.h>
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <Lrfd\StrandPool.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\SplicedGirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Float64 gs_DefaultOffset2 = ::ConvertToSysUnits(6.0,unitMeasure::Inch);

const CDuctSize CDuctSize::DuctSizes[] = {CDuctSize(_T("4\""),    ::ConvertToSysUnits(4.0,unitMeasure::Inch),31,22,::ConvertToSysUnits(0.55,unitMeasure::Inch),::ConvertToSysUnits(0.70,unitMeasure::Inch)),
                                            CDuctSize(_T("4 1/2\""),::ConvertToSysUnits(4.5,unitMeasure::Inch),37,27,::ConvertToSysUnits(1.00,unitMeasure::Inch),::ConvertToSysUnits(1.00,unitMeasure::Inch))};
const Uint32 CDuctSize::nDuctSizes = sizeof(CDuctSize::DuctSizes)/sizeof(CDuctSize::DuctSizes[0]);

StrandIndexType CDuctSize::GetMaxStrands(matPsStrand::Size size) const
{
   StrandIndexType nMax;
   switch(size)
   {
   case matPsStrand::D1270: // 0.5"
      nMax = MaxStrands5;
      break;

   case matPsStrand::D1524: // 0.6"
      nMax = MaxStrands6;
      break;

   default:
      ATLASSERT(false);
   }
   return nMax;
}

Float64 CDuctSize::GetEccentricity(matPsStrand::Size size) const
{
   Float64 e;
   switch(size)
   {
   case matPsStrand::D1270: // 0.5"
      e = Ecc5;
      break;

   case matPsStrand::D1524: // 0.6"
      e = Ecc6;
      break;

   default:
      ATLASSERT(false);
   }
   return e;
}


CComVariant GetOffsetTypeProperty(CDuctGeometry::OffsetType offsetType)
{
   switch(offsetType)
   {
   case CDuctGeometry::TopGirder:
      return CComVariant(_T("TopGirder"));

   case CDuctGeometry::BottomGirder:
      return CComVariant(_T("BottomGirder"));
   }

   ATLASSERT(false);
   return CComVariant();
}

CDuctGeometry::OffsetType GetOffsetType(VARIANT& var)
{
   if ( CComBSTR(_T("TopGirder")) == CComBSTR(var.bstrVal) )
      return CDuctGeometry::TopGirder;
   else if ( CComBSTR(_T("BottomGirder")) == CComBSTR(var.bstrVal) )
      return CDuctGeometry::BottomGirder;

   ATLASSERT(false);
   return CDuctGeometry::BottomGirder;
}
////////////////////////////////////////////
CDuctGeometry::CDuctGeometry()
{
   m_pGirder = NULL;
}

CDuctGeometry::CDuctGeometry(const CDuctGeometry& rOther)
{
   MakeCopy(rOther);
}

CDuctGeometry::CDuctGeometry(const CSplicedGirderData* pGirder)
{
   m_pGirder = pGirder;
}

CDuctGeometry::~CDuctGeometry()
{
}

CDuctGeometry& CDuctGeometry::operator=(const CDuctGeometry& rOther)
{
   if ( &rOther != this )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CDuctGeometry::SetGirder(const CSplicedGirderData* pGirder)
{
   m_pGirder = pGirder;
}

const CSplicedGirderData* CDuctGeometry::GetGirder() const
{
   return m_pGirder;
}

void CDuctGeometry::MakeCopy(const CDuctGeometry& rOther)
{
   m_pGirder = rOther.m_pGirder;
}

void CDuctGeometry::MakeAssignment(const CDuctGeometry& rOther)
{
   MakeCopy(rOther);
}

////////////////////////////////////////////
CLinearDuctGeometry::CLinearDuctGeometry()
{
}

CLinearDuctGeometry::CLinearDuctGeometry(const CSplicedGirderData* pGirder) :
CDuctGeometry(pGirder)
{
}

bool CLinearDuctGeometry::operator==(const CLinearDuctGeometry& rOther) const
{
   return m_Points == rOther.m_Points;
}

bool CLinearDuctGeometry::operator!=(const CLinearDuctGeometry& rOther) const
{
   return !operator==(rOther);
}

void CLinearDuctGeometry::AddPoint(Float64 distFromPrev,Float64 offset,OffsetType offsetType)
{
   PointRecord record;
   record.distFromPrev = distFromPrev;
   record.offset       = offset;
   record.offsetType   = offsetType;
   m_Points.push_back(record);
}

CollectionIndexType CLinearDuctGeometry::GetPointCount() const
{
   return m_Points.size();
}

void CLinearDuctGeometry::GetPoint(CollectionIndexType pntIdx,Float64* pDistFromPrev,Float64 *pOffset,OffsetType *pOffsetType) const
{
   PointRecord record = m_Points[pntIdx];
   *pDistFromPrev     = record.distFromPrev;
   *pOffset           = record.offset;
   *pOffsetType       = record.offsetType;
}

HRESULT CLinearDuctGeometry::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("LinearDuctGeometry"),1.0);

   pStrSave->put_Property(_T("PointCount"),CComVariant(m_Points.size()));

   std::vector<PointRecord>::iterator iter(m_Points.begin());
   std::vector<PointRecord>::iterator end(m_Points.end());
   for ( ; iter != end; iter++ )
   {
      PointRecord& point = *iter;
      pStrSave->BeginUnit(_T("Point"),1.0);
      pStrSave->put_Property(_T("DistanceFromPrevious"),CComVariant(point.distFromPrev));
      pStrSave->put_Property(_T("Offset"),CComVariant(point.offset));
      pStrSave->put_Property(_T("OffsetFrom"),GetOffsetTypeProperty(point.offsetType));
      pStrSave->EndUnit();
   }

   pStrSave->EndUnit();

   return S_OK;
}

HRESULT CLinearDuctGeometry::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   pStrLoad->BeginUnit(_T("LinearDuctGeometry"));

   CComVariant var;
   var.vt = VT_INDEX;
   pStrLoad->get_Property(_T("PointCount"),&var);
   CollectionIndexType nPoints = VARIANT2INDEX(var);

   m_Points.clear();

   for ( CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      PointRecord point;
      pStrLoad->BeginUnit(_T("Point"));

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("DistanceFromPrevious"),&var);
      point.distFromPrev = var.dblVal;

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Offset"),&var);
      point.offset = var.dblVal;

      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("OffsetFrom"),&var);
      point.offsetType = GetOffsetType(var);

      m_Points.push_back(point);
      pStrLoad->EndUnit();
   }

   pStrLoad->EndUnit();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////
CParabolicDuctGeometry::CParabolicDuctGeometry()
{
}

CParabolicDuctGeometry::CParabolicDuctGeometry(const CSplicedGirderData* pGirder) :
CDuctGeometry(pGirder)
{
   Init();
}

CParabolicDuctGeometry::CParabolicDuctGeometry(const CParabolicDuctGeometry& rOther) :
CDuctGeometry(rOther)
{
   MakeCopy(rOther);
}

CParabolicDuctGeometry& CParabolicDuctGeometry::operator=(const CParabolicDuctGeometry& rOther)
{
   if ( &rOther != this )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CParabolicDuctGeometry::operator==(const CParabolicDuctGeometry& rOther) const
{
   if ( StartPoint != rOther.StartPoint )
      return false;

   if ( EndPoint != rOther.EndPoint )
      return false;

   if ( LowPoints != rOther.LowPoints )
      return false;

   if ( HighPoints != rOther.HighPoints )
      return false;

   return true;
}

bool CParabolicDuctGeometry::operator!=(const CParabolicDuctGeometry& rOther) const
{
   return !operator==(rOther);
}

void CParabolicDuctGeometry::Init()
{
   PierIndexType startPierIdx = m_pGirder->GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = m_pGirder->GetPierIndex(pgsTypes::metEnd);

   LowPoints.clear();
   HighPoints.clear();

   StartPoint.Distance   = 0;
   StartPoint.Offset     = gs_DefaultOffset2;
   StartPoint.OffsetType = CDuctGeometry::TopGirder;

   SpanIndexType startSpanIdx = startPierIdx;
   SpanIndexType endSpanIdx = endPierIdx-1;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      Point p;
      p.Distance = (spanIdx == startSpanIdx ? -0.6 : spanIdx == endSpanIdx ? -0.4 : -0.5);
      p.Offset = gs_DefaultOffset2;
      p.OffsetType = CDuctGeometry::BottomGirder;
      LowPoints.push_back(p);
   }

   for ( PierIndexType pierIdx = startPierIdx+1; pierIdx < endPierIdx; pierIdx++ )
   {
      HighPoint p;
      HighPoints.push_back(p);
   }

   EndPoint.Distance   = 0;
   EndPoint.Offset     = gs_DefaultOffset2;
   EndPoint.OffsetType = CDuctGeometry::TopGirder;

   ASSERT_VALID;
}

SpanIndexType CParabolicDuctGeometry::GetSpanCount() const
{
   return (SpanIndexType)LowPoints.size();
}

void CParabolicDuctGeometry::InsertSpan(SpanIndexType newSpanIdx)
{
   HighPoint highPoint;
   if ( 0 < HighPoints.size() )
   {
      if ( newSpanIdx < (SpanIndexType)HighPoints.size() )
      {
         highPoint = HighPoints[newSpanIdx];
         HighPoints.insert(HighPoints.begin()+newSpanIdx,highPoint);
      }
      else
      {
         HighPoints.push_back(HighPoints.back());
      }
   }
   else
   {
      HighPoints.push_back(highPoint);
   }


   Point lowPoint;
   ATLASSERT(0 < LowPoints.size()); // there is always 1 low point
   if ( newSpanIdx < (SpanIndexType)LowPoints.size() )
   {
      lowPoint = LowPoints[newSpanIdx];
      LowPoints.insert(LowPoints.begin()+newSpanIdx,lowPoint);
   }
   else
   {
      LowPoints.push_back(LowPoints.back());
   }

   ASSERT_VALID;
}

void CParabolicDuctGeometry::RemoveSpan(SpanIndexType spanIdx,PierIndexType pierIdx)
{
   SpanIndexType nSpans = (SpanIndexType)LowPoints.size();

   LowPoints.erase(LowPoints.begin()+spanIdx);

   if ( nSpans == pierIdx )
   {
      // end abutment is being removed... just pop off the last high point
      HighPoints.pop_back();
   }
   else if ( pierIdx == 0 )
   {
      // start abutment is being removed, erase the first high point
      HighPoints.erase(HighPoints.begin());
   }
   else
   {
      // an intermedate pier is being removed... remember that the container index is one less than the pier index
      HighPoints.erase(HighPoints.begin()+pierIdx-1);
   }

   ASSERT_VALID;
}

void CParabolicDuctGeometry::SetStartPoint(Float64 dist,Float64 offset,OffsetType offsetType)
{
   StartPoint.Distance   = dist;
   StartPoint.Offset     = offset;
   StartPoint.OffsetType = offsetType;
}

void CParabolicDuctGeometry::GetStartPoint(Float64 *pDist,Float64 *pOffset,OffsetType *pOffsetType) const
{
   *pDist       = StartPoint.Distance;
   *pOffset     = StartPoint.Offset;
   *pOffsetType = StartPoint.OffsetType;
}

void CParabolicDuctGeometry::SetLowPoint(SpanIndexType spanIdx,Float64   distLow,Float64   offsetLow,OffsetType lowOffsetType)
{
   ATLASSERT(m_pGirder->GetPierIndex(pgsTypes::metStart) <= spanIdx && spanIdx < m_pGirder->GetPierIndex(pgsTypes::metEnd));
   SpanIndexType startSpanIdx = (SpanIndexType)m_pGirder->GetPierIndex(pgsTypes::metStart);

   Point p;
   p.Distance   = distLow;
   p.Offset     = offsetLow;
   p.OffsetType = lowOffsetType;
   LowPoints[spanIdx-startSpanIdx] = p;
}

void CParabolicDuctGeometry::GetLowPoint(SpanIndexType spanIdx,Float64* pDistLow,Float64 *pOffsetLow,OffsetType *pLowOffsetType) const
{
   ATLASSERT(m_pGirder->GetPierIndex(pgsTypes::metStart) <= spanIdx && spanIdx < m_pGirder->GetPierIndex(pgsTypes::metEnd));
   SpanIndexType startSpanIdx = (SpanIndexType)m_pGirder->GetPierIndex(pgsTypes::metStart);

   Point p = LowPoints[spanIdx - startSpanIdx];
   *pDistLow       = p.Distance;
   *pOffsetLow     = p.Offset;
   *pLowOffsetType = p.OffsetType;
}

void CParabolicDuctGeometry::SetHighPoint(PierIndexType pierIdx,
                     Float64 distLeftIP,
                     Float64 highOffset,OffsetType highOffsetType,
                     Float64 distRightIP)
{
   ATLASSERT(m_pGirder->GetPierIndex(pgsTypes::metStart) < pierIdx && pierIdx < m_pGirder->GetPierIndex(pgsTypes::metEnd));
   ATLASSERT(0 < HighPoints.size());

   PierIndexType startPierIdx = m_pGirder->GetPierIndex(pgsTypes::metStart);


   HighPoint p;
   p.distLeftIP        = distLeftIP;

   p.highOffset        = highOffset;
   p.highOffsetType    = highOffsetType;

   p.distRightIP       = distRightIP;

   HighPoints[pierIdx-startPierIdx-1] = p;
}

void CParabolicDuctGeometry::GetHighPoint(PierIndexType pierIdx,
                     Float64* distLeftIP,
                     Float64* highOffset,OffsetType* highOffsetType,
                     Float64* distRightIP) const
{
   ATLASSERT(m_pGirder->GetPierIndex(pgsTypes::metStart) < pierIdx && pierIdx < m_pGirder->GetPierIndex(pgsTypes::metEnd));
   ATLASSERT(0 < HighPoints.size());

   PierIndexType startPierIdx = m_pGirder->GetPierIndex(pgsTypes::metStart);

   HighPoint p = HighPoints[pierIdx-startPierIdx-1];
   *distLeftIP        = p.distLeftIP;

   *highOffset        = p.highOffset;
   *highOffsetType    = p.highOffsetType;

   *distRightIP       = p.distRightIP;
}

void CParabolicDuctGeometry::SetEndPoint(Float64 dist,Float64 offset,OffsetType offsetType)
{
   EndPoint.Distance   = dist;
   EndPoint.Offset     = offset;
   EndPoint.OffsetType = offsetType;
}

void CParabolicDuctGeometry::GetEndPoint(Float64 *pDist,Float64 *pOffset,OffsetType *pOffsetType) const
{
   *pDist       = EndPoint.Distance;
   *pOffset     = EndPoint.Offset;
   *pOffsetType = EndPoint.OffsetType;
}

void CParabolicDuctGeometry::MakeCopy(const CParabolicDuctGeometry& rOther)
{
   StartPoint = rOther.StartPoint;
   EndPoint   = rOther.EndPoint;
   LowPoints  = rOther.LowPoints;
   HighPoints = rOther.HighPoints;

   ASSERT_VALID;
}

void CParabolicDuctGeometry::MakeAssignment(const CParabolicDuctGeometry& rOther)
{
   __super::MakeAssignment(rOther);
   MakeCopy(rOther);
}

HRESULT CParabolicDuctGeometry::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("ParabolicDuctGeometry"),1.0);

   pStrSave->BeginUnit(_T("StartPoint"),1.0);
   pStrSave->put_Property(_T("Distance"),CComVariant(StartPoint.Distance));
   pStrSave->put_Property(_T("Offset"),CComVariant(StartPoint.Offset));
   pStrSave->put_Property(_T("OffsetFrom"),GetOffsetTypeProperty(StartPoint.OffsetType));
   pStrSave->EndUnit();
   
   SpanIndexType nSpans = (SpanIndexType)LowPoints.size();
   if ( 1 < nSpans )
   {
      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans-1; spanIdx++ )
      {
         Point lowPoint = LowPoints[spanIdx];
         pStrSave->BeginUnit(_T("LowPoint"),1.0);
         pStrSave->put_Property(_T("Distance"),CComVariant(lowPoint.Distance));
         pStrSave->put_Property(_T("Offset"),CComVariant(lowPoint.Offset));
         pStrSave->put_Property(_T("OffsetFrom"),GetOffsetTypeProperty(lowPoint.OffsetType));
         pStrSave->EndUnit();

         HighPoint highPoint = HighPoints[spanIdx];
         pStrSave->BeginUnit(_T("HighPoint"),1.0);
         pStrSave->put_Property(_T("LeftIPDistance"),CComVariant(highPoint.distLeftIP));

         pStrSave->put_Property(_T("HighPointOffset"),CComVariant(highPoint.highOffset));
         pStrSave->put_Property(_T("HighPointOffsetFrom"),GetOffsetTypeProperty(highPoint.highOffsetType));

         pStrSave->put_Property(_T("RightIPDistance"),CComVariant(highPoint.distRightIP));
         pStrSave->EndUnit();
      }
   }

   // last low point
   Point lowPoint = LowPoints.back();
   pStrSave->BeginUnit(_T("LowPoint"),1.0);
   pStrSave->put_Property(_T("Distance"),CComVariant(lowPoint.Distance));
   pStrSave->put_Property(_T("Offset"),CComVariant(lowPoint.Offset));
   pStrSave->put_Property(_T("OffsetFrom"),GetOffsetTypeProperty(lowPoint.OffsetType));
   pStrSave->EndUnit();

   pStrSave->BeginUnit(_T("EndPoint"),1.0);
   pStrSave->put_Property(_T("Distance"),CComVariant(EndPoint.Distance));
   pStrSave->put_Property(_T("Offset"),CComVariant(EndPoint.Offset));
   pStrSave->put_Property(_T("OffsetFrom"),GetOffsetTypeProperty(EndPoint.OffsetType));
   pStrSave->EndUnit();
   
   pStrSave->EndUnit();
   return S_OK;
}

HRESULT CParabolicDuctGeometry::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CComVariant var;

   pStrLoad->BeginUnit(_T("ParabolicDuctGeometry"));

   // Start Point
   pStrLoad->BeginUnit(_T("StartPoint"));

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Distance"),&var);
   StartPoint.Distance = var.dblVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Offset"),&var);
   StartPoint.Offset = var.dblVal;

   var.vt = VT_BSTR;
   pStrLoad->get_Property(_T("OffsetFrom"),&var);
   StartPoint.OffsetType = GetOffsetType(var);
   pStrLoad->EndUnit();
   
   // Low/High Points (load until we run out of high points)
   LowPoints.clear();
   HighPoints.clear();

   bool bDone = false;
   while ( !bDone )
   {
      var.vt = VT_R8;

      Point lowPoint;
      pStrLoad->BeginUnit(_T("LowPoint"));

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Distance"),&var);
      lowPoint.Distance = var.dblVal;

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Offset"),&var);
      lowPoint.Offset = var.dblVal;

      var.vt = VT_BSTR;
      pStrLoad->get_Property(_T("OffsetFrom"),&var);
      lowPoint.OffsetType = GetOffsetType(var);
      pStrLoad->EndUnit();
      LowPoints.push_back(lowPoint);

      if ( SUCCEEDED(pStrLoad->BeginUnit(_T("HighPoint"))) )
      {
         HighPoint highPoint;
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("LeftIPDistance"),&var);
         highPoint.distLeftIP = var.dblVal;

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("HighPointOffset"),&var);
         highPoint.highOffset = var.dblVal;

         var.vt = VT_BSTR;
         pStrLoad->get_Property(_T("HighPointOffsetFrom"),&var);
         highPoint.highOffsetType = GetOffsetType(var);

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("RightIPDistance"),&var);
         highPoint.distRightIP = var.dblVal;

         pStrLoad->EndUnit();
         HighPoints.push_back(highPoint);
      }
      else
      {
         bDone = true;
      }
   }

   // End Point
   var.vt = VT_R8;
   pStrLoad->BeginUnit(_T("EndPoint"));
   pStrLoad->get_Property(_T("Distance"),&var);
   EndPoint.Distance = var.dblVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Offset"),&var);
   EndPoint.Offset = var.dblVal;

   var.vt = VT_BSTR;
   pStrLoad->get_Property(_T("OffsetFrom"),&var);
   EndPoint.OffsetType = GetOffsetType(var);
   pStrLoad->EndUnit();
   
   pStrLoad->EndUnit();


   ASSERT_VALID;

   return S_OK;
}

#if defined _DEBUG
void CParabolicDuctGeometry::AssertValid()
{
   if ( 0 < LowPoints.size() )
   {
      // if there are low points, there should be one fewer high points
      // we don't have low points in a default duct that isn't used.
      ATLASSERT(LowPoints.size() == HighPoints.size()+1);
   }
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

HRESULT COffsetDuctGeometry::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("OffsetDuctGeometry"),1.0);

   pStrSave->put_Property(_T("OffsetFromDuct"),CComVariant(RefDuctIdx));
   pStrSave->put_Property(_T("OffsetPointCount"),CComVariant(Points.size()));

   std::vector<Point>::iterator iter(Points.begin());
   std::vector<Point>::iterator end(Points.end());
   for ( ; iter != end; iter++ )
   {
      Point& point = *iter;
      pStrSave->BeginUnit(_T("Point"),1.0);
      pStrSave->put_Property(_T("Distance"),CComVariant(point.distance));
      pStrSave->put_Property(_T("Offset"),CComVariant(point.offset));
      pStrSave->EndUnit();
   }

   pStrSave->EndUnit();
   return S_OK;
}

HRESULT COffsetDuctGeometry::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   pStrLoad->BeginUnit(_T("OffsetDuctGeometry"));

   CComVariant var;
   var.vt = VT_I4;
   pStrLoad->get_Property(_T("OffsetFromDuct"),&var);
   RefDuctIdx = var.iVal;

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("OffsetPointCount"),&var);
   CollectionIndexType nPoints = var.iVal;

   Points.clear();
   for ( CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      Point point;

      pStrLoad->BeginUnit(_T("Point"));
      var.vt = VT_R8;

      pStrLoad->get_Property(_T("Distance"),&var);
      point.distance = var.dblVal;

      pStrLoad->get_Property(_T("Offset"),&var);
      point.offset = var.dblVal;

      pStrLoad->EndUnit();

      Points.push_back(point);
   }

   pStrLoad->EndUnit();
   return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
CDuctData::CDuctData()
{
   m_pPTData = NULL;

   Size = 0;
   nStrands = 0;
   bPjCalc = true;
   Pj = 0.0;
   LastUserPj = 0.0;

   DuctGeometryType = CDuctGeometry::Linear;
   JackingEnd = pgsTypes::jeLeft;
}

CDuctData::CDuctData(const CSplicedGirderData* pGirder)
{
   m_pPTData = NULL;

   Size = 0;
   nStrands = 0;
   bPjCalc = true;
   Pj = 0.0;
   LastUserPj = 0.0;

   DuctGeometryType = CDuctGeometry::Linear;
   JackingEnd = pgsTypes::jeLeft;

   Init(pGirder);
}

void CDuctData::SetGirder(const CSplicedGirderData* pGirder)
{
   LinearDuctGeometry.SetGirder(pGirder);
   ParabolicDuctGeometry.SetGirder(pGirder);
   OffsetDuctGeometry.SetGirder(pGirder);
}

void CDuctData::Init(const CSplicedGirderData* pGirder)
{
   LinearDuctGeometry.SetGirder(pGirder);
   ParabolicDuctGeometry.SetGirder(pGirder);
   ParabolicDuctGeometry.Init();
   OffsetDuctGeometry.SetGirder(pGirder);
}

bool CDuctData::operator==(const CDuctData& rOther) const
{
   if ( Size != rOther.Size )
      return false;

   if ( nStrands != rOther.nStrands )
      return false;

   if ( bPjCalc != rOther.bPjCalc )
      return false;

   if ( !IsEqual(Pj,rOther.Pj) )
      return false;

   if ( DuctGeometryType != rOther.DuctGeometryType )
      return false;

   if ( JackingEnd != rOther.JackingEnd )
      return false;

   switch (DuctGeometryType)
   {
   case CDuctGeometry::Linear:
      if ( LinearDuctGeometry != rOther.LinearDuctGeometry )
         return false;

      break;

   case CDuctGeometry::Parabolic:
      if ( ParabolicDuctGeometry != rOther.ParabolicDuctGeometry )
         return false;

      break;

   case CDuctGeometry::Offset:
      if ( OffsetDuctGeometry != rOther.OffsetDuctGeometry )
         return false;

      break;

   default:
      ATLASSERT(false); // should never get here
      return false;
   }

   return true;
}

void CDuctData::InsertSpan(SpanIndexType newSpanIndex)
{
   if ( DuctGeometryType == CDuctGeometry::Parabolic )
      ParabolicDuctGeometry.InsertSpan(newSpanIndex);
}

void CDuctData::RemoveSpan(SpanIndexType spanIdx,PierIndexType pierIdx)
{
   if ( DuctGeometryType == CDuctGeometry::Parabolic )
      ParabolicDuctGeometry.RemoveSpan(spanIdx,pierIdx);
}

HRESULT CDuctData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CComVariant var;
   ATLASSERT(m_pPTData != NULL);

   pStrLoad->BeginUnit(_T("Duct"));

   var.vt = VT_UI2;
   pStrLoad->get_Property(_T("Size"),&var);
   Size = var.uiVal;

   var.vt = VT_UI4;
   pStrLoad->get_Property(_T("NumStrands"),&var);
   nStrands = var.uiVal;

   var.vt = VT_BOOL;
   pStrLoad->get_Property(_T("CalcPj"),&var);
   bPjCalc = (var.boolVal == VARIANT_TRUE);

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Pj"),&var);
   Pj = var.dblVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("LastUserPj"),&var);
   LastUserPj = var.dblVal;

   var.vt = VT_BSTR;
   pStrLoad->get_Property(_T("StressingEnd"),&var);
   if ( CComBSTR(_T("Left")) == CComBSTR(var.bstrVal) )
      JackingEnd = pgsTypes::jeLeft;
   else if ( CComBSTR(_T("Right")) == CComBSTR(var.bstrVal) )
      JackingEnd = pgsTypes::jeRight;
   else
      JackingEnd = pgsTypes::jeBoth;

   var.vt = VT_BSTR;
   pStrLoad->get_Property(_T("Geometry"),&var);
   if ( CComBSTR(_T("Linear")) == CComBSTR(var.bstrVal) )
   {
      DuctGeometryType = CDuctGeometry::Linear;
      LinearDuctGeometry.Load(pStrLoad,pProgress);
   }
   else if ( CComBSTR(_T("Parabolic")) == CComBSTR(var.bstrVal) )
   {
      DuctGeometryType = CDuctGeometry::Parabolic;
      ParabolicDuctGeometry.Load(pStrLoad,pProgress);
   }
   else
   {
      DuctGeometryType = CDuctGeometry::Offset;
      OffsetDuctGeometry.Load(pStrLoad,pProgress);
   }

   pStrLoad->EndUnit();

   return S_OK;
}

HRESULT CDuctData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("Duct"),1.0);
   pStrSave->put_Property(_T("Size"),CComVariant(Size));
   pStrSave->put_Property(_T("NumStrands"),CComVariant(nStrands));
   pStrSave->put_Property(_T("CalcPj"),CComVariant(bPjCalc ? VARIANT_TRUE : VARIANT_FALSE));
   pStrSave->put_Property(_T("Pj"),CComVariant(Pj));
   pStrSave->put_Property(_T("LastUserPj"),CComVariant(LastUserPj));
   pStrSave->put_Property(_T("StressingEnd"),CComVariant(JackingEnd == pgsTypes::jeLeft ? _T("Left") : (JackingEnd == pgsTypes::jeRight ? _T("Right") : _T("Both"))));
   switch ( DuctGeometryType )
   {
   case CDuctGeometry::Linear:
      pStrSave->put_Property(_T("Geometry"),CComVariant(_T("Linear")));
      LinearDuctGeometry.Save(pStrSave,pProgress);
      break;
   case CDuctGeometry::Parabolic:
      pStrSave->put_Property(_T("Geometry"),CComVariant(_T("Parabolic")));
      ParabolicDuctGeometry.Save(pStrSave,pProgress);
      break;
   case CDuctGeometry::Offset:
      pStrSave->put_Property(_T("Geometry"),CComVariant(_T("Offset")));
      OffsetDuctGeometry.Save(pStrSave,pProgress);
      break;
   default:
      ATLASSERT(false); // is there a new duct geometry type???
   }

   pStrSave->EndUnit();

   return S_OK;
}

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPTData::CPTData()
{
   m_pGirder = NULL;

   nTempStrands   = 0;
   PjTemp         = 0;
   bPjTempCalc    = true;
   LastUserPjTemp = 0;

   pStrand = lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1860,matPsStrand::LowRelaxation,matPsStrand::D1524);
}  

CPTData::CPTData(const CPTData& rOther)
{
   MakeCopy(rOther);
}

CPTData::~CPTData()
{
   RemoveFromTimeline();
}

//======================== OPERATORS  =======================================
CPTData& CPTData::operator= (const CPTData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CPTData::operator==(const CPTData& rOther) const
{
   if ( pStrand != rOther.pStrand )
      return false;

   if (m_Ducts != rOther.m_Ducts)
      return false;

   if(nTempStrands != rOther.nTempStrands)
      return false;

   if(PjTemp != rOther.PjTemp)
      return false;

   if(bPjTempCalc != rOther.bPjTempCalc)
      return false;

   if(LastUserPjTemp != rOther.LastUserPjTemp)
      return false;

   return true;
}

bool CPTData::operator!=(const CPTData& rOther) const
{
   return !operator==(rOther);
}

//======================== OPERATIONS =======================================
void CPTData::SetGirder(CSplicedGirderData* pGirder)
{
   m_pGirder = pGirder;

   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator iterEnd(m_Ducts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CDuctData& duct = *iter;
      duct.Init(m_pGirder);
   }
}

CSplicedGirderData* CPTData::GetGirder()
{
   return m_pGirder;
}

const CSplicedGirderData* CPTData::GetGirder() const
{
   return m_pGirder;
}

void CPTData::AddDuct(CDuctData& duct)
{
   duct.m_pPTData = this;
   duct.Init(m_pGirder);
   m_Ducts.push_back(duct);
}

DuctIndexType CPTData::GetDuctCount() const
{
   return m_Ducts.size();
}

const CDuctData* CPTData::GetDuct(DuctIndexType idx) const
{
   return &m_Ducts[idx];
}

CDuctData* CPTData::GetDuct(DuctIndexType idx)
{
   return &m_Ducts[idx];
}

bool CPTData::CanRemoveDuct(DuctIndexType idx) const
{
   // can't remove a duct if other ducts reference it
   std::vector<CDuctData>::const_iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::const_iterator iterEnd(m_Ducts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CDuctData& ductData = *iter;
      if ( ductData.DuctGeometryType == CDuctGeometry::Offset )
      {
         if ( ductData.OffsetDuctGeometry.RefDuctIdx == idx )
            return false;
      }
   }

   return true;
}

void CPTData::RemoveDuct(DuctIndexType idx)
{
   ATLASSERT( CanRemoveDuct(idx) == true );

   // repair offset duct reference id's
   // all reference duct indices that are after the duct to be removed
   // must be decremented
   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator end(m_Ducts.end());
   for ( ; iter != end; iter++ )
   {
      CDuctData& ductData = *iter;
      if ( ductData.DuctGeometryType == CDuctGeometry::Offset )
      {
         ATLASSERT( idx != ductData.OffsetDuctGeometry.RefDuctIdx );
         if ( idx < ductData.OffsetDuctGeometry.RefDuctIdx )
            ductData.OffsetDuctGeometry.RefDuctIdx--;
      }
   }
   m_Ducts.erase( m_Ducts.begin() + idx );
}

StrandIndexType CPTData::GetNumStrands(DuctIndexType ductIndex) const
{
   return m_Ducts[ductIndex].nStrands;
}

StrandIndexType CPTData::GetNumStrands() const
{
   StrandIndexType nStrands = 0;
   std::vector<CDuctData>::const_iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::const_iterator iterEnd(m_Ducts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CDuctData& input = *iter;
      nStrands += input.nStrands;
   }
   return nStrands;
}

Float64 CPTData::GetPjack(DuctIndexType ductIndex) const
{
   if ( m_Ducts[ductIndex].bPjCalc )
      return m_Ducts[ductIndex].Pj;
   else
      return m_Ducts[ductIndex].LastUserPj;
}

void CPTData::InsertSpan(SpanIndexType newSpanIndex)
{
   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator end(m_Ducts.end());
   for ( ; iter != end; iter++ )
   {
      CDuctData& duct = *iter;
      duct.InsertSpan(newSpanIndex);
   }
}

void CPTData::RemoveSpan(SpanIndexType spanIdx,PierIndexType pierIdx)
{
   PierIndexType startPierIdx = m_pGirder->GetPierIndex(pgsTypes::metStart);
   SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;

   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator end(m_Ducts.end());
   for ( ; iter != end; iter++ )
   {
      CDuctData& duct = *iter;
      duct.RemoveSpan(spanIdx-startSpanIdx,pierIdx-startPierIdx);
   }
}

HRESULT CPTData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   CComVariant var;

   ATLASSERT(m_pGirder != NULL);

   pStrLoad->BeginUnit(_T("PTData"));

   m_Ducts.clear();

   var.vt = VT_I2;
   pStrLoad->get_Property(_T("DuctCount"),&var);
   DuctIndexType ductCount = var.iVal;

   for ( DuctIndexType ductIdx = 0; ductIdx < ductCount; ductIdx++ )
   {
      CDuctData duct(m_pGirder);
      duct.m_pPTData = this;
      duct.Load(pStrLoad,pProgress);
      m_Ducts.push_back(duct);
   }

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("NumTempStrands"), &var);
   nTempStrands = var.iVal;

   var.vt = VT_BOOL;
   pStrLoad->get_Property(_T("CalcPjTemp"),&var);
   bPjTempCalc = (var.boolVal == VARIANT_TRUE);

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("PjTemp"), &var);
   PjTemp = var.dblVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("LastUserPjTemp"), &var);
   LastUserPjTemp = var.dblVal;

   pStrLoad->EndUnit();

   return hr;
}

HRESULT CPTData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PTData"),1.0);

   DuctIndexType ductCount = m_Ducts.size();
   pStrSave->put_Property(_T("DuctCount"),CComVariant(ductCount));
   for ( DuctIndexType ductIdx = 0; ductIdx < ductCount; ductIdx++ )
   {
      m_Ducts[ductIdx].Save(pStrSave,pProgress);
   }

   pStrSave->put_Property(_T("NumTempStrands"),CComVariant(nTempStrands));
   pStrSave->put_Property(_T("CalcPjTemp"),CComVariant(bPjTempCalc ? VARIANT_TRUE : VARIANT_FALSE));
   pStrSave->put_Property(_T("PjTemp"), CComVariant(PjTemp));
   pStrSave->put_Property(_T("LastUserPjTemp"),CComVariant(LastUserPjTemp));

   pStrSave->EndUnit();

   return hr;
}

void CPTData::MakeCopy(const CPTData& rOther)
{
   RemoveFromTimeline();

   nTempStrands   = rOther.nTempStrands;
   PjTemp         = rOther.PjTemp;
   bPjTempCalc    = rOther.bPjTempCalc;
   LastUserPjTemp = rOther.LastUserPjTemp;

   m_Ducts      = rOther.m_Ducts;
   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator iterEnd(m_Ducts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CDuctData& duct = *iter;
      duct.SetGirder(m_pGirder);
   }

   pStrand = rOther.pStrand;
}

void CPTData::MakeAssignment(const CPTData& rOther)
{
   MakeCopy( rOther );
}

CTimelineManager* CPTData::GetTimelineManager()
{
   if ( m_pGirder )
   {
      CGirderGroupData* pGroup = m_pGirder->GetGirderGroup();
      if ( pGroup )
      {
         CBridgeDescription2* pBridgeDesc = pGroup->GetBridgeDescription();
         if ( pBridgeDesc )
         {
            return pBridgeDesc->GetTimelineManager();
         }
      }
   }

   return NULL;
}

void CPTData::RemoveFromTimeline()
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   if ( m_pGirder && pTimelineMgr )
   {
      CGirderKey girderKey(m_pGirder->GetGirderKey());
      DuctIndexType nDucts = GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(girderKey,ductIdx);
         CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
         pTimelineEvent->GetStressTendonActivity().RemoveTendon(girderKey,ductIdx);
      }
   }
}
