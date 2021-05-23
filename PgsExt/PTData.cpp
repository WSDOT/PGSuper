///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Float64 gs_DefaultOffset2 = ::ConvertToSysUnits(6.0,unitMeasure::Inch);


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
   {
      return CDuctGeometry::TopGirder;
   }
   else if ( CComBSTR(_T("BottomGirder")) == CComBSTR(var.bstrVal) )
   {
      return CDuctGeometry::BottomGirder;
   }

   ATLASSERT(false);
   return CDuctGeometry::BottomGirder;
}
////////////////////////////////////////////
CDuctGeometry::CDuctGeometry()
{
   m_pGirder = nullptr;
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
CDuctGeometry(pGirder),
m_MeasurementType(AlongGirder)
{
   Init();
}

CLinearDuctGeometry::CLinearDuctGeometry(const CLinearDuctGeometry& rOther) :
CDuctGeometry(rOther)
{
   MakeCopy(rOther);
}

CLinearDuctGeometry& CLinearDuctGeometry::operator=(const CLinearDuctGeometry& rOther)
{
   if ( &rOther != this )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CLinearDuctGeometry::operator==(const CLinearDuctGeometry& rOther) const
{
   return m_MeasurementType == rOther.m_MeasurementType && m_Points == rOther.m_Points;
}

bool CLinearDuctGeometry::operator!=(const CLinearDuctGeometry& rOther) const
{
   return !operator==(rOther);
}

void CLinearDuctGeometry::SetMeasurementType(CLinearDuctGeometry::MeasurementType mt)
{
   if ( m_MeasurementType != mt )
   {
      m_MeasurementType = mt;
      m_Points.clear();
   }
}

void CLinearDuctGeometry::ConvertMeasurementType(CLinearDuctGeometry::MeasurementType mt,Float64 Lg)
{
   if ( m_MeasurementType != mt )
   {
      // only convert if measurement type is changing

      if ( mt == AlongGirder )
      {
         // changing from FromPrevious to AlongGirder
         Float64 Xg = 0;
         std::vector<PointRecord>::iterator iter(m_Points.begin());
         std::vector<PointRecord>::iterator end(m_Points.end());
         for ( ; iter != end; iter++ )
         {
            PointRecord& record = *iter;
            Xg += record.location;
            record.location = Xg;
         }
      }
      else
      {
         // changing form AlongGirder to FromPrevious
         Float64 XgPrev = 0;
         std::vector<PointRecord>::iterator iter(m_Points.begin());
         std::vector<PointRecord>::iterator end(m_Points.end());
         for ( ; iter != end; iter++ )
         {
            PointRecord& record = *iter;
            Float64 Xg = record.location;
            if ( Xg < 0 )
            {
               // Xg is fractional
               Xg *= -Lg;
            }

            Float64 dXg = Xg - XgPrev;
            record.location = dXg;

            XgPrev = Xg;
         }
      }
      m_MeasurementType = mt;
   }
}

CLinearDuctGeometry::MeasurementType CLinearDuctGeometry::GetMeasurementType() const
{
   return m_MeasurementType;
}

void CLinearDuctGeometry::Clear()
{
   m_Points.clear();
}

void CLinearDuctGeometry::AddPoint(Float64 location,Float64 offset,OffsetType offsetType)
{
#if defined _DEBUG
   if ( location < 0 )
   {
      // < 0 is a fractional measure so measurementType must be AlongGirder
      ATLASSERT(m_MeasurementType == AlongGirder); 
   }

   if ( m_MeasurementType == AlongGirder && location < 0 )
   {
      // can't be more that 100% of the girder length
      ATLASSERT( ::InRange(-1.0,location,0.0) );
   }
#endif
   PointRecord record;
   record.location = location;
   record.offset       = offset;
   record.offsetType   = offsetType;
   m_Points.push_back(record);
}

CollectionIndexType CLinearDuctGeometry::GetPointCount() const
{
   return m_Points.size();
}

void CLinearDuctGeometry::GetPoint(CollectionIndexType pntIdx,Float64* pLocation,Float64 *pOffset,OffsetType *pOffsetType) const
{
   PointRecord record = m_Points[pntIdx];
   *pLocation     = record.location;
   *pOffset       = record.offset;
   *pOffsetType   = record.offsetType;
}

Float64 g_Lmax;
bool RemoveBeyondLmax(const CLinearDuctGeometry::PointRecord& pointRecord)
{
   return (g_Lmax < pointRecord.location ? true : false);
}

void CLinearDuctGeometry::RemovePoints(Float64 Lmax,Float64 Lg)
{
   MeasurementType oldMeasurementType = m_MeasurementType;
   ConvertMeasurementType(AlongGirder,Lg);
   g_Lmax = Lmax;
   m_Points.erase(std::remove_if(m_Points.begin(),m_Points.end(),RemoveBeyondLmax),m_Points.end());
   ConvertMeasurementType(oldMeasurementType,Lg);
}

void CLinearDuctGeometry::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face)
{
   // do nothing... the last point is always at the end of the bridge... the duct will stretch
   // user may have to clean up duct geometry after adding span
}

void CLinearDuctGeometry::RemoveSpan(SpanIndexType relSpanIdx,PierIndexType relPierIdx)
{
   // need to remove all points after the end of the girder
   //working here
}

HRESULT CLinearDuctGeometry::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("LinearDuctGeometry"),1.0);

   pStrSave->put_Property(_T("MeasurementType"),CComVariant(m_MeasurementType));

   pStrSave->put_Property(_T("PointCount"),CComVariant(m_Points.size()));

   std::vector<PointRecord>::iterator iter(m_Points.begin());
   std::vector<PointRecord>::iterator end(m_Points.end());
   for ( ; iter != end; iter++ )
   {
      PointRecord& point = *iter;
      pStrSave->BeginUnit(_T("Point"),1.0);
      pStrSave->put_Property(_T("Location"),CComVariant(point.location));
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
   var.vt = VT_I4;
   pStrLoad->get_Property(_T("MeasurementType"),&var);
   m_MeasurementType = (MeasurementType)var.lVal;

   var.vt = VT_INDEX;
   pStrLoad->get_Property(_T("PointCount"),&var);
   CollectionIndexType nPoints = VARIANT2INDEX(var);

   m_Points.clear();

   for ( CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      PointRecord point;
      pStrLoad->BeginUnit(_T("Point"));

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Location"),&var);
      point.location = var.dblVal;

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

void CLinearDuctGeometry::Init()
{
   m_MeasurementType = AlongGirder;
   Float64 Y = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   AddPoint( 0.0,Y,CDuctGeometry::BottomGirder);
   AddPoint(-1.0,Y,CDuctGeometry::BottomGirder);
}

void CLinearDuctGeometry::MakeCopy(const CLinearDuctGeometry& rOther)
{
   m_MeasurementType = rOther.m_MeasurementType;
   m_Points = rOther.m_Points;
}

void CLinearDuctGeometry::MakeAssignment(const CLinearDuctGeometry& rOther)
{
   __super::MakeAssignment(rOther);
   MakeCopy(rOther);
}

/////////////////////////////////////////////////////////////////////////////////////////////
CParabolicDuctGeometry::CParabolicDuctGeometry()
{
   StartPierIdx = INVALID_INDEX;
   EndPierIdx = INVALID_INDEX;
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
   if (StartPierIdx != rOther.StartPierIdx)
   {
      return false;
   }

   if ( StartPoint != rOther.StartPoint )
   {
      return false;
   }

   if (EndPierIdx != rOther.EndPierIdx)
   {
      return false;
   }

   if ( EndPoint != rOther.EndPoint )
   {
      return false;
   }

   if ( LowPoints != rOther.LowPoints )
   {
      return false;
   }

   if ( HighPoints != rOther.HighPoints )
   {
      return false;
   }

   return true;
}

bool CParabolicDuctGeometry::operator!=(const CParabolicDuctGeometry& rOther) const
{
   return !operator==(rOther);
}

void CParabolicDuctGeometry::Init()
{
   StartPierIdx = m_pGirder->GetPierIndex(pgsTypes::metStart);
   EndPierIdx   = m_pGirder->GetPierIndex(pgsTypes::metEnd);

   LowPoints.clear();
   HighPoints.clear();

   StartPoint.Distance   = 0;
   StartPoint.Offset     = gs_DefaultOffset2;
   StartPoint.OffsetType = CDuctGeometry::TopGirder;

   SpanIndexType startSpanIdx = StartPierIdx;
   SpanIndexType endSpanIdx = EndPierIdx-1;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      Point p;
      if ( startSpanIdx == endSpanIdx )
      {
         p.Distance = -0.5;
      }
      else
      {
         p.Distance = (spanIdx == startSpanIdx || spanIdx == endSpanIdx ? -0.6 : -0.5);
      }

      p.Offset = gs_DefaultOffset2;
      p.OffsetType = CDuctGeometry::BottomGirder;
      LowPoints.push_back(p);
   }

   for ( PierIndexType pierIdx = StartPierIdx+1; pierIdx < EndPierIdx; pierIdx++ )
   {
      HighPoint p;
      HighPoints.push_back(p);
   }

   EndPoint.Distance   = 0;
   EndPoint.Offset     = gs_DefaultOffset2;
   EndPoint.OffsetType = CDuctGeometry::TopGirder;

   PGS_ASSERT_VALID;
}

SpanIndexType CParabolicDuctGeometry::GetSpanCount() const
{
   SpanIndexType nSpans = (SpanIndexType)LowPoints.size();
   ATLASSERT(nSpans == (EndPierIdx - StartPierIdx));
   return nSpans;
}

void CParabolicDuctGeometry::InsertSpan(PierIndexType refPierIdx, pgsTypes::PierFaceType face)
{
   SpanIndexType spanIdx = (SpanIndexType)refPierIdx;
   if (face == pgsTypes::Back && spanIdx != 0)
   {
      spanIdx--;
   }

   HighPoint highPoint;
   if (0 < HighPoints.size())
   {
      // there are already some high points so use them as the default
      if (spanIdx < (SpanIndexType)StartPierIdx)
      {
         HighPoints.insert(HighPoints.begin(), HighPoints.front());
      }
      else if ((SpanIndexType)(EndPierIdx - 1) < spanIdx)
      {
         HighPoints.push_back(HighPoints.back());
      }
      else
      {
         // the new span is in the middle of the range so use the adjacent high point as the default
         highPoint = HighPoints[spanIdx - (SpanIndexType)StartPierIdx];
         HighPoints.insert(HighPoints.begin() + (spanIdx - (SpanIndexType)StartPierIdx), highPoint);
      }
   }
   else
   {
      // there aren't any high points so insert the default information
      HighPoints.push_back(highPoint);
   }


   Point lowPoint;
   ATLASSERT(0 < LowPoints.size()); // there is always 1 low point
   if (spanIdx < (SpanIndexType)StartPierIdx)
   {
      LowPoints.insert(LowPoints.begin(), LowPoints.front());
   }
   else if ((SpanIndexType)(EndPierIdx - 1) < spanIdx)
   {
      LowPoints.push_back(LowPoints.back());
   }
   else
   {
      lowPoint = LowPoints[spanIdx - (SpanIndexType)StartPierIdx];
      LowPoints.insert(LowPoints.begin() + (spanIdx - (SpanIndexType)StartPierIdx), lowPoint);
   }

   // a span has been inserted between StartPierIdx and EndPierIdx, therefore, the end pier must be incremented
   if (spanIdx < (SpanIndexType)StartPierIdx)
   {
      StartPierIdx--;
   }
   else
   {
      EndPierIdx++;
   }

   PGS_ASSERT_VALID;
}

void CParabolicDuctGeometry::RemoveSpan(SpanIndexType spanIdx,PierIndexType pierIdx)
{
   SpanIndexType startSpanIdx = (SpanIndexType)StartPierIdx;
   SpanIndexType endSpanIdx = (SpanIndexType)(EndPierIdx - 1);
   if (spanIdx < startSpanIdx || endSpanIdx < spanIdx)
   {
      // new span is not in the range of the tendon so there is nothing to do
      return;
   }

   SpanIndexType nSpans = GetSpanCount();

   LowPoints.erase(LowPoints.begin() + (spanIdx - (SpanIndexType)StartPierIdx));

   if ( pierIdx == EndPierIdx )
   {
      // the last pier in the range is being removed... just pop off the last high point
      HighPoints.pop_back();
   }
   else if ( pierIdx == StartPierIdx )
   {
      // the first pier in the range being removed, erase the first high point
      HighPoints.erase(HighPoints.begin());
   }
   else
   {
      // an intermedate pier is being removed... remember that the container index is one less than the pier index
      HighPoints.erase(HighPoints.begin() + (pierIdx - StartPierIdx - 1));
   }

   if (spanIdx == (SpanIndexType)StartPierIdx)
   {
      // first span is removed, so increment the start
      StartPierIdx++;
   }
   else
   {
      EndPierIdx--;
   }

   PGS_ASSERT_VALID;
}

void CParabolicDuctGeometry::SetRange(PierIndexType startPierIdx, PierIndexType endPierIdx)
{
   SpanIndexType startSpanIdx = (SpanIndexType)StartPierIdx;
   SpanIndexType endSpanIdx = (SpanIndexType)(EndPierIdx - 1);

   SpanIndexType newStartSpanIdx = (SpanIndexType)startPierIdx;
   SpanIndexType newEndSpanIdx = (SpanIndexType)(endPierIdx - 1);

   if (newStartSpanIdx < startSpanIdx)
   {
      // add low points at the start
      SpanIndexType nNewSpans = startSpanIdx - newStartSpanIdx;
      LowPoints.insert(LowPoints.begin(), nNewSpans, LowPoints.front());

      // add high points at start
      PierIndexType nNewPiers = nNewSpans;
      HighPoints.insert(HighPoints.begin(), nNewPiers, (0 < HighPoints.size() ? HighPoints.front() : HighPoint()));
   }
   else if(startSpanIdx < newStartSpanIdx)
   {
      // remove low points at the start
      SpanIndexType nRemovedSpans = newStartSpanIdx - startSpanIdx;
      LowPoints.erase(LowPoints.begin(), LowPoints.begin() + nRemovedSpans);

      // remove high points at start
      PierIndexType nRemovedPiers = nRemovedSpans;
      HighPoints.erase(HighPoints.begin(), HighPoints.begin() + nRemovedPiers);
   }


   if (endSpanIdx < newEndSpanIdx)
   {
      // add low points at end
      SpanIndexType nNewSpans = newEndSpanIdx - endSpanIdx;
      LowPoints.insert(LowPoints.end(), nNewSpans, LowPoints.back());

      // add high points at end
      PierIndexType nNewPiers = nNewSpans;
      HighPoints.insert(HighPoints.end(), nNewPiers, (0 < HighPoints.size() ? HighPoints.back() : HighPoint()));
   }
   else if (newEndSpanIdx < endSpanIdx)
   {
      // remove low points at end
      SpanIndexType nRemovedSpans = endSpanIdx - newEndSpanIdx;
      LowPoints.erase(LowPoints.end() - nRemovedSpans, LowPoints.end());

      // move high points at end
      PierIndexType nRemovedPiers = nRemovedSpans;
      HighPoints.erase(HighPoints.end() - nRemovedPiers, HighPoints.end());
   }

   StartPierIdx = startPierIdx;
   EndPierIdx = endPierIdx;

   PGS_ASSERT_VALID;
}

void CParabolicDuctGeometry::GetRange(PierIndexType* pStartPierIdx, PierIndexType* pEndPierIdx) const
{
   *pStartPierIdx = StartPierIdx;
   *pEndPierIdx = EndPierIdx;
}

void CParabolicDuctGeometry::SetStartPoint(PierIndexType pierIdx,Float64 dist,Float64 offset,OffsetType offsetType)
{
   StartPierIdx = pierIdx;
   StartPoint.Distance   = dist;
   StartPoint.Offset     = offset;
   StartPoint.OffsetType = offsetType;
}

void CParabolicDuctGeometry::GetStartPoint(PierIndexType* pPierIdx,Float64 *pDist,Float64 *pOffset,OffsetType *pOffsetType) const
{
   *pPierIdx    = StartPierIdx;
   *pDist       = StartPoint.Distance;
   *pOffset     = StartPoint.Offset;
   *pOffsetType = StartPoint.OffsetType;
}

void CParabolicDuctGeometry::SetLowPoint(SpanIndexType spanIdx,Float64 distLow,Float64 offsetLow,OffsetType lowOffsetType)
{
   ATLASSERT(StartPierIdx <= spanIdx && spanIdx <= EndPierIdx-1);

   Point p;
   p.Distance   = distLow;
   p.Offset     = offsetLow;
   p.OffsetType = lowOffsetType;
   LowPoints[spanIdx-(SpanIndexType)StartPierIdx] = p;
}

void CParabolicDuctGeometry::GetLowPoint(SpanIndexType spanIdx,Float64* pDistLow,Float64 *pOffsetLow,OffsetType *pLowOffsetType) const
{
   ATLASSERT(StartPierIdx <= spanIdx && spanIdx <= EndPierIdx-1);

   Point p = LowPoints[spanIdx - (SpanIndexType)StartPierIdx];
   *pDistLow       = p.Distance;
   *pOffsetLow     = p.Offset;
   *pLowOffsetType = p.OffsetType;
}

void CParabolicDuctGeometry::SetHighPoint(PierIndexType pierIdx,
                     Float64 distLeftIP,
                     Float64 highOffset,OffsetType highOffsetType,
                     Float64 distRightIP)
{
   ATLASSERT(StartPierIdx < pierIdx && pierIdx < EndPierIdx);
   ATLASSERT(0 < HighPoints.size());

   HighPoint p;
   p.distLeftIP        = distLeftIP;

   p.highOffset        = highOffset;
   p.highOffsetType    = highOffsetType;

   p.distRightIP       = distRightIP;

   HighPoints[pierIdx-StartPierIdx-1] = p;
}

void CParabolicDuctGeometry::GetHighPoint(PierIndexType pierIdx,
                     Float64* distLeftIP,
                     Float64* highOffset,OffsetType* highOffsetType,
                     Float64* distRightIP) const
{
   ATLASSERT(m_pGirder->GetPierIndex(pgsTypes::metStart) < pierIdx && pierIdx < m_pGirder->GetPierIndex(pgsTypes::metEnd));
   ATLASSERT(StartPierIdx < pierIdx && pierIdx < EndPierIdx);
   ATLASSERT(0 < HighPoints.size());

   HighPoint p = HighPoints[pierIdx-StartPierIdx-1];
   *distLeftIP        = p.distLeftIP;

   *highOffset        = p.highOffset;
   *highOffsetType    = p.highOffsetType;

   *distRightIP       = p.distRightIP;
}

void CParabolicDuctGeometry::SetEndPoint(PierIndexType pierIdx,Float64 dist,Float64 offset,OffsetType offsetType)
{
   EndPierIdx          = pierIdx;
   EndPoint.Distance   = dist;
   EndPoint.Offset     = offset;
   EndPoint.OffsetType = offsetType;
}

void CParabolicDuctGeometry::GetEndPoint(PierIndexType* pPierIdx,Float64 *pDist,Float64 *pOffset,OffsetType *pOffsetType) const
{
   *pPierIdx    = EndPierIdx;
   *pDist       = EndPoint.Distance;
   *pOffset     = EndPoint.Offset;
   *pOffsetType = EndPoint.OffsetType;
}

void CParabolicDuctGeometry::Shift(int nSpans)
{
   ATLASSERT(nSpans == 1 || nSpans == -1);
   ATLASSERT(nSpans == -1 ? 0 < StartPierIdx : true);
   StartPierIdx += nSpans;
   EndPierIdx += nSpans;
}

void CParabolicDuctGeometry::MakeCopy(const CParabolicDuctGeometry& rOther)
{
   StartPierIdx = rOther.StartPierIdx;
   EndPierIdx = rOther.EndPierIdx;
   StartPoint = rOther.StartPoint;
   EndPoint   = rOther.EndPoint;
   LowPoints  = rOther.LowPoints;
   HighPoints = rOther.HighPoints;

   PGS_ASSERT_VALID;
}

void CParabolicDuctGeometry::MakeAssignment(const CParabolicDuctGeometry& rOther)
{
   __super::MakeAssignment(rOther);
   MakeCopy(rOther);
}

HRESULT CParabolicDuctGeometry::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("ParabolicDuctGeometry"),1.0);

   pStrSave->BeginUnit(_T("StartPoint"),2.0);
   pStrSave->put_Property(_T("StartPier"), CComVariant(StartPierIdx)); // added in version 2
   pStrSave->put_Property(_T("Distance"),CComVariant(StartPoint.Distance));
   pStrSave->put_Property(_T("Offset"),CComVariant(StartPoint.Offset));
   pStrSave->put_Property(_T("OffsetFrom"),GetOffsetTypeProperty(StartPoint.OffsetType));
   pStrSave->EndUnit();
   
   SpanIndexType nSpans = GetSpanCount();
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

   // last low point
   Point lowPoint = LowPoints.back();
   pStrSave->BeginUnit(_T("LowPoint"),1.0);
   pStrSave->put_Property(_T("Distance"),CComVariant(lowPoint.Distance));
   pStrSave->put_Property(_T("Offset"),CComVariant(lowPoint.Offset));
   pStrSave->put_Property(_T("OffsetFrom"),GetOffsetTypeProperty(lowPoint.OffsetType));
   pStrSave->EndUnit();

   pStrSave->BeginUnit(_T("EndPoint"),2.0);
   pStrSave->put_Property(_T("EndPier"), CComVariant(EndPierIdx)); // added in version 2
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

   Float64 version;
   pStrLoad->get_Version(&version);

   if (1 < version)
   {
      // added in version 2
      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("StartPier"), &var);
      StartPierIdx = VARIANT2INDEX(var);
   }
   else
   {
      StartPierIdx = m_pGirder->GetPierIndex(pgsTypes::metStart);
   }

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

   pStrLoad->get_Version(&version);

   if (1 < version)
   {
      // added in version 2
      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("EndPier"), &var);
      EndPierIdx = VARIANT2INDEX(var);
   }
   else
   {
      EndPierIdx = m_pGirder->GetPierIndex(pgsTypes::metEnd);
   }

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


   PGS_ASSERT_VALID;

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

   // span count should match the pier range
   ATLASSERT(GetSpanCount() == (EndPierIdx - StartPierIdx));

   // there is one low point per span
   ATLASSERT(LowPoints.size() == GetSpanCount());

   // Start/End piers must be between the start and end of the girder
   if (m_pGirder)
   {
      ATLASSERT(m_pGirder->GetPierIndex(pgsTypes::metStart) <= StartPierIdx && EndPierIdx <= m_pGirder->GetPierIndex(pgsTypes::metEnd));
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
   m_pPTData = nullptr;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibraryNames,pLibNames);
   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);
   Name = vNames.front();

   pDuctLibEntry = nullptr;

   nStrands = 0;
   bPjCalc = true;
   Pj = 0.0;
   LastUserPj = 0.0;

   DuctGeometryType = CDuctGeometry::Parabolic;
   JackingEnd = pgsTypes::jeStart;
}

