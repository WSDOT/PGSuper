///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\SpanData2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Float64 gs_DefaultGirderEndDistance2   = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Inch);
Float64 gs_DefaultGirderBearingOffset2 = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet);

/****************************************************************************
CLASS
   CTemporarySupportData
****************************************************************************/
CTemporarySupportData::CTemporarySupportData() :
   m_PrivateSlabOffset{0.0,0.0}
{
   m_ID    = INVALID_ID;
   m_Index = INVALID_INDEX;
   m_pSpan = nullptr;

   m_SupportType    = pgsTypes::ErectionTower;
   m_ConnectionType = pgsTypes::tsctContinuousSegment;

   m_Station = 0;
   m_strOrientation = _T("Normal");

   m_GirderEndDistance            = gs_DefaultGirderEndDistance2;
   m_EndDistanceMeasurementType   = ConnectionLibraryEntry::FromBearingNormalToPier;

   m_GirderBearingOffset          = gs_DefaultGirderBearingOffset2;
   m_BearingOffsetMeasurementType = ConnectionLibraryEntry::NormalToPier;

   m_ElevationAdjustment = 0.0;

   m_Spacing.SetTemporarySupport(this);
}

CTemporarySupportData::CTemporarySupportData(const CTemporarySupportData& rOther)
{
   m_ID    = INVALID_ID;
   m_Index = INVALID_INDEX;
   m_pSpan = nullptr;

   m_Spacing.SetTemporarySupport(this);

   MakeCopy(rOther,true/*copy only data*/);
}

CTemporarySupportData::~CTemporarySupportData()
{
   RemoveFromTimeline();
}

void CTemporarySupportData::RemoveFromTimeline()
{
   if ( m_pSpan )
   {
      CTimelineManager* pTimelineMgr = m_pSpan->GetBridgeDescription()->GetTimelineManager();

      EventIndexType erectionEventIdx, removalEventIdx;
      pTimelineMgr->GetTempSupportEvents(m_ID,&erectionEventIdx,&removalEventIdx);

      if ( erectionEventIdx != INVALID_INDEX )
      {
         pTimelineMgr->GetEventByIndex(erectionEventIdx)->GetErectPiersActivity().RemoveTempSupport(m_ID);
      }

      if ( removalEventIdx != INVALID_INDEX )
      {
         pTimelineMgr->GetEventByIndex(removalEventIdx)->GetRemoveTempSupportsActivity().RemoveTempSupport(m_ID);
      }
   }
}