CDuctData::CDuctData(const CSplicedGirderData* pGirder)
{
   m_pPTData = nullptr;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibraryNames,pLibNames);
   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);
   Name = vNames.front();

   pDuctLibEntry = nullptr;

   nStrands = 0;
   bPjCalc = true;
   Pj = 0.0;
   LastUserPj = 0.0;

   DuctGeometryType = CDuctGeometry::Parabolic;
   JackingEnd = pgsTypes::jeStart;

   Init(pGirder);
}

void CDuctData::SetGirder(const CSplicedGirderData* pGirder)
{
   LinearDuctGeometry.SetGirder(pGirder);
   ParabolicDuctGeometry.SetGirder(pGirder);
   OffsetDuctGeometry.SetGirder(pGirder);
}

const CSplicedGirderData* CDuctData::GetGirder() const
{
   // all geometry types have the same girder so it doesn't matter which one we use here
   return LinearDuctGeometry.GetGirder();
}

void CDuctData::Init(const CSplicedGirderData* pGirder)
{
   LinearDuctGeometry.SetGirder(pGirder);
   LinearDuctGeometry.Init();

   ParabolicDuctGeometry.SetGirder(pGirder);
   ParabolicDuctGeometry.Init();

   OffsetDuctGeometry.SetGirder(pGirder);
}

bool CDuctData::operator==(const CDuctData& rOther) const
{
   if ( Name != rOther.Name )
   {
      return false;
   }

   if ( nStrands != rOther.nStrands )
   {
      return false;
   }

   if ( bPjCalc != rOther.bPjCalc )
   {
      return false;
   }

   if ( !IsEqual(Pj,rOther.Pj) )
   {
      return false;
   }

   if ( DuctGeometryType != rOther.DuctGeometryType )
   {
      return false;
   }

   if ( JackingEnd != rOther.JackingEnd )
   {
      return false;
   }

   switch (DuctGeometryType)
   {
   case CDuctGeometry::Linear:
      if ( LinearDuctGeometry != rOther.LinearDuctGeometry )
      {
         return false;
      }

      break;

   case CDuctGeometry::Parabolic:
      if ( ParabolicDuctGeometry != rOther.ParabolicDuctGeometry )
      {
         return false;
      }

      break;

   case CDuctGeometry::Offset:
      if ( OffsetDuctGeometry != rOther.OffsetDuctGeometry )
      {
         return false;
      }

      break;

   default:
      ATLASSERT(false); // should never get here
      return false;
   }

   return true;
}

void CDuctData::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face)
{
   if ( DuctGeometryType == CDuctGeometry::Parabolic )
   {
      ParabolicDuctGeometry.InsertSpan(refPierIdx,face);
   }
   else if ( DuctGeometryType == CDuctGeometry::Linear )
   {
      LinearDuctGeometry.InsertSpan(refPierIdx,face);
   }
   else
   {
      ATLASSERT(false);
   }
}