CTemporarySupportData& CTemporarySupportData::operator= (const CTemporarySupportData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CTemporarySupportData::CopyTemporarySupportData(const CTemporarySupportData* pTS)
{
   MakeCopy(*pTS,true/*copy only data*/);
}

bool CTemporarySupportData::operator==(const CTemporarySupportData& rOther) const
{
   if ( m_SupportType != rOther.m_SupportType )
   {
      return false;
   }

   if ( m_ConnectionType != rOther.m_ConnectionType )
   {
      return false;
   }

   if ( !IsEqual(m_Station,rOther.m_Station) )
   {
      return false;
   }

   if ( m_strOrientation != rOther.m_strOrientation )
   {
      return false;
   }

   if ( !IsEqual(m_GirderEndDistance,rOther.m_GirderEndDistance) )
   {
      return false;
   }

   if ( m_EndDistanceMeasurementType != rOther.m_EndDistanceMeasurementType )
   {
      return false;
   }

   if ( !IsEqual( m_GirderBearingOffset, rOther.m_GirderBearingOffset) )
   {
      return false;
   }
   
   if ( m_BearingOffsetMeasurementType != rOther.m_BearingOffsetMeasurementType )
   {
      return false;
   }

   if ( HasElevationAdjustment() && !IsEqual(m_ElevationAdjustment,rOther.m_ElevationAdjustment) )
   {
      return false;
   }

   if ( m_Spacing != rOther.m_Spacing )
   {
      return false;
   }

   if ( m_pSpan && rOther.m_pSpan && m_pSpan->GetIndex() != rOther.m_pSpan->GetIndex() )
   {
      return false;
   }

   return true;
}

bool CTemporarySupportData::operator!=(const CTemporarySupportData& rOther) const
{
   return !operator==(rOther);
}

bool CTemporarySupportData::operator<(const CTemporarySupportData& rOther) const
{
   return m_Station < rOther.m_Station;
}

HRESULT CTemporarySupportData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress,CBridgeDescription2* pBridgeDesc)
{
   USES_CONVERSION;
   CHRException hr;

   try
   {
      CComVariant var;
      hr = pStrLoad->BeginUnit(_T("TemporarySupportData"));

      Float64 version;
      hr = pStrLoad->get_Version(&version);

      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_ID = VARIANT2ID(var);

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Type"),&var);
      CString strType = OLE2T(var.bstrVal);
      if ( strType == CString(_T("ErectionTower")) )
      {
         m_SupportType = pgsTypes::ErectionTower;
      }
      else if (strType == CString(_T("StrongBack")) )
      {
         m_SupportType = pgsTypes::StrongBack;
      }
      else
      {
         ATLASSERT(false);
      }

      hr = pStrLoad->get_Property(_T("ConnectionType"),&var);
      CString strConnectionType = OLE2T(var.bstrVal);
      if ( strConnectionType == CString(_T("ClosurePour")) )
      {
         m_ConnectionType = pgsTypes::tsctClosureJoint;
      }
      else if ( strConnectionType == CString(_T("ContinuousSegment")) )
      {
         m_ConnectionType = pgsTypes::tsctContinuousSegment;
      }
      else
      {
         ATLASSERT(false);
      }

#if defined _DEBUG
      if (m_SupportType == pgsTypes::StrongBack )
      {
         // can't be continuous if strong back
         ATLASSERT(m_ConnectionType != pgsTypes::tsctContinuousSegment);
      }
#endif

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Station"),&var);
      m_Station = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Orientation"),&var);
      m_strOrientation = OLE2T(var.bstrVal);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderEndDistance"),&var);
      m_GirderEndDistance = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("EndDistanceMeasurementType"),&var);
      m_EndDistanceMeasurementType = ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderBearingOffset"),&var);
      m_GirderBearingOffset = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BearingOffsetMeasurementType"),&var);
      m_BearingOffsetMeasurementType = ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(OLE2T(var.bstrVal));

      if (4 > version)
      {
         // support width removed in version 4
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SupportWidth"), &var);
      }

      if (version==5 && ( m_SupportType == pgsTypes::StrongBack || (m_SupportType == pgsTypes::ErectionTower && m_ConnectionType == pgsTypes::tsctClosureJoint )))
      {
         // added in version 5, and removed in version 6
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("BackSlabOffset"), &var);
         m_PrivateSlabOffset[pgsTypes::Back] = var.dblVal;

         hr = pStrLoad->get_Property(_T("AheadSlabOffset"), &var);
         m_PrivateSlabOffset[pgsTypes::Ahead] = var.dblVal;
      }

      if ( 2 < version )
      {
         // conditional changed in vesrion 5 from erection tower to HasElevationAdjustment()
         if ( (version < 5 && m_SupportType == pgsTypes::ErectionTower) || (4 < version && HasElevationAdjustment()) )
         {
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("ElevationAdjustment"), &var );
            m_ElevationAdjustment = var.dblVal;
         }
      }

      if ( version < 2 )
      {
         if ( m_ConnectionType != pgsTypes::tsctContinuousSegment )
         {
            m_Spacing.Load(pStrLoad,pProgress);
         }
      }
      else
      {
         if ( m_ConnectionType != pgsTypes::tsctContinuousSegment && !::IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
         {
            m_Spacing.Load(pStrLoad,pProgress);
         }
      }

      hr = pStrLoad->EndUnit();
   }
   catch(...)
   {
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return hr;
}