void CDuctData::RemoveSpan(SpanIndexType relSpanIdx,PierIndexType relPierIdx)
{
   if ( DuctGeometryType == CDuctGeometry::Parabolic )
   {
      ParabolicDuctGeometry.RemoveSpan(relSpanIdx,relPierIdx);
   }
   else if ( DuctGeometryType == CDuctGeometry::Linear )
   {
      LinearDuctGeometry.RemoveSpan(relSpanIdx,relPierIdx);
   }
   else
   {
      ATLASSERT(false);
   }
}

HRESULT CDuctData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CComVariant var;
   ATLASSERT(m_pPTData != nullptr);

   pStrLoad->BeginUnit(_T("Duct"));

   Float64 version;
   pStrLoad->get_Version(&version);

   var.vt = VT_BSTR;
   if ( FAILED(pStrLoad->get_Property(_T("Name"),&var)) )
   {
      var.vt = VT_UI4;
      pStrLoad->get_Property(_T("Size"),&var);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,ILibraryNames,pLibNames);
      std::vector<std::_tstring> vNames;
      pLibNames->EnumDuctNames(&vNames);
      Name = vNames.front();
   }
   else
   {
      USES_CONVERSION;
      Name = OLE2T(var.bstrVal);
   }

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
   {
      JackingEnd = pgsTypes::jeStart;
   }
   else if ( CComBSTR(_T("Right")) == CComBSTR(var.bstrVal) )
   {
      JackingEnd = pgsTypes::jeEnd;
   }
   else
   {
      JackingEnd = pgsTypes::jeBoth;
   }

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
   pStrSave->put_Property(_T("Name"),CComVariant(Name.c_str()));
   pStrSave->put_Property(_T("NumStrands"),CComVariant(nStrands));
   pStrSave->put_Property(_T("CalcPj"),CComVariant(bPjCalc ? VARIANT_TRUE : VARIANT_FALSE));
   pStrSave->put_Property(_T("Pj"),CComVariant(Pj));
   pStrSave->put_Property(_T("LastUserPj"),CComVariant(LastUserPj));
   pStrSave->put_Property(_T("StressingEnd"),CComVariant(JackingEnd == pgsTypes::jeStart ? _T("Left") : (JackingEnd == pgsTypes::jeEnd ? _T("Right") : _T("Both"))));
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
   m_pGirder = nullptr;

   nTempStrands   = 0;
   PjTemp         = 0;
   bPjTempCalc    = true;
   LastUserPjTemp = 0;

   DuctType = pgsTypes::dtMetal;
   InstallationType = pgsTypes::sitPush;

   pStrand = lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1860,matPsStrand::LowRelaxation,matPsStrand::None,matPsStrand::D1524);
}  