HRESULT CTemporarySupportData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("TemporarySupportData"),6.0);
   pStrSave->put_Property(_T("ID"),CComVariant(m_ID));
   switch( m_SupportType )

   {
   case pgsTypes::ErectionTower:
      pStrSave->put_Property(_T("Type"),CComVariant(_T("ErectionTower")));
      break;

   case pgsTypes::StrongBack:
      pStrSave->put_Property(_T("Type"),CComVariant(_T("StrongBack")));
      break;

   default:
      ATLASSERT(false);
   }

   switch( m_ConnectionType )
   {
   case pgsTypes::tsctClosureJoint:
      pStrSave->put_Property(_T("ConnectionType"),CComVariant(_T("ClosurePour")));
      break;

   case pgsTypes::tsctContinuousSegment:
      pStrSave->put_Property(_T("ConnectionType"),CComVariant(_T("ContinuousSegment")));
      break;

   default:
      ATLASSERT(false);
   }

   pStrSave->put_Property(_T("Station"),                     CComVariant(m_Station) );
   pStrSave->put_Property(_T("Orientation"),                 CComVariant( CComBSTR(m_strOrientation.c_str()) ) );
   pStrSave->put_Property(_T("GirderEndDistance"),           CComVariant( m_GirderEndDistance ) );
   pStrSave->put_Property(_T("EndDistanceMeasurementType"),  CComVariant(ConnectionLibraryEntry::StringForEndDistanceMeasurementType(m_EndDistanceMeasurementType).c_str()) );
   pStrSave->put_Property(_T("GirderBearingOffset"),         CComVariant(m_GirderBearingOffset));
   pStrSave->put_Property(_T("BearingOffsetMeasurementType"),CComVariant(ConnectionLibraryEntry::StringForBearingOffsetMeasurementType(m_BearingOffsetMeasurementType).c_str()) );
   
   // added in version 5, and removed in version 6
   //if (HasSlabOffset())
   //{
   //   pStrSave->put_Property(_T("BackSlabOffset"), CComVariant(m_SlabOffset[pgsTypes::Back]));
   //   pStrSave->put_Property(_T("AheadSlabOffset"), CComVariant(m_SlabOffset[pgsTypes::Ahead]));
   //}

   // added in version 3, changed conditional in version 5
   //if ( m_SupportType == pgsTypes::ErectionTower )
   if (HasElevationAdjustment())
   {
      pStrSave->put_Property(_T("ElevationAdjustment"), CComVariant(m_ElevationAdjustment));
   }


   // add check for IsBridgeSpacing in version 2
   if ( m_ConnectionType != pgsTypes::tsctContinuousSegment && !::IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()))
   {
      m_Spacing.Save(pStrSave,pProgress);
   }

   pStrSave->EndUnit();

   return hr;
}

void CTemporarySupportData::MakeCopy(const CTemporarySupportData& rOther,bool bCopyDataOnly)
{
   if ( !bCopyDataOnly )
   {
      m_Index = rOther.m_Index;
      m_ID    = rOther.m_ID;
   }

   m_SupportType                  = rOther.m_SupportType;
   m_ConnectionType               = rOther.m_ConnectionType;

   m_Station                      = rOther.m_Station;
   m_strOrientation               = rOther.m_strOrientation;

   m_GirderEndDistance            = rOther.m_GirderEndDistance;
   m_GirderBearingOffset          = rOther.m_GirderBearingOffset;
   m_EndDistanceMeasurementType   = rOther.m_EndDistanceMeasurementType;
   m_BearingOffsetMeasurementType = rOther.m_BearingOffsetMeasurementType;

   m_ElevationAdjustment = rOther.m_ElevationAdjustment;

   m_Spacing                      = rOther.m_Spacing;
   m_Spacing.SetTemporarySupport(this);

   PGS_ASSERT_VALID;
}

void CTemporarySupportData::MakeAssignment(const CTemporarySupportData& rOther)
{
   MakeCopy( rOther, false/*copy everything*/ );
}

void CTemporarySupportData::SetSupportType(pgsTypes::TemporarySupportType type)
{
   m_SupportType = type;
}

pgsTypes::TemporarySupportType CTemporarySupportData::GetSupportType() const
{
   return m_SupportType;
}

void CTemporarySupportData::SetConnectionType(pgsTypes::TempSupportSegmentConnectionType newType,EventIndexType castClosureJointEvent)
{
   if ( m_ConnectionType == newType )
   {
      return; // nothing changed;
   }

   CBridgeDescription2* pBridgeDesc = m_pSpan->GetBridgeDescription();
   if ( newType == pgsTypes::tsctContinuousSegment )
   {
      // before the closure joints go away, remove their casting event from the timeline
      // manager
      CClosureJointData* pClosure = GetClosureJoint(0);
      if ( pClosure )
      {
         CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
         EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure);
         pTimelineMgr->GetEventByIndex(eventIdx)->GetCastClosureJointActivity().RemoveTempSupport(GetID());
      }

      // connection has changed from closure joint to continuous segments... join segments
      CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_pSpan);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         pGirder->JoinSegmentsAtTemporarySupport(m_Index);
      }

      m_ConnectionType = newType; // this must be done last for this case
   }
   else if ( newType == pgsTypes::tsctClosureJoint )
   {
      // connection has changed from continuous to closure joint... split at this temporary support
      CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_pSpan);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         pGirder->SplitSegmentsAtTemporarySupport(m_Index);
      }

      m_ConnectionType = newType; // this must be done here for this case

      // add the closure joint casting events to the timeline manager.
      CClosureJointData* pClosure = GetClosureJoint(0);
      ClosureIDType closureID = pClosure->GetID();
      CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
      pTimelineMgr->SetCastClosureJointEventByIndex(closureID,castClosureJointEvent);

      m_Spacing.SetGirderCount(nGirders);
   }
}

pgsTypes::TempSupportSegmentConnectionType CTemporarySupportData::GetConnectionType() const
{
   return m_ConnectionType;
}

void CTemporarySupportData::SetID(SupportIDType id)
{
   m_ID = id;
}

SupportIDType CTemporarySupportData::GetID() const
{
   return m_ID;
}

void CTemporarySupportData::SetIndex(SupportIndexType idx)
{
   m_Index = idx;
}

SupportIndexType CTemporarySupportData::GetIndex() const
{
   return m_Index;
}

void CTemporarySupportData::SetStation(Float64 station)
{
   m_Station = station;
}

Float64 CTemporarySupportData::GetStation() const
{
   return m_Station;
}

LPCTSTR CTemporarySupportData::GetOrientation() const
{
   return m_strOrientation.c_str();
}

void CTemporarySupportData::SetOrientation(LPCTSTR strOrientation)
{
   m_strOrientation = strOrientation;
}

void CTemporarySupportData::SetSpan(CSpanData2* pSpan)
{
   m_pSpan = pSpan;
   PGS_ASSERT_VALID;
}

CSpanData2* CTemporarySupportData::GetSpan()
{
   return m_pSpan;
}

const CSpanData2* CTemporarySupportData::GetSpan() const
{
   return m_pSpan;
}

CClosureJointData* CTemporarySupportData::GetClosureJoint(GirderIndexType gdrIdx)
{
   if ( m_ConnectionType == pgsTypes::tsctContinuousSegment )
   {
      return nullptr;
   }

   CGirderGroupData* pGroup = m_pSpan->GetBridgeDescription()->GetGirderGroup(m_pSpan);
   CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      // NOTE: nSegments-1 because there is one less closure than segments
      // no need to check the right end of the last segment as there isn't a closure there)
      CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
      if ( pClosure->GetTemporarySupport() == this )
      {
         return pClosure;
      }
   }

   return nullptr;
}

const CClosureJointData* CTemporarySupportData::GetClosureJoint(GirderIndexType gdrIdx) const
{
   if ( m_ConnectionType == pgsTypes::tsctContinuousSegment )
   {
      return nullptr;
   }

   const CGirderGroupData* pGroup = m_pSpan->GetBridgeDescription()->GetGirderGroup(m_pSpan);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      // NOTE: nSegments-1 because there is one less closure than segments
      // no need to check the right end of the last segment as there isn't a closure there)
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
      if ( pClosure->GetTemporarySupport() == this )
      {
         return pClosure;
      }
   }

   return nullptr;
}

CBridgeDescription2* CTemporarySupportData::GetBridgeDescription()
{
   return m_pSpan ? m_pSpan->GetBridgeDescription() : nullptr;
}

const CBridgeDescription2* CTemporarySupportData::GetBridgeDescription() const
{
   return m_pSpan ? m_pSpan->GetBridgeDescription() : nullptr;
}

void CTemporarySupportData::SetGirderEndDistance(Float64 endDist,ConnectionLibraryEntry::EndDistanceMeasurementType measure)
{
   m_GirderEndDistance          = endDist;
   m_EndDistanceMeasurementType = measure;
}