CPTData::CPTData(const CPTData& rOther)
{
   m_pGirder = nullptr;
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
   {
      return false;
   }

   if (m_Ducts != rOther.m_Ducts)
   {
      return false;
   }

   if(nTempStrands != rOther.nTempStrands)
   {
      return false;
   }

   if(PjTemp != rOther.PjTemp)
   {
      return false;
   }

   if(bPjTempCalc != rOther.bPjTempCalc)
   {
      return false;
   }

   if(LastUserPjTemp != rOther.LastUserPjTemp)
   {
      return false;
   }

   if ( DuctType != rOther.DuctType )
   {
      return false;
   }

   if ( InstallationType != rOther.InstallationType )
   {
      return false;
   }

   return true;
}

bool CPTData::operator!=(const CPTData& rOther) const
{
   return !operator==(rOther);
}

//======================== OPERATIONS =======================================
void CPTData::SetGirder(CSplicedGirderData* pGirder,bool bInit)
{
   m_pGirder = pGirder;

   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator iterEnd(m_Ducts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CDuctData& duct = *iter;
      if (bInit)
      {
         duct.Init(m_pGirder);
      }
      else
      {
         duct.SetGirder(m_pGirder);
      }
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

void CPTData::AddDuct(CDuctData& duct,EventIndexType stressTendonEventIdx)
{
   duct.m_pPTData = this;
   duct.Init(m_pGirder);
   m_Ducts.push_back(duct);

   AddToTimeline(m_Ducts.size()-1,stressTendonEventIdx);
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
         {
            return false;
         }
      }
   }

   return true;
}

void CPTData::RemoveDuct(DuctIndexType ductIdx)
{
   ATLASSERT( CanRemoveDuct(ductIdx) == true );

   // repair offset duct reference id's
   // all reference duct indices that are after the duct to be removed
   // must be decremented
   for ( auto& ductData : m_Ducts )
   {
      if ( ductData.DuctGeometryType == CDuctGeometry::Offset )
      {
         ATLASSERT(ductIdx != ductData.OffsetDuctGeometry.RefDuctIdx );
         if (ductIdx < ductData.OffsetDuctGeometry.RefDuctIdx )
         {
            ductData.OffsetDuctGeometry.RefDuctIdx--;
         }
      }
   }

   RemoveFromTimeline(ductIdx);
   m_Ducts.erase( m_Ducts.begin() + ductIdx);
}

void CPTData::RemoveDucts()
{
   DuctIndexType nDucts = GetDuctCount();
   for (DuctIndexType ductIdx = nDucts-1; ductIdx != INVALID_INDEX; ductIdx--)
   {
      RemoveDuct(ductIdx);
   }
}

StrandIndexType CPTData::GetStrandCount(DuctIndexType ductIndex) const
{
   return m_Ducts[ductIndex].nStrands;
}

StrandIndexType CPTData::GetStrandCount() const
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
   {
      return m_Ducts[ductIndex].Pj;
   }
   else
   {
      return m_Ducts[ductIndex].LastUserPj;
   }
}