void CTemporarySupportData::GetGirderEndDistance(Float64* pEndDist,ConnectionLibraryEntry::EndDistanceMeasurementType* pMeasure) const
{
   *pEndDist = m_GirderEndDistance;
   *pMeasure = m_EndDistanceMeasurementType;
}

void CTemporarySupportData::SetBearingOffset(Float64 offset,ConnectionLibraryEntry::BearingOffsetMeasurementType measure)
{
   m_GirderBearingOffset          = offset;
   m_BearingOffsetMeasurementType = measure;
}

void CTemporarySupportData::GetBearingOffset(Float64* pOffset,ConnectionLibraryEntry::BearingOffsetMeasurementType* pMeasure) const
{
   *pOffset  = m_GirderBearingOffset;
   *pMeasure = m_BearingOffsetMeasurementType;
}

void CTemporarySupportData::GetSlabOffsetPrivate(Float64* pBackSlabOffset, Float64* pAheadSlabOffset) const
{
   // Assert on old condition in a removed function called HasSlabOffset()
   ATLASSERT((m_SupportType == pgsTypes::StrongBack || (m_SupportType == pgsTypes::ErectionTower && m_ConnectionType == pgsTypes::tsctClosureJoint)));
   *pBackSlabOffset = m_PrivateSlabOffset[pgsTypes::Back];
   *pAheadSlabOffset = m_PrivateSlabOffset[pgsTypes::Ahead];
}

bool CTemporarySupportData::HasElevationAdjustment() const
{
   // elevation adjustments go hand in hand with slab offsets
   // this needs beefing up
   return m_SupportType == pgsTypes::StrongBack || (m_SupportType == pgsTypes::ErectionTower && m_ConnectionType == pgsTypes::tsctClosureJoint);
}

void CTemporarySupportData::SetElevationAdjustment(Float64 elevAdj)
{
   m_ElevationAdjustment = elevAdj;
}

Float64 CTemporarySupportData::GetElevationAdjustment() const
{
   return m_ElevationAdjustment;
}

bool CTemporarySupportData::HasSpacing() const
{
   // there is only spacing at a temporary support when the segment conneciton type has a closure joint
   return (m_ConnectionType == pgsTypes::tsctClosureJoint ? true : false);
}

void CTemporarySupportData::SetSegmentSpacing(const CGirderSpacing2& spacing)
{
   //ATLASSERT( m_ConnectionType != pgsTypes::tsctContinuousSegment );
   // this is just a warning... spacing doesn't make sence in if the connection is continuous
   // spacing is ignored if this assert fires

   m_Spacing = spacing;
}

CGirderSpacing2* CTemporarySupportData::GetSegmentSpacing()
{
   //ATLASSERT( m_ConnectionType != pgsTypes::tsctContinuousSegment );
   // this is just a warning... spacing doesn't make sence in if the connection is continuous
   // spacing is ignored if this assert fires

   return &m_Spacing;
}

const CGirderSpacing2* CTemporarySupportData::GetSegmentSpacing() const
{
   //ATLASSERT( m_ConnectionType != pgsTypes::tsctContinuousSegment );
   // this is just a warning... spacing doesn't make sence in if the connection is continuous
   // spacing is ignored if this assert fires

   return &m_Spacing;
}

LPCTSTR CTemporarySupportData::AsString(pgsTypes::TemporarySupportType type)
{
   if ( type == pgsTypes::ErectionTower )
   {
      return _T("Erection Tower");
   }
   else
   {
      return _T("Strong Back");
   }
}

LPCTSTR CTemporarySupportData::AsString(pgsTypes::TempSupportSegmentConnectionType type)
{
   if ( type == pgsTypes::tsctClosureJoint )
   {
      return _T("Closure Joint");
   }
   else
   {
      return _T("Continous Segment");
   }
}

#if defined _DEBUG
void CTemporarySupportData::AssertValid()
{
   if ( m_pSpan )
   {
      ATLASSERT(m_pSpan->GetPrevPier()->GetStation() <= m_Station && m_Station <= m_pSpan->GetNextPier()->GetStation());
      const CBridgeDescription2* pBridge = m_pSpan->GetBridgeDescription();
      ATLASSERT(pBridge->IsOnBridge(m_Station));
   }
}
#endif // _DEBUG