void CPTData::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face)
{
   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator end(m_Ducts.end());
   for ( ; iter != end; iter++ )
   {
      CDuctData& duct = *iter;
      duct.InsertSpan(refPierIdx,face);
   }
}

void CPTData::RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType)
{
   PierIndexType startPierIdx = m_pGirder->GetPierIndex(pgsTypes::metStart);
   SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;

   PierIndexType pierIdx = (rmPierType == pgsTypes::PrevPier ? spanIdx : spanIdx+1);

   ATLASSERT(startSpanIdx <= spanIdx);
   ATLASSERT(startPierIdx <= pierIdx);

   std::vector<CDuctData>::iterator iter(m_Ducts.begin());
   std::vector<CDuctData>::iterator end(m_Ducts.end());
   for ( ; iter != end; iter++ )
   {
      CDuctData& duct = *iter;
      duct.RemoveSpan(spanIdx-startSpanIdx,pierIdx-startPierIdx);

      if (duct.DuctGeometryType == CDuctGeometry::Parabolic && spanIdx == startSpanIdx)
      {
         // if the removed span is the same as the start span, then shift the duct because all the spans/piers shift
         PierIndexType ductStartPierIdx, ductEndPierIdx;
         duct.ParabolicDuctGeometry.GetRange(&ductStartPierIdx, &ductEndPierIdx);
         ATLASSERT(0 < ductStartPierIdx && 0 < ductEndPierIdx);
         duct.ParabolicDuctGeometry.SetRange(ductStartPierIdx - 1, ductEndPierIdx - 1);
      }
   }
}

HRESULT CPTData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   CComVariant var;

   ATLASSERT(m_pGirder != nullptr);

   hr = pStrLoad->BeginUnit(_T("PTData"));
   if ( FAILED(hr) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   Float64 version;
   pStrLoad->get_Version(&version);

   var.vt = VT_I4;
   hr = pStrLoad->get_Property(_T("TendonMaterialKey"),&var);
   if ( FAILED(hr) )
   {
      // prior to verion 3 of this data block, the tendon material key was forgotten
      // so failing to load the property is ok... the default value works
      if ( 3 < version )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      Int64 key = var.lVal;
      if ( version < 4 )
      {
         key |= matPsStrand::None; // add default encoding for stand coating type... added in version 4
      }
      pStrand = pPool->GetStrand(key);
   }

   m_Ducts.clear();

   var.vt = VT_INDEX;
   hr = pStrLoad->get_Property(_T("DuctCount"),&var);
   if ( FAILED(hr) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   DuctIndexType ductCount = VARIANT2INDEX(var);

   for ( DuctIndexType ductIdx = 0; ductIdx < ductCount; ductIdx++ )
   {
      CDuctData duct(m_pGirder);
      duct.m_pPTData = this;
      hr = duct.Load(pStrLoad,pProgress);
      if ( FAILED(hr) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      m_Ducts.push_back(duct);
   }

   var.vt = VT_I4;
   hr = pStrLoad->get_Property(_T("NumTempStrands"), &var);
   if ( FAILED(hr) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }
   nTempStrands = var.iVal;

   var.vt = VT_BOOL;
   hr = pStrLoad->get_Property(_T("CalcPjTemp"),&var);
   if ( FAILED(hr) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }
   bPjTempCalc = (var.boolVal == VARIANT_TRUE);

   var.vt = VT_R8;
   hr = pStrLoad->get_Property(_T("PjTemp"), &var);
   if ( FAILED(hr) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }
   PjTemp = var.dblVal;

   var.vt = VT_R8;
   hr = pStrLoad->get_Property(_T("LastUserPjTemp"), &var);
   if ( FAILED(hr) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }
   LastUserPjTemp = var.dblVal;

   // added in version 2
   if ( 1 < version )
   {
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("DuctType"),&var);
      if ( FAILED(hr) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      DuctType = (pgsTypes::DuctType)var.lVal;
   }

   // added in version 3
   if ( 2 < version )
   {
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("InstallationType"),&var);
      if ( FAILED(hr) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      InstallationType = (pgsTypes::StrandInstallationType)var.lVal;
   }

   pStrLoad->EndUnit();

   return hr;
}

HRESULT CPTData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PTData"),4.0);

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int64 key = pPool->GetStrandKey(pStrand); // beginning with version 4, the strand key includes an including for strand coating type
   pStrSave->put_Property(_T("TendonMaterialKey"),CComVariant(key));

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

   // added in version 2
   pStrSave->put_Property(_T("DuctType"),CComVariant(DuctType));

   // added in version 3
   pStrSave->put_Property(_T("InstallationType"),CComVariant(InstallationType));

   pStrSave->EndUnit();

   return hr;
}

void CPTData::MakeCopy(const CPTData& rOther)
{
   // if the new set of PT data isn't associated with a girder, we can't
   // set the timeline events for PT. capture the current events for
   // our current PT data.
   std::vector<EventIndexType> vEvents;
   if ( rOther.GetGirder() == nullptr )
   {
      // capture the timeline events for the current ducts
      CTimelineManager* pTimelineMgr = GetTimelineManager();
      if ( m_pGirder && pTimelineMgr )
      {
         GirderIDType gdrID = m_pGirder->GetID();
         DuctIndexType nDucts = GetDuctCount();
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(gdrID,ductIdx);
            vEvents.push_back(eventIdx);
         }
      }
   }

   vEvents.resize(rOther.GetDuctCount(),vEvents.size() == 0 ? INVALID_INDEX : vEvents.back());

   RemoveFromTimeline();

   nTempStrands   = rOther.nTempStrands;
   PjTemp         = rOther.PjTemp;
   bPjTempCalc    = rOther.bPjTempCalc;
   LastUserPjTemp = rOther.LastUserPjTemp;

   m_Ducts        = rOther.m_Ducts;
   std::vector<CDuctData>::iterator ductIter(m_Ducts.begin());
   std::vector<CDuctData>::iterator ductIterEnd(m_Ducts.end());
   std::vector<CDuctData>::const_iterator otherDuctIter(rOther.m_Ducts.begin());
   std::vector<EventIndexType>::const_iterator eventIter(vEvents.begin());
   DuctIndexType ductIdx = 0;
   for ( ; ductIter != ductIterEnd; ductIter++, otherDuctIter++, ductIdx++, eventIter++ )
   {
      CDuctData& duct = *ductIter;
      duct.SetGirder(m_pGirder);

      UpdateTimeline(*otherDuctIter,ductIdx,*eventIter);
   }

   pStrand = rOther.pStrand;

   DuctType = rOther.DuctType;
   InstallationType = rOther.InstallationType;
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

   return nullptr;
}

void CPTData::RemoveFromTimeline()
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   if ( m_pGirder && pTimelineMgr )
   {
      GirderIDType gdrID = m_pGirder->GetID();
      DuctIndexType nDucts = GetDuctCount();
      for ( DuctIndexType ductIdx = nDucts-1; 0 <= ductIdx && ductIdx != INVALID_INDEX; ductIdx-- )
      {
         EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(gdrID,ductIdx);
         CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
         if ( pTimelineEvent )
         {
            pTimelineEvent->GetStressTendonActivity().RemoveTendon(gdrID,ductIdx,true/*duct is removed from bridge model*/);
         }
      }
   }
}

void CPTData::RemoveFromTimeline(DuctIndexType ductIdx)
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   if ( m_pGirder && pTimelineMgr )
   {
      GirderIDType gdrID = m_pGirder->GetID();
      EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(gdrID,ductIdx);
      CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      if ( pTimelineEvent )
      {
         pTimelineEvent->GetStressTendonActivity().RemoveTendon(gdrID,ductIdx,true/*duct is removed from bridge model*/);
      }
   }
}

void CPTData::AddToTimeline(DuctIndexType ductIdx,EventIndexType stressTendonEventIdx)
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   if ( m_pGirder && pTimelineMgr )
   {
      GirderIDType gdrID = m_pGirder->GetID();
      CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(stressTendonEventIdx);
      pTimelineEvent->GetStressTendonActivity().AddTendon(gdrID,ductIdx);
   }
}

void CPTData::UpdateTimeline(const CDuctData& otherDuct,DuctIndexType ductIdx,EventIndexType defaultEventIdx)
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   if ( pTimelineMgr )
   {
      const CSplicedGirderData* pOtherGirder = otherDuct.GetGirder();

      EventIndexType eventIdx;
      if( pOtherGirder == nullptr )
      {
         // the other set of ducts isn't associated with a girder so we'll assume it isn't in the timeline
         eventIdx = defaultEventIdx;
      }
      else
      {
         eventIdx = pTimelineMgr->GetStressTendonEventIndex(pOtherGirder->GetID(),ductIdx);
      }
      CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      pTimelineEvent->GetStressTendonActivity().AddTendon(m_pGirder->GetID(),ductIdx);
   }
}
