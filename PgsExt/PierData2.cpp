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
#include <PgsExt\PierData2.h>
#include <PgsExt\SpanData2.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderSpacing2.h>
#include <PgsExt\ClosureJointData.h>

#include "PierData.h"

#include <Units\Convert.h>
#include <StdIo.h>
#include <StrData.cpp>
#include <WBFLCogo.h>

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// maybe not here, but somewhere... these parameters are needed so compute the column
// height if it is input by bottom elevation

/****************************************************************************
CLASS
   CPierData2
****************************************************************************/

CPierData2::CPierData2()
{
   m_PierIdx = INVALID_INDEX;
   m_PierID  = INVALID_INDEX;

   m_pPrevSpan = nullptr;
   m_pNextSpan = nullptr;

   m_pBridgeDesc = nullptr;

   m_Station     = 0.0;
   Orientation = Normal;
   Angle       = 0.0;

   m_BoundaryConditionType = pgsTypes::bctHinge;
   m_SegmentConnectionType = pgsTypes::psctContinuousSegment;

   m_strOrientation = _T("Normal");

   m_PierModelType = pgsTypes::pmtIdealized;

   m_Concrete.bHasInitial = false;
   m_Concrete.Fc = WBFL::Units::ConvertToSysUnits(4,WBFL::Units::Measure::KSI);
   m_Concrete.StrengthDensity = WBFL::Units::ConvertToSysUnits(0.145, WBFL::Units::Measure::KipPerFeet3); // plan concrete LRFD Table 3.5.1-1
   m_Concrete.WeightDensity = WBFL::Units::ConvertToSysUnits(0.155, WBFL::Units::Measure::KipPerFeet3); // 10 kcf allowance for reinforcement (BDM Ch 3).

   m_RefColumnIdx = 0;
   m_TransverseOffset = 0;
   m_TransverseOffsetMeasurement = pgsTypes::omtAlignment;
   m_XBeamHeight[pgsTypes::stLeft]  = WBFL::Units::ConvertToSysUnits(5,WBFL::Units::Measure::Feet);
   m_XBeamHeight[pgsTypes::stRight] = WBFL::Units::ConvertToSysUnits(5,WBFL::Units::Measure::Feet);
   m_XBeamTaperHeight[pgsTypes::stLeft]  = 0;
   m_XBeamTaperHeight[pgsTypes::stRight] = 0;
   m_XBeamTaperLength[pgsTypes::stLeft]  = 0;
   m_XBeamTaperLength[pgsTypes::stRight] = 0;
   m_XBeamEndSlopeOffset[pgsTypes::stLeft] = 0;
   m_XBeamEndSlopeOffset[pgsTypes::stRight] = 0;
   m_XBeamOverhang[pgsTypes::stLeft]  = WBFL::Units::ConvertToSysUnits(5,WBFL::Units::Measure::Feet);
   m_XBeamOverhang[pgsTypes::stRight] = WBFL::Units::ConvertToSysUnits(5,WBFL::Units::Measure::Feet);
   m_XBeamWidth = WBFL::Units::ConvertToSysUnits(5,WBFL::Units::Measure::Feet);

   m_ColumnFixity = pgsTypes::cftFixed;
   CColumnData defaultColumn(this);
   m_Columns.push_back(defaultColumn);

   for ( int i = 0; i < 2; i++ )
   {
      m_GirderEndDistance[i]            = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Inch);
      m_EndDistanceMeasurementType[i]   = ConnectionLibraryEntry::FromBearingNormalToPier;

      m_GirderBearingOffset[i]          = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet);
      m_BearingOffsetMeasurementType[i] = ConnectionLibraryEntry::NormalToPier;

      m_DiaphragmHeight[i] = 0;
      m_DiaphragmWidth[i] = 0;
      m_DiaphragmLoadType[i] = ConnectionLibraryEntry::DontApply;
      m_DiaphragmLoadLocation[i] = 0;
   }

   m_GirderSpacing[pgsTypes::Back].SetPier(this);
   m_GirderSpacing[pgsTypes::Ahead].SetPier(this);

   m_bDistributionFactorsFromOlderVersion = false;
}

CPierData2::CPierData2(const CPierData2& rOther)
{
   m_PierIdx = INVALID_INDEX;
   m_PierID  = INVALID_INDEX;

   m_pPrevSpan = nullptr;
   m_pNextSpan = nullptr;

   m_pBridgeDesc = nullptr;

   m_Station   = 0.0;
   Orientation = Normal;
   Angle       = 0.0;
   
   m_strOrientation = _T("Normal");

   m_GirderSpacing[pgsTypes::Back].SetPier(this);
   m_GirderSpacing[pgsTypes::Ahead].SetPier(this);

   MakeCopy(rOther,true /*copy data only*/);

   m_SegmentConnectionType = pgsTypes::psctContinuousSegment; // this must be continuous segment. this is assumed through the system for new piers. MakeCopy copied rOther's value so make it correct here!
}

CPierData2::~CPierData2()
{
   RemoveFromTimeline();
}

void CPierData2::RemoveFromTimeline()
{
   if ( m_pBridgeDesc )
   {
      CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();
      EventIndexType erectPierEventIdx = pTimelineMgr->GetPierErectionEventIndex(m_PierID);
      if ( erectPierEventIdx != INVALID_INDEX )
      {
         CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(erectPierEventIdx);
         pTimelineEvent->GetErectPiersActivity().RemovePier(m_PierID);
      }
   }
}

CPierData2& CPierData2::operator= (const CPierData2& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CPierData2::CopyPierData(const CPierData2* pPier)
{
   MakeCopy(*pPier,true /* copy data only*/);
}

bool CPierData2::operator==(const CPierData2& rOther) const
{
   if ( m_PierIdx != rOther.m_PierIdx )
   {
      return false;
   }

   if ( m_PierID != rOther.m_PierID )
   {
      return false;
   }

   if ( m_Station != rOther.m_Station )
   {
      return false;
   }

   if ( m_strOrientation != rOther.m_strOrientation )
   {
      return false;
   }

   if ( m_BoundaryConditionType != rOther.m_BoundaryConditionType )
   {
      return false;
   }

   if (m_pBridgeDesc && m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBearingLine && HasSlabOffset())
   {
      if (!IsEqual(GetSlabOffset(pgsTypes::Back), rOther.GetSlabOffset(pgsTypes::Back)) || !IsEqual(GetSlabOffset(pgsTypes::Ahead), rOther.GetSlabOffset(pgsTypes::Ahead)))
      {
         return false;
      }
   }

   if ( IsInteriorPier() )
   {
      if ( m_SegmentConnectionType != rOther.m_SegmentConnectionType )
      {
         return false;
      }
   }

   if ( m_PierModelType != rOther.m_PierModelType )
   {
      return false;
   }
   
   if ( m_PierModelType == pgsTypes::pmtPhysical )
   {
      if ( m_RefColumnIdx != rOther.m_RefColumnIdx )
      {
         return false;
      }

      if ( !IsEqual(m_TransverseOffset,rOther.m_TransverseOffset) )
      {
         return false;
      }

      if ( m_TransverseOffsetMeasurement != rOther.m_TransverseOffsetMeasurement )
      {
         return false;
      }

      if ( m_ColumnFixity != rOther.m_ColumnFixity )
      {
         return false;
      }

      if ( m_ColumnSpacing != rOther.m_ColumnSpacing )
      {
         return false;
      }

      if ( m_Columns != rOther.m_Columns )
      {
         return false;
      }
   }

   // Must update mutable bearing data before comparison
   this->ProtectBearingData();
   rOther.ProtectBearingData();

   for ( int i = 0; i < 2; i++ )
   {
      if ( m_PierModelType == pgsTypes::pmtPhysical )
      {
         pgsTypes::SideType side = (pgsTypes::SideType)i;

         if ( !IsEqual(m_XBeamHeight[side],rOther.m_XBeamHeight[side]) )
         {
            return false;
         }

         if ( !IsEqual(m_XBeamTaperHeight[side],rOther.m_XBeamTaperHeight[side]) )
         {
            return false;
         }

         if ( !IsEqual(m_XBeamTaperLength[side],rOther.m_XBeamTaperLength[side]) )
         {
            return false;
         }

         if ( !IsEqual(m_XBeamEndSlopeOffset[side],rOther.m_XBeamEndSlopeOffset[side]) )
         {
            return false;
         }

         if ( !IsEqual(m_XBeamOverhang[side],rOther.m_XBeamOverhang[side]) )
         {
            return false;
         }
      }

      if ( m_Concrete != rOther.m_Concrete )
      {
         return false;
      }

      if ( !IsEqual(m_XBeamWidth,rOther.m_XBeamWidth) )
      {
         return false;
      }

      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;

      if ( !IsEqual(m_GirderEndDistance[face], rOther.m_GirderEndDistance[face]) )
      {
         return false;
      }

      if ( m_EndDistanceMeasurementType[face] != rOther.m_EndDistanceMeasurementType[face] )
      {
         return false;
      }

      if ( !IsEqual(m_GirderBearingOffset[face], rOther.m_GirderBearingOffset[face]) )
      {
         return false;
      }

      if ( m_BearingOffsetMeasurementType[face] != rOther.m_BearingOffsetMeasurementType[face] )
      {
         return false;
      }

      if (m_pBridgeDesc->GetBearingType() != pgsTypes::brtBridge)
      {
         if (m_BearingData[face] != rOther.m_BearingData[face])
         {
            return false;
         }
      }

      if ( !IsEqual(m_DiaphragmHeight[face], rOther.m_DiaphragmHeight[face]) )
      {
         return false;
      }
      
      if ( !IsEqual(m_DiaphragmWidth[face], rOther.m_DiaphragmWidth[face]) )
      {
         return false;
      }

      if ( m_DiaphragmLoadType[face] != rOther.m_DiaphragmLoadType[face] )
      {
         return false;
      }

      if ( !IsEqual(m_DiaphragmLoadLocation[face], rOther.m_DiaphragmLoadLocation[face]) )
      {
         return false;
      }
   }

   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      if (m_LLDFs != rOther.m_LLDFs)
      {
         return false;
      }
   }

   if ( !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
   {
      if ( IsBoundaryPier() )
      {
         if ( m_pPrevSpan )
         {
            if ( m_GirderSpacing[pgsTypes::Back] != rOther.m_GirderSpacing[pgsTypes::Back] )
            {
               return false;
            }
         }

         if ( m_pNextSpan )
         {
            if ( m_GirderSpacing[pgsTypes::Ahead] != rOther.m_GirderSpacing[pgsTypes::Ahead] )
            {
               return false;
            }
         }
      }
      else if (m_SegmentConnectionType == pgsTypes::psctContinousClosureJoint || m_SegmentConnectionType == pgsTypes::psctIntegralClosureJoint)
      {
         if ( m_GirderSpacing[pgsTypes::Back] != rOther.m_GirderSpacing[pgsTypes::Back] )
         {
            return false;
         }
      }
   }

   return true;
}

bool CPierData2::operator!=(const CPierData2& rOther) const
{
   return !operator==(rOther);
}

LPCTSTR CPierData2::AsString(pgsTypes::BoundaryConditionType type,bool bNoDeck)
{
   switch(type)
   { 
   case pgsTypes::bctHinge:
      return _T("Hinge");

   case pgsTypes::bctRoller:
      return _T("Roller");

   case pgsTypes::bctContinuousAfterDeck:
      if ( bNoDeck )
         return _T("Continuous");
      else
         return _T("Continuous after deck placement");

   case pgsTypes::bctContinuousBeforeDeck:
      ATLASSERT(bNoDeck == false); // "before deck" not valid for no deck types
      return _T("Continuous before deck placement");

   case pgsTypes::bctIntegralAfterDeck:
      if ( bNoDeck )
         return _T("Integral");
      else
         return _T("Integral after deck placement");

   case pgsTypes::bctIntegralBeforeDeck:
      ATLASSERT(bNoDeck == false); // "before deck" not valid for no deck types
      return _T("Integral before deck placement");

   case pgsTypes::bctIntegralAfterDeckHingeBack:
      if ( bNoDeck )
         return _T("Hinged on back side; Integral on ahead side");
      else
         return _T("Hinged on back side; Integral on ahead side after deck placement");

   case pgsTypes::bctIntegralBeforeDeckHingeBack:
      ATLASSERT(bNoDeck == false); // "before deck" not valid for no deck types
      return _T("Hinged on back side; Integral on ahead side before deck placement");

   case pgsTypes::bctIntegralAfterDeckHingeAhead:
      if ( bNoDeck )
         return _T("Integral on back side; Hinged on ahead side");
      else
         return _T("Integral on back side after deck placement; Hinged on ahead side");

   case pgsTypes::bctIntegralBeforeDeckHingeAhead:
      ATLASSERT(bNoDeck == false); // "before deck" not valid for no deck types
      return _T("Integral on back side before deck placement; Hinged on ahead side");
   
   default:
      ATLASSERT(false);

   };

   return _T("");
}

LPCTSTR CPierData2::AsString(pgsTypes::PierSegmentConnectionType type)
{
   switch(type)
   { 
   case pgsTypes::psctContinousClosureJoint:
      return _T("Continuous Segments w/ Closure Joint");

   case pgsTypes::psctIntegralClosureJoint:
      return _T("Integral Segment w/ Closure Joint");

   case pgsTypes::psctContinuousSegment:
      return _T("Continuous Segment");

   case pgsTypes::psctIntegralSegment:
      return _T("Integral Segment");
   
   default:
      ATLASSERT(false);

   };

   return _T("");
}

HRESULT CPierData2::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   // Make sure outside changes to the bridge haven't made our data out of sync
   ProtectBearingData();
   pStrSave->BeginUnit(_T("PierDataDetails"),21.0);
   
   pStrSave->put_Property(_T("ID"),CComVariant(m_PierID));

   pStrSave->put_Property(_T("Station"),         CComVariant(m_Station) );
   pStrSave->put_Property(_T("Orientation"),     CComVariant( CComBSTR(m_strOrientation.c_str()) ) );
   pStrSave->put_Property(_T("IsBoundaryPier"),  CComVariant( IsBoundaryPier() ? VARIANT_TRUE : VARIANT_FALSE) ); // added in version 11

   pStrSave->put_Property(_T("PierConnectionType"),  CComVariant( m_BoundaryConditionType ) ); // changed from left and right to a single value in version 7
   if ( IsInteriorPier() )
   {
      pStrSave->put_Property(_T("SegmentConnectionType"),  CComVariant( m_SegmentConnectionType ) );
   }

   // added in version 13, removed in version 20
   //if ( m_bHasCantilever )
   //{
   //   pStrSave->put_Property(_T("CantileverLength"),CComVariant(m_CantileverLength));
   //}

   // added in version 19
   pStrSave->put_Property(_T("BackSlabOffset"), CComVariant(m_SlabOffset[pgsTypes::Back]));
   pStrSave->put_Property(_T("AheadSlabOffset"), CComVariant(m_SlabOffset[pgsTypes::Ahead]));

   pStrSave->put_Property(_T("PierModelType"),CComVariant(m_PierModelType));
   if ( m_PierModelType == pgsTypes::pmtPhysical )
   {
      //pStrSave->put_Property(_T("Ec"),CComVariant(m_Ec)); // removed in version 15
      m_Concrete.Save(pStrSave,pProgress); // added in version 15

      pStrSave->put_Property(_T("RefColumnIndex"),CComVariant(m_RefColumnIdx));
      pStrSave->put_Property(_T("TransverseOffset"),CComVariant(m_TransverseOffset));
      pStrSave->put_Property(_T("TransverseOffsetMeasurement"),CComVariant(m_TransverseOffsetMeasurement));
      pStrSave->put_Property(_T("XBeamHeight_Left"),CComVariant(m_XBeamHeight[pgsTypes::stLeft]));
      pStrSave->put_Property(_T("XBeamHeight_Right"),CComVariant(m_XBeamHeight[pgsTypes::stRight]));
      pStrSave->put_Property(_T("XBeamTaperHeight_Left"),CComVariant(m_XBeamTaperHeight[pgsTypes::stLeft]));
      pStrSave->put_Property(_T("XBeamTaperHeight_Right"),CComVariant(m_XBeamTaperHeight[pgsTypes::stRight]));
      pStrSave->put_Property(_T("XBeamTaperLength_Left"),CComVariant(m_XBeamTaperLength[pgsTypes::stLeft]));
      pStrSave->put_Property(_T("XBeamTaperLength_Right"),CComVariant(m_XBeamTaperLength[pgsTypes::stRight]));
      pStrSave->put_Property(_T("XBeamEndSlopeOffset_Left"), CComVariant(m_XBeamEndSlopeOffset[pgsTypes::stLeft])); // added version 17
      pStrSave->put_Property(_T("XBeamEndSlopeOffset_Right"), CComVariant(m_XBeamEndSlopeOffset[pgsTypes::stRight])); // added version 17
      pStrSave->put_Property(_T("XBeamOverhang_Left"),CComVariant(m_XBeamOverhang[pgsTypes::stLeft]));
      pStrSave->put_Property(_T("XBeamOverhang_Right"),CComVariant(m_XBeamOverhang[pgsTypes::stRight]));
      pStrSave->put_Property(_T("XBeamWidth"),CComVariant(m_XBeamWidth));

      pStrSave->put_Property(_T("ColumnFixity"),CComVariant(m_ColumnFixity)); // added version 16

      pStrSave->put_Property(_T("ColumnCount"),CComVariant(m_Columns.size()));
      std::vector<Float64>::iterator spacingIter = m_ColumnSpacing.begin();
      std::vector<CColumnData>::iterator columnIterBegin = m_Columns.begin();
      std::vector<CColumnData>::iterator columnIter = columnIterBegin;
      std::vector<CColumnData>::iterator columnIterEnd = m_Columns.end();
      for ( ; columnIter != columnIterEnd; columnIter++ )
      {
         if ( columnIter != columnIterBegin )
         {
            Float64 spacing = *spacingIter;
            pStrSave->put_Property(_T("Spacing"),CComVariant(spacing));
            spacingIter++;
         }
         CColumnData& columnData = *columnIter;
         columnData.Save(pStrSave,pProgress);
      }
   }

   pStrSave->BeginUnit(_T("Back"),2.0);
   pStrSave->put_Property(_T("GirderEndDistance"),CComVariant( m_GirderEndDistance[pgsTypes::Back] ) );
   pStrSave->put_Property(_T("EndDistanceMeasurementType"), CComVariant(ConnectionLibraryEntry::StringForEndDistanceMeasurementType(m_EndDistanceMeasurementType[pgsTypes::Back]).c_str()) );
   pStrSave->put_Property(_T("GirderBearingOffset"),CComVariant(m_GirderBearingOffset[pgsTypes::Back]));
   pStrSave->put_Property(_T("BearingOffsetMeasurementType"),CComVariant(ConnectionLibraryEntry::StringForBearingOffsetMeasurementType(m_BearingOffsetMeasurementType[pgsTypes::Back]).c_str()) );

   if (m_pBridgeDesc->GetBearingType() != pgsTypes::brtBridge)
   {
      pStrSave->BeginUnit(_T("BearingDataVec"), 1.0);

      // if bearing data is by pier, only save first entry
      if (m_pBridgeDesc->GetBearingType() == pgsTypes::brtPier)
      {
         std::size_t cnt = m_BearingData[pgsTypes::Back].size();
         cnt = cnt > 0 ? 1 : 0;
         pStrSave->put_Property(_T("Count"), CComVariant(cnt));
         if (cnt > 0)
         {
            m_BearingData[pgsTypes::Back][0].Save(pStrSave, pProgress);
         }
      }
      else
      {
         pStrSave->put_Property(_T("Count"), CComVariant(m_BearingData[pgsTypes::Back].size()));
         for (const auto& val : m_BearingData[pgsTypes::Back])
         {
            val.Save(pStrSave, pProgress);
         }
      }

      pStrSave->EndUnit();
   }

   if ( HasSpacing() )
   {
      m_GirderSpacing[pgsTypes::Back].Save(pStrSave,pProgress);
   }

   {
      pStrSave->BeginUnit(_T("Diaphragm"),1.0);
      pStrSave->put_Property(_T("DiaphragmWidth"),  CComVariant(m_DiaphragmWidth[pgsTypes::Back]));
      pStrSave->put_Property(_T("DiaphragmHeight"), CComVariant(m_DiaphragmHeight[pgsTypes::Back]));

      if (m_DiaphragmLoadType[pgsTypes::Back] == ConnectionLibraryEntry::ApplyAtBearingCenterline)
      {
         pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("ApplyAtBearingCenterline")));
      }
      else if (m_DiaphragmLoadType[pgsTypes::Back] == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
      {
         pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("ApplyAtSpecifiedLocation")));
         pStrSave->put_Property(_T("DiaphragmLoadLocation"),CComVariant(m_DiaphragmLoadLocation[pgsTypes::Back]));
      }
      else if (m_DiaphragmLoadType[pgsTypes::Back] == ConnectionLibraryEntry::DontApply)
      {
         pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("DontApply")));
      }
      else
      {
         ATLASSERT(false); // is there a new load type?
      }
      pStrSave->EndUnit(); // Diaphragm
   }
   pStrSave->EndUnit(); // Back

   pStrSave->BeginUnit(_T("Ahead"),2.0);
   pStrSave->put_Property(_T("GirderEndDistance"),CComVariant( m_GirderEndDistance[pgsTypes::Ahead] ) );
   pStrSave->put_Property(_T("EndDistanceMeasurementType"), CComVariant(ConnectionLibraryEntry::StringForEndDistanceMeasurementType(m_EndDistanceMeasurementType[pgsTypes::Ahead]).c_str()) );
   pStrSave->put_Property(_T("GirderBearingOffset"),CComVariant(m_GirderBearingOffset[pgsTypes::Ahead]));
   pStrSave->put_Property(_T("BearingOffsetMeasurementType"),CComVariant(ConnectionLibraryEntry::StringForBearingOffsetMeasurementType(m_BearingOffsetMeasurementType[pgsTypes::Ahead]).c_str()) );

   if (m_pBridgeDesc->GetBearingType() != pgsTypes::brtBridge)
   {
      pStrSave->BeginUnit(_T("BearingDataVec"), 1.0);

      // if bearing data is by pier, only save first entry
      if (m_pBridgeDesc->GetBearingType() == pgsTypes::brtPier)
      {
         std::size_t cnt = m_BearingData[pgsTypes::Ahead].size();
         cnt = cnt > 0 ? 1 : 0;
         pStrSave->put_Property(_T("Count"), CComVariant(cnt));
         if (cnt > 0)
         {
            m_BearingData[pgsTypes::Ahead][0].Save(pStrSave, pProgress);
         }
      }
      else
      {
         pStrSave->put_Property(_T("Count"), CComVariant(m_BearingData[pgsTypes::Ahead].size()));
         for (const auto& val : m_BearingData[pgsTypes::Ahead])
         {
            val.Save(pStrSave, pProgress);
         }
      }

      pStrSave->EndUnit();
   }

   if ( HasSpacing() )
   {
      m_GirderSpacing[pgsTypes::Ahead].Save(pStrSave,pProgress);
   }

   {
      pStrSave->BeginUnit(_T("Diaphragm"),1.0);
      pStrSave->put_Property(_T("DiaphragmWidth"),  CComVariant(m_DiaphragmWidth[pgsTypes::Ahead]));
      pStrSave->put_Property(_T("DiaphragmHeight"), CComVariant(m_DiaphragmHeight[pgsTypes::Ahead]));

      if (m_DiaphragmLoadType[pgsTypes::Ahead] == ConnectionLibraryEntry::ApplyAtBearingCenterline)
      {
         pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("ApplyAtBearingCenterline")));
      }
      else if (m_DiaphragmLoadType[pgsTypes::Ahead] == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
      {
         pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("ApplyAtSpecifiedLocation")));
         pStrSave->put_Property(_T("DiaphragmLoadLocation"),CComVariant(m_DiaphragmLoadLocation[pgsTypes::Ahead]));
      }
      else if (m_DiaphragmLoadType[pgsTypes::Ahead] == ConnectionLibraryEntry::DontApply)
      {
         pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("DontApply")));
      }
      else
      {
         ATLASSERT(false); // is there a new load type?
      }
      pStrSave->EndUnit(); // Diaphragm
   }
   pStrSave->EndUnit(); // Ahead

   // added in version 5
   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pStrSave->BeginUnit(_T("LLDF"),4.0); // Version 3 went from interior/exterior to girder by girder
                                           // Version 4 got rid of gR's

      GirderIndexType ngs = GetLldfGirderCount();
      pStrSave->put_Property(_T("nLLDFGirders"),CComVariant(ngs));

      for (GirderIndexType igs=0; igs<ngs; igs++)
      {
         pStrSave->BeginUnit(_T("LLDF_Girder"),1.0);
         LLDF& lldf = GetLLDF(igs);

         pStrSave->put_Property(_T("gM_Strength"), CComVariant(lldf.gM[0]));
         pStrSave->put_Property(_T("gM_Fatigue"),  CComVariant(lldf.gM[1]));
         pStrSave->EndUnit(); // LLDF_Girder
      }

      pStrSave->EndUnit(); // LLDF
   }

   pStrSave->EndUnit();

   return hr;
}

HRESULT CPierData2::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;
   CHRException hr;

   try
   {
      HRESULT hr2 = pStrLoad->BeginUnit(_T("PierDataDetails"));
      if ( FAILED(hr2) )
      {
         hr = pStrLoad->BeginUnit(_T("PierData"));
         Float64 version;
         pStrLoad->get_Version(&version);
         hr = LoadOldPierData(version,pStrLoad,pProgress,_T("PierData"));
         pStrLoad->EndUnit(); // PierData

         return hr;
      }

      Float64 version;
      pStrLoad->get_Version(&version);
      if ( version < 10 )
      {
         hr = LoadOldPierData(version,pStrLoad,pProgress,_T("PierDataDetails"));
         pStrLoad->EndUnit(); // PierDataDetails

         return hr;
      }

      CComVariant var;
      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_PierID = VARIANT2ID(var);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Station"),&var);
      m_Station = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Orientation"), &var );
      m_strOrientation = OLE2T(var.bstrVal);

      VARIANT_BOOL vbIsBoundaryPier = VARIANT_TRUE;
      if ( 10 < version )
      {
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("IsBoundaryPier"),&var);
         vbIsBoundaryPier = var.boolVal;
      }

      if ( 11 < version )
      {
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("PierConnectionType"),&var);
         m_BoundaryConditionType = (pgsTypes::BoundaryConditionType)var.lVal;

         if ( vbIsBoundaryPier == VARIANT_FALSE )
         {
            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("SegmentConnectionType"),&var);
            m_SegmentConnectionType = (pgsTypes::PierSegmentConnectionType)var.lVal;
         }
      }
      else if ( 10 < version && version < 12 )
      {
         if ( vbIsBoundaryPier == VARIANT_TRUE )
         {
            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("PierConnectionType"),&var);
            m_BoundaryConditionType = (pgsTypes::BoundaryConditionType)var.lVal;
         }
         else
         {
            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("SegmentConnectionType"),&var);
            m_SegmentConnectionType = (pgsTypes::PierSegmentConnectionType)var.lVal;
         }
      }
      else
      {
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("PierConnectionType"),&var);
         m_BoundaryConditionType = (pgsTypes::BoundaryConditionType)var.lVal;

         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("SegmentConnectionType"),&var);
         m_SegmentConnectionType = (pgsTypes::PierSegmentConnectionType)var.lVal;
      }

      std::array<bool, 2> bCantilever{ false,false }; // if we load cantilever information, skip connection information below
      if ( 12 < version && version < 20)
      {
         // added in version 13, removed in version 20
         var.vt = VT_R8;
         if ( SUCCEEDED(pStrLoad->get_Property(_T("CantileverLength"),&var)) )
         {
            // convert span/pier defined cantilever into equivalent connection geometry
            ATLASSERT(m_PierIdx != INVALID_INDEX); // we are counting on the pier index being assigned
            pgsTypes::PierFaceType face = (m_PierIdx == 0 ? pgsTypes::Ahead : pgsTypes::Back);
            bCantilever[face] = true;
            m_GirderEndDistance[face] = var.dblVal; // cantilever length is now the girder end distance, measured from the CLBearing
            m_EndDistanceMeasurementType[face] = ConnectionLibraryEntry::FromBearingNormalToPier;

            m_GirderBearingOffset[face] = 0; // bearing offset is at the reference line
            m_BearingOffsetMeasurementType[face] = ConnectionLibraryEntry::NormalToPier;
         }
      }

      if (18 < version)
      {
         // added in version 19
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("BackSlabOffset"), &var);
         m_SlabOffset[pgsTypes::Back] = var.dblVal;

         hr = pStrLoad->get_Property(_T("AheadSlabOffset"), &var);
         m_SlabOffset[pgsTypes::Ahead] = var.dblVal;
      }

      if ( 13 < version )
      {
         // added in version 14
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("PierModelType"),&var);
         m_PierModelType = (pgsTypes::PierModelType)var.lVal;

         if ( m_PierModelType == pgsTypes::pmtPhysical )
         {
            if ( version < 15 )
            {
               // Version 15 and Ec was during beta-development of PGSuper/PGSplice
               // Most users probably haven't modeled the columns so it doesn't matter
               // was Ec is.... just load it and ignore it and use the default for m_Concrete
               var.vt = VT_R8;
               hr = pStrLoad->get_Property(_T("Ec"),&var);
            }
            else
            {
               hr = m_Concrete.Load(pStrLoad,pProgress);
            }

            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("RefColumnIndex"),&var);
            m_RefColumnIdx = VARIANT2INDEX(var);

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("TransverseOffset"),&var);
            m_TransverseOffset = var.dblVal;

            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("TransverseOffsetMeasurement"),&var);
            m_TransverseOffsetMeasurement = (pgsTypes::OffsetMeasurementType)var.lVal;

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("XBeamHeight_Left"),&var);
            m_XBeamHeight[pgsTypes::stLeft] = var.dblVal;

            hr = pStrLoad->get_Property(_T("XBeamHeight_Right"),&var);
            m_XBeamHeight[pgsTypes::stRight] = var.dblVal;

            hr = pStrLoad->get_Property(_T("XBeamTaperHeight_Left"),&var);
            m_XBeamTaperHeight[pgsTypes::stLeft] = var.dblVal;

            hr = pStrLoad->get_Property(_T("XBeamTaperHeight_Right"),&var);
            m_XBeamTaperHeight[pgsTypes::stRight] = var.dblVal;

            hr = pStrLoad->get_Property(_T("XBeamTaperLength_Left"),&var);
            m_XBeamTaperLength[pgsTypes::stLeft] = var.dblVal;

            hr = pStrLoad->get_Property(_T("XBeamTaperLength_Right"),&var);
            m_XBeamTaperLength[pgsTypes::stRight] = var.dblVal;

            if ( 16 < version )
            {
               // Added version 17
               hr = pStrLoad->get_Property(_T("XBeamEndSlopeOffset_Left"), &var);
               m_XBeamEndSlopeOffset[pgsTypes::stLeft] = var.dblVal;

               hr = pStrLoad->get_Property(_T("XBeamEndSlopeOffset_Right"), &var);
               m_XBeamEndSlopeOffset[pgsTypes::stRight] = var.dblVal;
            }

            hr = pStrLoad->get_Property(_T("XBeamOverhang_Left"),&var);
            m_XBeamOverhang[pgsTypes::stLeft] = var.dblVal;

            hr = pStrLoad->get_Property(_T("XBeamOverhang_Right"),&var);
            m_XBeamOverhang[pgsTypes::stRight] = var.dblVal;

            hr = pStrLoad->get_Property(_T("XBeamWidth"),&var);
            m_XBeamWidth = var.dblVal;

            if ( 15 < version )
            {
               // added int version 16
               var.vt = VT_I4;
               hr = pStrLoad->get_Property(_T("ColumnFixity"),&var);
               m_ColumnFixity = (pgsTypes::ColumnLongitudinalBaseFixityType)var.lVal;

               // NOTE: during the original implemention of v3 (PGSplice)
               // I thought we could model the longitudinal base fixity of the pier
               // as either fixed or pinned. This would give bounded results for
               // flexable footing systems. However, it turns out that the bridge
               // model is unstable before continuity is achieved if there is a
               // pin at the base of the column. For this reason, the UI has been
               // changed to allow only fixed columns and we are forcing the bridge
               // model to use only fixed base columns as well.
               if (m_ColumnFixity == pgsTypes::cftPinned)
               {
                  m_ColumnFixity = pgsTypes::cftFixed;
               }
            }


            m_Columns.clear();
            m_ColumnSpacing.clear();
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("ColumnCount"),&var);
            ColumnIndexType nColumns = VARIANT2INDEX(var);
            for ( ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++ )
            {
               if ( 0 < colIdx )
               {
                  var.vt = VT_R8;
                  hr = pStrLoad->get_Property(_T("Spacing"),&var);
                  Float64 spacing = var.dblVal;
                  m_ColumnSpacing.push_back(spacing);
               }
               CColumnData columnData(this);
               columnData.Load(pStrLoad,pProgress);
               m_Columns.push_back(columnData);
            }
         }
      }
      
      hr = pStrLoad->BeginUnit(_T("Back"));

      Float64 back_version;
      pStrLoad->get_Version(&back_version);

      // NOTE: for the next 4 items... starting with version 20, the explicit cantilever dimension is removed and cantilevers are modeled
      // exclusively with connection geometry. If we loaded a cantilever dimension above (as defined by bCantilever[]), the don't alter
      // the connection information (we still have to load the information, but we just discard it)
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderEndDistance"),&var);
      m_GirderEndDistance[pgsTypes::Back] = bCantilever[pgsTypes::Back] ? m_GirderEndDistance[pgsTypes::Back] : var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("EndDistanceMeasurementType"),&var);
      m_EndDistanceMeasurementType[pgsTypes::Back] = bCantilever[pgsTypes::Back] ? m_EndDistanceMeasurementType[pgsTypes::Back] : ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderBearingOffset"),&var);
      m_GirderBearingOffset[pgsTypes::Back] = bCantilever[pgsTypes::Back] ? m_GirderBearingOffset[pgsTypes::Back] : var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BearingOffsetMeasurementType"),&var);
      m_BearingOffsetMeasurementType[pgsTypes::Back] = bCantilever[pgsTypes::Back] ? m_BearingOffsetMeasurementType[pgsTypes::Back] : ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(OLE2T(var.bstrVal));

      // Some gymnastics to load support width from older versions and assign to bearingdata. Assign to first bearing on bearing line
      if (version < 18)
      {
         Float64 suppwidth;
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SupportWidth"), &var);
         suppwidth = var.dblVal;

         CBearingData2 bd;
         bd.Length = suppwidth;
         m_BearingData[pgsTypes::Back].push_back(bd);
      }
      else
      {
         // Bearing Data in vers 18 or later
         if (m_pBridgeDesc->GetBearingType() != pgsTypes::brtBridge)
         {
            hr = pStrLoad->BeginUnit(_T("BearingDataVec"));

            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            Uint32 cnt = var.lVal;

            try 
            {
            for (Uint32 ib = 0; ib < cnt; ib++)
            {
               CBearingData2 bd;
               hr = bd.Load(pStrLoad, pProgress);
               m_BearingData[pgsTypes::Back].push_back(bd);
            }

            hr = pStrLoad->EndUnit();
         }
            catch (...)
            {
               // This was a bug in pgsuper version 4.0.10. The program could write out a value of cnt==1, but then have no data to read.
               ATLASSERT(cnt == 1);
               hr = pStrLoad->EndUnit();
            }
         }
      }

      if ( back_version < 2 )
      {
         if ( !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
         {
            hr = m_GirderSpacing[pgsTypes::Back].Load(pStrLoad,pProgress);
         }
      }
      else
      {
         if ( HasSpacing(vbIsBoundaryPier) )
         {
            hr = m_GirderSpacing[pgsTypes::Back].Load(pStrLoad,pProgress);
         }
      }

      {
         hr = pStrLoad->BeginUnit(_T("Diaphragm"));
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("DiaphragmWidth"),&var);
         m_DiaphragmWidth[pgsTypes::Back] = var.dblVal;
         if (m_DiaphragmWidth[pgsTypes::Back] < 0)
         {
            // there was a bug that split the -1 between two sides of the pier
            // so the resulting width was taken to be -0.5... this is wrong
            // reset to -1
            m_DiaphragmWidth[pgsTypes::Back] = -1;
         }

         hr = pStrLoad->get_Property(_T("DiaphragmHeight"),&var);
         m_DiaphragmHeight[pgsTypes::Back] = var.dblVal;

         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("DiaphragmLoadType"),&var);
         if ( FAILED(hr) )
         {
            // there was a bug in version 2.8.2 that caused the DiaphragmLoadType to
            // be omitted when it was set to "DontApply". If there is a problem loading
            // the DiaphragmLoadType, assume it should be "DontApply"
            var.bstrVal = T2BSTR(_T("DontApply"));
         }

         std::_tstring tmp(OLE2T(var.bstrVal));
         if (tmp == _T("ApplyAtBearingCenterline"))
         {
            m_DiaphragmLoadType[pgsTypes::Back] = ConnectionLibraryEntry::ApplyAtBearingCenterline;
         }
         else if (tmp == _T("ApplyAtSpecifiedLocation"))
         {
            m_DiaphragmLoadType[pgsTypes::Back] = ConnectionLibraryEntry::ApplyAtSpecifiedLocation;

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("DiaphragmLoadLocation"),&var);
            m_DiaphragmLoadLocation[pgsTypes::Back] = var.dblVal;
         }
         else if (tmp == _T("DontApply"))
         {
            m_DiaphragmLoadType[pgsTypes::Back] = ConnectionLibraryEntry::DontApply;
         }
         else
         {
            hr = STRLOAD_E_INVALIDFORMAT;
         }

         hr = pStrLoad->EndUnit(); // Diaphragm
      }

      hr = pStrLoad->EndUnit(); // Back

      hr = pStrLoad->BeginUnit(_T("Ahead"));

      Float64 ahead_version;
      pStrLoad->get_Version(&ahead_version);

      // NOTE: for the next 4 items... starting with version 20, the explicit cantilever dimension is removed and cantilevers are modeled
      // exclusively with connection geometry. If we loaded a cantilever dimension above (as defined by bCantilever[]), the don't alter
      // the connection information (we still have to load the information, but we just discard it)
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderEndDistance"),&var);
      m_GirderEndDistance[pgsTypes::Ahead] = bCantilever[pgsTypes::Ahead] ? m_GirderEndDistance[pgsTypes::Ahead] : var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("EndDistanceMeasurementType"),&var);
      m_EndDistanceMeasurementType[pgsTypes::Ahead] = bCantilever[pgsTypes::Ahead] ? m_EndDistanceMeasurementType[pgsTypes::Ahead] : ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderBearingOffset"),&var);
      m_GirderBearingOffset[pgsTypes::Ahead] = bCantilever[pgsTypes::Ahead] ? m_GirderBearingOffset[pgsTypes::Ahead] : var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BearingOffsetMeasurementType"),&var);
      m_BearingOffsetMeasurementType[pgsTypes::Ahead] = bCantilever[pgsTypes::Ahead] ? m_BearingOffsetMeasurementType[pgsTypes::Ahead] : ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(OLE2T(var.bstrVal));

      if (version < 18)
      {
         Float64 suppwidth;
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SupportWidth"), &var);
         suppwidth = var.dblVal;

         CBearingData2 bd;
         bd.Length = suppwidth;
         m_BearingData[pgsTypes::Ahead].push_back(bd);
      }
      else
      {
         // Bearing Data in vers 18 or later
         if (m_pBridgeDesc->GetBearingType() != pgsTypes::brtBridge)
         {
            hr = pStrLoad->BeginUnit(_T("BearingDataVec"));

            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            Uint32 cnt = var.lVal;

            try
            {
            for (Uint32 ib = 0; ib < cnt; ib++)
            {
               CBearingData2 bd;
               hr = bd.Load(pStrLoad, pProgress);
               m_BearingData[pgsTypes::Ahead].push_back(bd);
            }

            hr = pStrLoad->EndUnit();
            }
            catch (...)
            {
               // This was a bug in pgsuper version 4.0.10. The program could write out a value of cnt==1, but then have no data to read.
               ATLASSERT(cnt == 1);
               hr = pStrLoad->EndUnit();
            }
         }
      }

      if ( ahead_version < 2 )
      {
         if ( !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
         {
            hr = m_GirderSpacing[pgsTypes::Ahead].Load(pStrLoad,pProgress);
         }
      }
      else
      {
         if ( HasSpacing(vbIsBoundaryPier) )
         {
            hr = m_GirderSpacing[pgsTypes::Ahead].Load(pStrLoad,pProgress);
         }
      }


      {
         hr = pStrLoad->BeginUnit(_T("Diaphragm"));
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("DiaphragmWidth"),&var);
         m_DiaphragmWidth[pgsTypes::Ahead] = var.dblVal;
         if (m_DiaphragmWidth[pgsTypes::Ahead] < 0)
         {
            // there was a bug that split the -1 between two sides of the pier
            // so the resulting width was taken to be -0.5... this is wrong
            // reset to -1
            m_DiaphragmWidth[pgsTypes::Ahead] = -1;
         }

         hr = pStrLoad->get_Property(_T("DiaphragmHeight"),&var);
         m_DiaphragmHeight[pgsTypes::Ahead] = var.dblVal;

         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("DiaphragmLoadType"),&var);
         if ( FAILED(hr) )
         {
            // there was a bug in version 2.8.2 that caused the DiaphragmLoadType to
            // be omitted when it was set to "DontApply". If there is a problem loading
            // the DiaphragmLoadType, assume it should be "DontApply"
            var.bstrVal = T2BSTR(_T("DontApply"));
         }

         std::_tstring tmp(OLE2T(var.bstrVal));
         if (tmp == _T("ApplyAtBearingCenterline"))
         {
            m_DiaphragmLoadType[pgsTypes::Ahead] = ConnectionLibraryEntry::ApplyAtBearingCenterline;
         }
         else if (tmp == _T("ApplyAtSpecifiedLocation"))
         {
            m_DiaphragmLoadType[pgsTypes::Ahead] = ConnectionLibraryEntry::ApplyAtSpecifiedLocation;

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("DiaphragmLoadLocation"),&var);
            m_DiaphragmLoadLocation[pgsTypes::Ahead] = var.dblVal;
         }
         else if (tmp == _T("DontApply"))
         {
            m_DiaphragmLoadType[pgsTypes::Ahead] = ConnectionLibraryEntry::DontApply;
         }
         else
         {
            hr = STRLOAD_E_INVALIDFORMAT;
         }

         hr = pStrLoad->EndUnit(); // Diaphragm
      }
      hr = pStrLoad->EndUnit(); // Ahead


      if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
      {
         pStrLoad->BeginUnit(_T("LLDF"));
         Float64 lldf_version;
         pStrLoad->get_Version(&lldf_version);

         if ( lldf_version < 3 )
         {
            // Prior to version 3, factors were for interior and exterior only
            Float64 gM[2][2];

            if ( lldf_version < 2 )
            {
               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gM_Interior"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gM[pgsTypes::Interior][0] = var.dblVal;
               gM[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gM_Exterior"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gM[pgsTypes::Exterior][0] = var.dblVal;
               gM[pgsTypes::Exterior][1] = var.dblVal;

               // Reaction lldf's were removed in version 4 (pierdata version 21). Load them, but don't use them
               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Interior"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }
            }
            else
            {
               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gM_Interior_Strength"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gM[pgsTypes::Interior][0] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gM_Exterior_Strength"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gM[pgsTypes::Exterior][0] = var.dblVal;

               // Reaction lldf's were removed in pierdata version 21. Load them, but don't use them
               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Interior_Strength"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior_Strength"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gM_Interior_Fatigue"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gM[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gM_Exterior_Fatigue"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gM[pgsTypes::Exterior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Interior_Fatigue"),&var)) ) // no longer used
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior_Fatigue"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }
            }

            // Move interior and exterior factors into first two slots in df vector. We will 
            // need to move them into all girder slots once this object is fully connected to the bridge
            m_bDistributionFactorsFromOlderVersion = true;

            LLDF df;
            df.gM[0] = gM[pgsTypes::Exterior][0];
            df.gM[1] = gM[pgsTypes::Exterior][1];

            m_LLDFs.push_back(df); // First in list is exterior

            df.gM[0] = gM[pgsTypes::Interior][0];
            df.gM[1] = gM[pgsTypes::Interior][1];

            m_LLDFs.push_back(df); // Second is interior
         }
         else
         {
            // distribution factors by girder
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("nLLDFGirders"),&var);
            IndexType ng = VARIANT2INDEX(var);

            var.vt = VT_R8;

            for (IndexType ig=0; ig<ng; ig++)
            {
               LLDF lldf;

               hr = pStrLoad->BeginUnit(_T("LLDF_Girder"));

               hr = pStrLoad->get_Property(_T("gM_Strength"),&var);
               lldf.gM[0] = var.dblVal;

               if (lldf_version < 4)
               {
                  hr = pStrLoad->get_Property(_T("gR_Strength"), &var); // no longer used in version 4
               }

               hr = pStrLoad->get_Property(_T("gM_Fatigue"),&var);
               lldf.gM[1] = var.dblVal;

               if (lldf_version < 4)
               {
                  hr = pStrLoad->get_Property(_T("gR_Fatigue"), &var);
               }

               pStrLoad->EndUnit(); // LLDF

               m_LLDFs.push_back(lldf);
            }
         }

         pStrLoad->EndUnit();
      }

      hr = pStrLoad->EndUnit(); // PierDataDetails
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   PGS_ASSERT_VALID;

   return S_OK;
}

void CPierData2::MakeCopy(const CPierData2& rOther,bool bCopyDataOnly)
{
   if ( !bCopyDataOnly )
   {
      m_PierIdx = rOther.m_PierIdx;
      m_PierID  = rOther.m_PierID;
   }

   m_Station               = rOther.m_Station;
   m_strOrientation        = rOther.m_strOrientation;
   Orientation             = rOther.Orientation;
   Angle                   = rOther.Angle;

   m_PierModelType = rOther.m_PierModelType;
   m_RefColumnIdx = rOther.m_RefColumnIdx;
   m_TransverseOffset = rOther.m_TransverseOffset;
   m_TransverseOffsetMeasurement = rOther.m_TransverseOffsetMeasurement;

   m_SlabOffset = rOther.m_SlabOffset;

   m_ColumnFixity = rOther.m_ColumnFixity;
   m_ColumnSpacing = rOther.m_ColumnSpacing;
   m_Columns = rOther.m_Columns;

   m_Concrete = rOther.m_Concrete;
   m_XBeamWidth = rOther.m_XBeamWidth;

   for ( int i = 0; i < 2; i++ )
   {
      m_XBeamHeight[i] = rOther.m_XBeamHeight[i];
      m_XBeamTaperHeight[i] = rOther.m_XBeamTaperHeight[i];
      m_XBeamTaperLength[i] = rOther.m_XBeamTaperLength[i];
      m_XBeamEndSlopeOffset[i] = rOther.m_XBeamEndSlopeOffset[i];
      m_XBeamOverhang[i]    = rOther.m_XBeamOverhang[i];

      m_GirderEndDistance[i]            = rOther.m_GirderEndDistance[i];
      m_EndDistanceMeasurementType[i]   = rOther.m_EndDistanceMeasurementType[i];
      m_GirderBearingOffset[i]          = rOther.m_GirderBearingOffset[i];
      m_BearingOffsetMeasurementType[i] = rOther.m_BearingOffsetMeasurementType[i];
      m_BearingData[i]                  = rOther.m_BearingData[i];

      m_GirderSpacing[i] = rOther.m_GirderSpacing[i];
      m_GirderSpacing[i].SetPier(this);

      m_DiaphragmHeight[i]       = rOther.m_DiaphragmHeight[i];       
      m_DiaphragmWidth[i]        = rOther.m_DiaphragmWidth[i];
      m_DiaphragmLoadType[i]     = rOther.m_DiaphragmLoadType[i];
      m_DiaphragmLoadLocation[i] = rOther.m_DiaphragmLoadLocation[i];
   }

   m_LLDFs = rOther.m_LLDFs;
   m_bDistributionFactorsFromOlderVersion = rOther.m_bDistributionFactorsFromOlderVersion;
   
   if ( m_pBridgeDesc && rOther.GetBridgeDescription() )
   {
      // If this pier is part of a bridge, use the SetXXXConnectionType method so
      // girder segments are split/joined as necessary for the new connection types
      if ( IsBoundaryPier() )
      {
         SetBoundaryConditionType(rOther.m_BoundaryConditionType);
      }
      else
      {
         const CClosureJointData* pClosure = rOther.GetClosureJoint(0);
         EventIndexType eventIdx = rOther.GetBridgeDescription()->GetTimelineManager()->GetCastClosureJointEventIndex(pClosure);
         SetSegmentConnectionType(rOther.m_SegmentConnectionType,eventIdx);
      }
   }
   else
   {
      // If this pier is not part of a bridge, just capture the data
      m_BoundaryConditionType = rOther.m_BoundaryConditionType;
      m_SegmentConnectionType = rOther.m_SegmentConnectionType;
   }

   PGS_ASSERT_VALID;
}

void CPierData2::MakeAssignment(const CPierData2& rOther)
{
   MakeCopy( rOther, false /*copy everything*/ );
}

void CPierData2::SetIndex(PierIndexType pierIdx)
{
   m_PierIdx = pierIdx;
}

void CPierData2::SetID(PierIDType pierID)
{
   m_PierID = pierID;
}

PierIndexType CPierData2::GetIndex() const
{
   return m_PierIdx;
}

PierIDType CPierData2::GetID() const
{
   return m_PierID;
}

void CPierData2::SetBridgeDescription(CBridgeDescription2* pBridge)
{
   m_pBridgeDesc = pBridge;
}

const CBridgeDescription2* CPierData2::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

CBridgeDescription2* CPierData2::GetBridgeDescription()
{
   return m_pBridgeDesc;
}

void CPierData2::SetSpan(pgsTypes::PierFaceType face,CSpanData2* pSpan)
{
   if ( face == pgsTypes::Back )
   {
      m_pPrevSpan = pSpan;
   }
   else
   {
      m_pNextSpan = pSpan;
   }

   ValidateBoundaryConditionType();
}

void CPierData2::SetSpans(CSpanData2* pPrevSpan,CSpanData2* pNextSpan)
{
   m_pPrevSpan = pPrevSpan;
   m_pNextSpan = pNextSpan;
}

CSpanData2* CPierData2::GetPrevSpan()
{
   return m_pPrevSpan;
}

CSpanData2* CPierData2::GetNextSpan()
{
   return m_pNextSpan;
}

CSpanData2* CPierData2::GetSpan(pgsTypes::PierFaceType face)
{
   return (face == pgsTypes::Ahead ? m_pNextSpan : m_pPrevSpan);
}

const CSpanData2* CPierData2::GetPrevSpan() const
{
   return m_pPrevSpan;
}

const CSpanData2* CPierData2::GetNextSpan() const
{
   return m_pNextSpan;
}

const CSpanData2* CPierData2::GetSpan(pgsTypes::PierFaceType face) const
{
   return (face == pgsTypes::Ahead ? m_pNextSpan : m_pPrevSpan);
}

CGirderGroupData* CPierData2::GetPrevGirderGroup()
{
   return m_pBridgeDesc ? m_pBridgeDesc->GetGirderGroup(m_pPrevSpan) : nullptr;
}

CGirderGroupData* CPierData2::GetNextGirderGroup()
{
   return m_pBridgeDesc ? m_pBridgeDesc->GetGirderGroup(m_pNextSpan) : nullptr;
}

CGirderGroupData* CPierData2::GetGirderGroup(pgsTypes::PierFaceType face)
{
   return (face == pgsTypes::Ahead ? GetNextGirderGroup() : GetPrevGirderGroup());
}

const CGirderGroupData* CPierData2::GetPrevGirderGroup() const
{
   return (m_pBridgeDesc ? m_pBridgeDesc->GetGirderGroup(m_pPrevSpan) : nullptr);
}

const CGirderGroupData* CPierData2::GetNextGirderGroup() const
{
   return (m_pBridgeDesc ? m_pBridgeDesc->GetGirderGroup(m_pNextSpan) : nullptr);
}

const CGirderGroupData* CPierData2::GetGirderGroup(pgsTypes::PierFaceType face) const
{
   return (face == pgsTypes::Ahead ? GetNextGirderGroup() : GetPrevGirderGroup());
}

Float64 CPierData2::GetStation() const
{
   return m_Station;
}

void CPierData2::SetStation(Float64 station)
{
   m_Station = station;
}

LPCTSTR CPierData2::GetOrientation() const
{
   return m_strOrientation.c_str();
}

void CPierData2::SetOrientation(LPCTSTR strOrientation)
{
   m_strOrientation = strOrientation;
}

pgsTypes::BoundaryConditionType CPierData2::GetBoundaryConditionType() const
{
   ATLASSERT(IsBoundaryPier()); // only applicable to boundary piers
   return m_BoundaryConditionType;
}

void CPierData2::SetBoundaryConditionType(pgsTypes::BoundaryConditionType newType)
{
   pgsTypes::BoundaryConditionType oldType = m_BoundaryConditionType;
   if (oldType == newType)
   {
      return; // nothing is changing
   }

   m_BoundaryConditionType = newType;

   if (!m_pBridgeDesc)
   {
      return; // can't do anything else if this pier isn't attached to a bridge
   }

   // fix up the deck casting model
   EventIndexType castDeckEventIdx = m_pBridgeDesc->GetTimelineManager()->GetCastDeckEventIndex();
   if (castDeckEventIdx != INVALID_INDEX)
   {
      CTimelineEvent* pEvent = m_pBridgeDesc->GetTimelineManager()->GetEventByIndex(castDeckEventIdx);
      CCastDeckActivity& castDeckActivity = pEvent->GetCastDeckActivity();
      PierIndexType pierIdx = GetIndex();

      bool bIsOldBCContinuous = IsContinuousBoundaryCondition(oldType);
      bool bIsNewBCContinuous = IsContinuousBoundaryCondition(newType);
      if (bIsOldBCContinuous && !bIsNewBCContinuous)
      {
         // remove neg moment casting region for this pier
         castDeckActivity.RemoveNegMomentRegion(pierIdx);
      }
      else if (!bIsOldBCContinuous && bIsNewBCContinuous)
      {
         // add neg moment casting region for this pier
         castDeckActivity.AddNegMomentRegion(pierIdx);
      }
   }
}

pgsTypes::PierSegmentConnectionType CPierData2::GetSegmentConnectionType() const
{
   ATLASSERT(IsInteriorPier()); //  only applicable to interior piers
   return m_SegmentConnectionType;
}

void CPierData2::SetSegmentConnectionType(pgsTypes::PierSegmentConnectionType newType,EventIndexType castClosureJointEvent)
{
   pgsTypes::PierSegmentConnectionType oldType = m_SegmentConnectionType;

   if ( oldType == newType )
   {
      return; // nothing is changing
   }

   if ( !m_pBridgeDesc )
   {
      m_SegmentConnectionType = newType;
      return; // can't do anything else if this pier isn't attached to a bridge
   }

   if ( newType == pgsTypes::psctContinuousSegment || newType == pgsTypes::psctIntegralSegment )
   {
      // connection has changed to continuous segments... join segments at this pier
      CGirderGroupData* pGroup = GetGirderGroup(pgsTypes::Ahead);
      ATLASSERT(pGroup == GetGirderGroup(pgsTypes::Back)); // this pier must be in the middle of a group
                                                             // to make the segments continuous

      // remove the closure joints at this pier from the timeline manager
      CClosureJointData* pClosure = GetClosureJoint(0);
      if ( pClosure )
      {
         CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();
         EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure);
         pTimelineMgr->GetEventByIndex(eventIdx)->GetCastClosureJointActivity().RemovePier(GetID());
      }

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         pGirder->JoinSegmentsAtPier(m_PierIdx);
      }

      // now that the segments are jointed, change the connection type at this pier
      m_SegmentConnectionType = newType;
   }
   else if ( oldType == pgsTypes::psctContinuousSegment || oldType == pgsTypes::psctIntegralSegment )
   {
      // connection has changed from continuous segments... split segments at this pier

      CGirderGroupData* pGroup = GetGirderGroup(pgsTypes::Ahead);
      ATLASSERT(pGroup == GetGirderGroup(pgsTypes::Back)); // this pier must be in the middle of a group
                                                            // to make the segments continuous

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         pGirder->SplitSegmentsAtPier(m_PierIdx); // a closure joint is modeled in a "default" event... usually the same event as other closure joints are installed
      }

      // now that the segments are split, change the connection type at this pier
      m_SegmentConnectionType = newType;

      // add the closure joint casting events to the timeline manager.
      CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();
      CClosureJointData* pClosureJoint = GetClosureJoint(0);
      pTimelineMgr->SetCastClosureJointEventByIndex(pClosureJoint, castClosureJointEvent);

      m_GirderSpacing[pgsTypes::Back].SetGirderCount(nGirders);
      m_GirderSpacing[pgsTypes::Ahead].SetGirderCount(nGirders);
   }
   else
   {
      // type has changed, but no modifications are required
      m_SegmentConnectionType = newType;
   }
}

CPierData2::PierConnectionFlags CPierData2::IsConnectionDataAvailable() const
{
   // First check if we possibly have connection data. Exterior piers always do
   bool hasConnectionData(true);
   if (IsInteriorPier())
   {
      hasConnectionData = !(m_SegmentConnectionType == pgsTypes::psctContinuousSegment || m_SegmentConnectionType == pgsTypes::psctIntegralSegment);
   }

   if (hasConnectionData)
   {
      int startface = (GetPrevSpan() != nullptr) ? 1 : 0;
      int endface   = (GetNextSpan() != nullptr) ? 2 : 0;

      CPierData2::PierConnectionFlags flag = (CPierData2::PierConnectionFlags)(startface | endface);
      return flag;
   }
   else
   {
      return  pcfNoConnections;
   }
}

bool CPierData2::SetGirderEndDistance(pgsTypes::PierFaceType face,Float64 endDist,ConnectionLibraryEntry::EndDistanceMeasurementType measure)
{
   if (endDist != m_GirderEndDistance[face] || measure != m_EndDistanceMeasurementType[face])
   {
      m_GirderEndDistance[face] = endDist;
      m_EndDistanceMeasurementType[face] = measure;
      return true;
   }
   else
   {
      return false;
   }
}

void CPierData2::GetGirderEndDistance(pgsTypes::PierFaceType face,Float64* pEndDist,ConnectionLibraryEntry::EndDistanceMeasurementType* pMeasure,bool bRaw) const
{
#ifdef DEBUG
   // Check if caller has any business asking for connection data
   if (!bRaw)
   {
      CPierData2::PierConnectionFlags flag = IsConnectionDataAvailable();
      if (pgsTypes::Ahead == face)
      {
         ATLASSERT(pcfBothFaces == flag || pcfAheadOnly == flag);
      }
      else
      {
         ATLASSERT(pcfBothFaces == flag || pcfBackOnly == flag);
      }
   }
#endif // DEBUG

   *pEndDist = m_GirderEndDistance[face];
   *pMeasure = m_EndDistanceMeasurementType[face];
}

bool CPierData2::SetBearingOffset(pgsTypes::PierFaceType face, Float64 offset, ConnectionLibraryEntry::BearingOffsetMeasurementType measure)
{
   if (m_GirderBearingOffset[face] != offset || m_BearingOffsetMeasurementType[face] != measure)
   {
      m_GirderBearingOffset[face] = offset;
      m_BearingOffsetMeasurementType[face] = measure;
      return true;
   }
   else
   {
      return false;
   }
}

void CPierData2::GetBearingOffset(pgsTypes::PierFaceType face,Float64* pOffset,ConnectionLibraryEntry::BearingOffsetMeasurementType* pMeasure,bool bRaw) const
{
#ifdef DEBUG
   // Check if caller has any business asking for connection data
   if (!bRaw)
   {
      CPierData2::PierConnectionFlags flag = IsConnectionDataAvailable();
      if (pgsTypes::Ahead == face)
      {
         ATLASSERT(pcfBothFaces == flag || pcfAheadOnly == flag);
      }
      else
      {
         ATLASSERT(pcfBothFaces == flag || pcfBackOnly == flag);
      }
   }
#endif // DEBUG

   *pOffset = m_GirderBearingOffset[face];
   *pMeasure = m_BearingOffsetMeasurementType[face];
}

void CPierData2::SetBearingData(pgsTypes::PierFaceType face, const CBearingData2 & bd)
{
   // Interior piers have a single bearing - use ahead slot to store
   if (IsInteriorPier())
   {
      face = pgsTypes::Ahead;
   }

   m_BearingData[face].clear();
   m_BearingData[face].push_back(bd);

   ProtectBearingData();
}

void CPierData2::SetBearingData(GirderIndexType gdrIdx, pgsTypes::PierFaceType face, const CBearingData2 & bd)
{
   if (IsInteriorPier())
   {
      face = pgsTypes::Ahead;
   }

   ProtectBearingData();
   ATLASSERT(gdrIdx < m_BearingData[face].size());

   m_BearingData[face][gdrIdx] = bd;
}

void CPierData2::MirrorBearingData(pgsTypes::PierFaceType face)
{
   if (face==pgsTypes::Ahead)
   {
      m_BearingData[pgsTypes::Back] = m_BearingData[pgsTypes::Ahead];
   }
   else
   {
      m_BearingData[pgsTypes::Ahead] = m_BearingData[pgsTypes::Back];
   }
}

CBearingData2* CPierData2::GetBearingData(GirderIndexType gdrIdx, pgsTypes::PierFaceType face)
{
   ProtectBearingData();
   ATLASSERT(gdrIdx < m_BearingData[face].size());

   return &m_BearingData[face][gdrIdx];
}

const CBearingData2* CPierData2::GetBearingData(GirderIndexType gdrIdx, pgsTypes::PierFaceType face) const
{
   ProtectBearingData();
   ATLASSERT(gdrIdx < m_BearingData[face].size());

   return &m_BearingData[face][gdrIdx];
}

Float64 CPierData2::GetSupportWidth(GirderIndexType gdrIdx, pgsTypes::PierFaceType face) const
{
   ProtectBearingData();
   ATLASSERT(gdrIdx < m_BearingData[face].size());

   return m_BearingData[face][gdrIdx].Length;
}

Float64 CPierData2::GetNetBearingHeight(GirderIndexType gdrIdx, pgsTypes::PierFaceType face) const
{
   ProtectBearingData();
   ATLASSERT(gdrIdx < m_BearingData[face].size());

   const CBearingData2& rb = m_BearingData[face][gdrIdx];
   return rb.GetNetBearingHeight();
}

void CPierData2::ProtectBearingData() const
{
   // This function keeps bearing data from being compromized when the bearing type (bridge, pier, girder) changes
   ATLASSERT(0 == (int)pgsTypes::Ahead); // whole scheme below breaks if this is not true

   for (Uint32 i = 0; i < 2; i++)
   {
      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;

      const CSpanData2* pSpan = GetSpan(face);
      if (pSpan)
      {
         // First: Compare size of our collection with current number of girders and resize if they don't match
         GirderIndexType nGirders = max(1, GetGirderCount(face));
         IndexType nBrgs = m_BearingData[face].size();

         if (nBrgs == 0)
         {
            // probably switched from brtBridge. Get bearing data from bridge and assign as a default
            const CBearingData2* pdefVal = m_pBridgeDesc->GetBearingData();
            m_BearingData[face].assign(nGirders, *pdefVal);
         }
         else if (nBrgs < nGirders)
         {
            // More girders than data - use back value for remaining girders
            CBearingData2 back = m_BearingData[face].back();

            m_BearingData[face].reserve(nGirders); // performance
            for (IndexType i = nBrgs; i < nGirders; i++)
            {
               m_BearingData[face].push_back(back);
            }
         }
         else if (nGirders < nBrgs)
         {
            // more bearings than girders - truncate
            m_BearingData[face].resize(nGirders);
         }
      }

      if (i == 0 && IsInteriorPier())
      {
         // Interior piers have a single bearing line. Enforce this
         m_BearingData[pgsTypes::Back] = m_BearingData[pgsTypes::Ahead];
         break;
      }
   }
}

GirderIndexType CPierData2::GetGirderCount(pgsTypes::PierFaceType face) const
{
   const CSpanData2* pSpan = GetSpan(face);
   if ( pSpan )
   {
      const CGirderGroupData* pGroup = pSpan->GetBridgeDescription()->GetGirderGroup(pSpan);
      return pGroup->GetGirderCount();
   }
   else
   {
      ATLASSERT(0); // probably should be protected by caller
      return 0;
   }
}

void CPierData2::SetGirderSpacing(pgsTypes::PierFaceType pierFace,const CGirderSpacing2& spacing)
{
   m_GirderSpacing[pierFace] = spacing;
   m_GirderSpacing[pierFace].SetPier(this);
}

CGirderSpacing2* CPierData2::GetGirderSpacing(pgsTypes::PierFaceType pierFace)
{
   return &m_GirderSpacing[pierFace];
}

const CGirderSpacing2* CPierData2::GetGirderSpacing(pgsTypes::PierFaceType pierFace) const
{
   return &m_GirderSpacing[pierFace];
}

CClosureJointData* CPierData2::GetClosureJoint(GirderIndexType gdrIdx)
{
   if ( IsBoundaryPier() )
   {
      ATLASSERT(false); // why are you asking for a closure joint at a boundary pier? it doesn't have one
      return nullptr;
   }

   if ( m_SegmentConnectionType == pgsTypes::psctContinuousSegment || m_SegmentConnectionType == pgsTypes::psctIntegralSegment )
   {
      return nullptr;
   }

   // If there is a closure at this pier, then this pier is in the middle of a group
   // so the group on the ahead and back side of this pier are the same. There can't
   // be a closure here if this is an end pier
   CGirderGroupData* pGroup = GetGirderGroup(pgsTypes::Ahead); // get on ahead side... could be back side
   if ( pGroup == nullptr )
   {
      return nullptr;
   }

   CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      // NOTE: nSegments-1 because there is one less closure than segments
      // no need to check the right end of the last segment as there isn't a closure there)
      CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
      if ( pClosure->GetPier() == this )
      {
         return pClosure;
      }
   }

   return nullptr;
}

const CClosureJointData* CPierData2::GetClosureJoint(GirderIndexType gdrIdx) const
{
   if ( IsBoundaryPier() )
   {
      ATLASSERT(false); // why are you asking for a closure joint at a boundary pier? it doesn't have one
      return nullptr;
   }

   if ( m_SegmentConnectionType == pgsTypes::psctContinuousSegment || m_SegmentConnectionType == pgsTypes::psctIntegralSegment )
   {
      return nullptr;
   }

   // If there is a closure at this pier, then this pier is in the middle of a group
   // so the group on the ahead and back side of this pier are the same. There can't
   // be a closure here if this is an end pier
   const CGirderGroupData* pGroup = GetGirderGroup(pgsTypes::Ahead); // get on ahead side... could be back side
   if ( pGroup == nullptr )
   {
      return nullptr;
   }

   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      // NOTE: nSegments-1 because there is one less closure than segments
      // no need to check the right end of the last segment as there isn't a closure there)
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
      if ( pClosure->GetPier() == this )
      {
         return pClosure;
      }
   }

   return nullptr;
}

bool CPierData2::HasSlabOffset() const
{
   // interior piers with segments that are continuous over them do not have slab offsets
   return (IsInteriorPier() && ::IsSegmentContinuousOverPier(m_SegmentConnectionType) ? false : true);
}

void CPierData2::SetSlabOffset(pgsTypes::PierFaceType face, Float64 slabOffset)
{
   m_SlabOffset[face] = slabOffset;
}

void CPierData2::SetSlabOffset(Float64 back, Float64 ahead)
{
   m_SlabOffset[pgsTypes::Back] = back;
   m_SlabOffset[pgsTypes::Ahead] = ahead;
}

Float64 CPierData2::GetSlabOffset(pgsTypes::PierFaceType face, bool bRawData) const
{
   // if this assert fires, there isn't a slab offset at this pier
   ATLASSERT( bRawData || HasSlabOffset() );

   if (bRawData || m_pBridgeDesc == nullptr || m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBearingLine)
   {
      return m_SlabOffset[face];
   }
   else if(m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
   {
      return m_pBridgeDesc->GetSlabOffset();
   }
   else
   {
      ATLASSERT(false); // should never get here
      // if slab offset type is sotSegment, we don't know which girder
      return m_SlabOffset[face];
   }
}

void CPierData2::GetSlabOffset(Float64* pBack, Float64* pAhead, bool bRawData) const
{
   *pBack = GetSlabOffset(pgsTypes::Back, bRawData);
   *pAhead = GetSlabOffset(pgsTypes::Ahead, bRawData);
}

pgsTypes::PierModelType CPierData2::GetPierModelType() const
{
   return m_PierModelType;
}

void CPierData2::SetPierModelType(pgsTypes::PierModelType modelType)
{
   m_PierModelType = modelType;
}

void CPierData2::SetConcrete(const CConcreteMaterial& concrete)
{
   m_Concrete = concrete;
}

CConcreteMaterial& CPierData2::GetConcrete()
{
   return m_Concrete;
}

const CConcreteMaterial& CPierData2::GetConcrete() const
{
   return m_Concrete;
}

void CPierData2::SetTransverseOffset(ColumnIndexType refColumnIdx,Float64 offset,pgsTypes::OffsetMeasurementType offsetType)
{
   m_RefColumnIdx = refColumnIdx;
   m_TransverseOffset = offset;
   m_TransverseOffsetMeasurement = offsetType;
}

void CPierData2::GetTransverseOffset(ColumnIndexType* pRefColumnIdx,Float64* pOffset,pgsTypes::OffsetMeasurementType* pOffsetType) const
{
   *pRefColumnIdx = m_RefColumnIdx;
   *pOffset = m_TransverseOffset;
   *pOffsetType = m_TransverseOffsetMeasurement;
}

void CPierData2::SetXBeamDimensions(pgsTypes::SideType side,Float64 height,Float64 taperHeight,Float64 taperLength,Float64 endSlopeOffset)
{
   m_XBeamHeight[side]         = height;
   m_XBeamTaperHeight[side]    = taperHeight;
   m_XBeamTaperLength[side]    = taperLength;
   m_XBeamEndSlopeOffset[side] = endSlopeOffset;
}

void CPierData2::GetXBeamDimensions(pgsTypes::SideType side,Float64* pHeight,Float64* pTaperHeight,Float64* pTaperLength,Float64* pEndSlopeOffset) const
{
   *pHeight         = m_XBeamHeight[side];
   *pTaperHeight    = m_XBeamTaperHeight[side];
   *pTaperLength    = m_XBeamTaperLength[side];
   *pEndSlopeOffset = m_XBeamEndSlopeOffset[side];
}

void CPierData2::SetXBeamWidth(Float64 width)
{
   m_XBeamWidth = width;
}

Float64 CPierData2::GetXBeamWidth() const
{
   return m_XBeamWidth;
}

void CPierData2::SetXBeamOverhang(pgsTypes::SideType side,Float64 overhang)
{
   m_XBeamOverhang[side] = overhang;
}

void CPierData2::SetXBeamOverhangs(Float64 leftOverhang,Float64 rightOverhang)
{
   m_XBeamOverhang[pgsTypes::stLeft]  = leftOverhang;
   m_XBeamOverhang[pgsTypes::stRight] = rightOverhang;
}

Float64 CPierData2::GetXBeamOverhang(pgsTypes::SideType side) const
{
   return m_XBeamOverhang[side];
}

void CPierData2::GetXBeamOverhangs(Float64* pLeftOverhang,Float64* pRightOverhang) const
{
   *pLeftOverhang  = m_XBeamOverhang[pgsTypes::stLeft];
   *pRightOverhang = m_XBeamOverhang[pgsTypes::stRight];
}

Float64 CPierData2::GetXBeamLength() const
{
   Float64 L = 0;
   for (const auto& s : m_ColumnSpacing)
   {
      L += s;
   }
   L += (m_XBeamOverhang[pgsTypes::stLeft]-m_XBeamEndSlopeOffset[pgsTypes::stLeft]) + (m_XBeamOverhang[pgsTypes::stRight]-m_XBeamEndSlopeOffset[pgsTypes::stRight]);
   return L;
}

void CPierData2::SetColumnFixity(pgsTypes::ColumnLongitudinalBaseFixityType fixityType)
{
   m_ColumnFixity = fixityType;
}

pgsTypes::ColumnLongitudinalBaseFixityType CPierData2::GetColumnFixity() const
{
   return m_ColumnFixity;
}

void CPierData2::RemoveColumns()
{
   // erase all but the first column... must always have one column
   m_Columns.erase(m_Columns.begin()+1,m_Columns.end());
   m_ColumnSpacing.clear();
   ATLASSERT(m_Columns.size() == m_ColumnSpacing.size()+1);
}

void CPierData2::AddColumn(Float64 spacing,const CColumnData& columnData)
{
   m_ColumnSpacing.push_back(spacing);
   m_Columns.push_back(columnData);
   m_Columns.back().SetPier(this);
   ATLASSERT(m_Columns.size() == m_ColumnSpacing.size()+1);
}

void CPierData2::SetColumnSpacing(SpacingIndexType spaceIdx,Float64 spacing)
{
   ATLASSERT(spaceIdx < m_ColumnSpacing.size());
   m_ColumnSpacing[spaceIdx] = spacing;
}

void CPierData2::SetColumnData(ColumnIndexType colIdx,const CColumnData& columnData)
{
   ATLASSERT(colIdx < m_Columns.size());
   m_Columns[colIdx] = columnData;
   m_Columns[colIdx].SetPier(this);
}

void CPierData2::SetColumnCount(ColumnIndexType nColumns)
{
   if ( nColumns < m_Columns.size() )
   {
      // the number of columns is being reduced ... remove columns on the right side of the pier
      m_Columns.erase(m_Columns.begin()+nColumns,m_Columns.end());
      m_ColumnSpacing.erase(m_ColumnSpacing.begin()+nColumns-1,m_ColumnSpacing.end());
   }
   else if ( m_Columns.size() < nColumns )
   {
      // the number of columns is being increased... add columns on the right side of the pier
      ColumnIndexType nColumnsToAdd = nColumns - m_Columns.size();
      CColumnData column = m_Columns.back(); // right-most column
      Float64 spacing = (m_ColumnSpacing.size() == 0 ? WBFL::Units::ConvertToSysUnits(10.0,WBFL::Units::Measure::Feet) : m_ColumnSpacing.back()); // right-most spacing
      m_Columns.insert(m_Columns.end(),nColumnsToAdd,column);
      m_ColumnSpacing.insert(m_ColumnSpacing.end(),nColumnsToAdd,spacing);
   }
   ATLASSERT(m_Columns.size() == m_ColumnSpacing.size()+1);
}

ColumnIndexType CPierData2::GetColumnCount() const
{
   ATLASSERT(m_Columns.size() == m_ColumnSpacing.size()+1);
   return m_Columns.size();
}

Float64 CPierData2::GetColumnSpacing(SpacingIndexType spaceIdx) const
{
   ATLASSERT(spaceIdx < m_ColumnSpacing.size());
   return m_ColumnSpacing[spaceIdx];
}

Float64 CPierData2::GetColumnSpacingWidth() const
{
   Float64 w = 0;
   for (const auto& s : m_ColumnSpacing)
   {
      w += s;
   }
   return w;
}

Float64 CPierData2::GetColumnSpacingWidthToColumn(ColumnIndexType colIdx) const
{
   Float64 w = 0;
   for ( ColumnIndexType idx = 0; idx < colIdx; idx++ )
   {
      w += m_ColumnSpacing[idx];
   }
   return w;
}

const CColumnData& CPierData2::GetColumnData(ColumnIndexType colIdx) const
{
   ATLASSERT(colIdx < m_Columns.size());
   return m_Columns[colIdx];
}

void CPierData2::SetDiaphragmHeight(pgsTypes::PierFaceType pierFace,Float64 d)
{
   ATLASSERT(IsEqual(d, -1.0) || 0 <= d); // -1 means compute, otherwise must be greater than zone
   m_DiaphragmHeight[pierFace] = d;
}

Float64 CPierData2::GetDiaphragmHeight(pgsTypes::PierFaceType pierFace) const
{
   return m_DiaphragmHeight[pierFace];
}

void CPierData2::SetDiaphragmWidth(pgsTypes::PierFaceType pierFace,Float64 w)
{
   ATLASSERT(IsEqual(w, -1.0) || 0 <= w); // -1 means compute, otherwise must be greater than zone
   m_DiaphragmWidth[pierFace] = w;
}

Float64 CPierData2::GetDiaphragmWidth(pgsTypes::PierFaceType pierFace)const
{
   return m_DiaphragmWidth[pierFace];
}

ConnectionLibraryEntry::DiaphragmLoadType CPierData2::GetDiaphragmLoadType(pgsTypes::PierFaceType pierFace) const
{
   return m_DiaphragmLoadType[pierFace];
}

void CPierData2::SetDiaphragmLoadType(pgsTypes::PierFaceType pierFace,ConnectionLibraryEntry::DiaphragmLoadType type)
{
   m_DiaphragmLoadType[pierFace] = type;
   m_DiaphragmLoadLocation[pierFace] = 0.0;
}

Float64 CPierData2::GetDiaphragmLoadLocation(pgsTypes::PierFaceType pierFace) const
{
   return m_DiaphragmLoadLocation[pierFace];
}

void CPierData2::SetDiaphragmLoadLocation(pgsTypes::PierFaceType pierFace,Float64 loc)
{
   m_DiaphragmLoadLocation[pierFace] = loc;
}

Float64 CPierData2::GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gM[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CPierData2::SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls, Float64 gM)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gM[ls == pgsTypes::FatigueI ? 1 : 0] = gM;
}

void CPierData2::SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls, Float64 gM)
{
   GirderIndexType ngdrs = GetLldfGirderCount();
   if (ngdrs>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<ngdrs-1; ig++)
      {
         SetLLDFNegMoment(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFNegMoment(0,ls,gM);
      SetLLDFNegMoment(ngdrs-1,ls,gM);
   }
}

bool CPierData2::IsContinuousConnection() const
{
   if ( IsInteriorPier() )
   {
      return true; // connection is always continuous/integral at interior piers
   }
   else
   {
      ATLASSERT(IsBoundaryPier());
      if ( m_BoundaryConditionType == pgsTypes::bctContinuousAfterDeck ||
           m_BoundaryConditionType == pgsTypes::bctContinuousBeforeDeck ||
           m_BoundaryConditionType == pgsTypes::bctIntegralAfterDeck ||
           m_BoundaryConditionType == pgsTypes::bctIntegralBeforeDeck )
      {
         return true;
      }
      else
      {
         return false;
      }
   }
}

bool CPierData2::IsContinuous() const
{
   if ( IsInteriorPier() )
   {
      return (m_SegmentConnectionType == pgsTypes::psctContinousClosureJoint || m_SegmentConnectionType == pgsTypes::psctContinuousSegment);
   }
   else
   {
      return (m_BoundaryConditionType == pgsTypes::bctContinuousBeforeDeck || m_BoundaryConditionType == pgsTypes::bctContinuousAfterDeck);
   }
}

void CPierData2::IsIntegral(bool* pbLeft,bool* pbRight) const
{
   if ( IsInteriorPier() )
   {
      if (m_SegmentConnectionType == pgsTypes::psctIntegralClosureJoint || m_SegmentConnectionType == pgsTypes::psctIntegralSegment)
      {
         *pbLeft  = true;
         *pbRight = true;
      }
      else
      {
         *pbLeft  = false;
         *pbRight = false;
      }
   }
   else
   {
      if (m_BoundaryConditionType == pgsTypes::bctIntegralBeforeDeck || m_BoundaryConditionType == pgsTypes::bctIntegralAfterDeck)
      {
         *pbLeft  = true;
         *pbRight = true;
      }
      else
      {
         *pbLeft  = m_BoundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeAhead || m_BoundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeAhead;
         *pbRight = m_BoundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeBack  || m_BoundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeBack;
      }
   }
}

bool CPierData2::IsAbutment() const
{
   return (m_pPrevSpan == nullptr || m_pNextSpan == nullptr) ? true : false;
}

bool CPierData2::IsPier() const
{
   return !IsAbutment();
}

bool CPierData2::IsInteriorPier() const
{
   // If the girder group on both sides of the pier is the same, then this pier
   // is interior to the group.
   const CGirderGroupData* pPrevGroup = GetPrevGirderGroup();
   const CGirderGroupData* pNextGroup = GetNextGirderGroup();
   if ( pPrevGroup == nullptr && pNextGroup == nullptr )
   {
      return false;
   }

   return (pPrevGroup == pNextGroup ? true : false);
}

bool CPierData2::IsBoundaryPier() const
{
   return !IsInteriorPier();
}

bool CPierData2::HasSpacing() const
{
   // use this version for all cases except when loading the file

   // there is spacing data at a pier if it
   if ( ::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
   {
      return false; // spacing data is at the bridge level
   }
   else
   {
      // spacing data is at the ends of segments, which is at the piers and temporary supports
      // there is spacing data if this is a boundary pier or if this is an interior pier and there is a closure joint
      return ( IsBoundaryPier() || (IsInteriorPier() && GetClosureJoint(0) != nullptr) ) ? true : false;
   }
}

bool CPierData2::HasSpacing(VARIANT_BOOL vbIsBoundaryPier) const
{
   // use this version when loading the file

   if ( ::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
   {
      return false; // spacing data is at the bridge level
   }
   else
   {
      return ( vbIsBoundaryPier == VARIANT_TRUE || 
                  (m_SegmentConnectionType == pgsTypes::psctContinousClosureJoint || m_SegmentConnectionType == pgsTypes::psctIntegralClosureJoint)
         ) ? true : false;
   }
}

CPierData2::LLDF& CPierData2::GetLLDF(GirderIndexType igs) const
{
   // First: Compare size of our collection with current number of girders and resize if they don't match
   GirderIndexType nGirders = GetLldfGirderCount();
   ATLASSERT(0 < nGirders);

   GirderIndexType nLLDF = m_LLDFs.size();

   if (m_bDistributionFactorsFromOlderVersion)
   {
      // data loaded from older versions should be loaded into first two entries
      if(nLLDF == 2)
      {
         LLDF exterior = m_LLDFs[0];
         LLDF interior = m_LLDFs[1];
         for (GirderIndexType gdrIdx = 2; gdrIdx < nGirders; gdrIdx++)
         {
            if (gdrIdx != nGirders-1)
            {
               m_LLDFs.push_back(interior);
            }
            else
            {
               m_LLDFs.push_back(exterior);
            }
         }

         m_bDistributionFactorsFromOlderVersion = false;
         nLLDF = nGirders;
      }
      else if ( nLLDF == 1 )
      {
         // single girder bridge
         m_bDistributionFactorsFromOlderVersion = false;
      }
      else
      {
         ATLASSERT(false); // something when wrong
      }
   }

   if (nLLDF == 0)
   {
      m_LLDFs.resize(nGirders);
   }
   else if (nLLDF < nGirders)
   {
      // More girders than factors - move exterior to last girder and use last interior for new interiors
      LLDF exterior = m_LLDFs.back();
      GirderIndexType inter_idx = (nGirders == 1 ? 0 : nGirders-2); // one-girder bridges could otherwise give us trouble
      LLDF interior = m_LLDFs[inter_idx];

      m_LLDFs[nLLDF-1] = interior;
      for (IndexType i = nLLDF; i < nGirders; i++)
      {
         if (i != nGirders-1)
         {
            m_LLDFs.push_back(interior);
         }
         else
         {
            m_LLDFs.push_back(exterior);
         }
      }
    }
   else if (nGirders < nLLDF)
   {
      // more factors than girders - truncate, then move last exterior to end
      LLDF exterior = m_LLDFs.back();
      m_LLDFs.resize(nGirders);
      m_LLDFs.back() = exterior;
   }

   // Next: let's deal with retrieval
   if (igs < 0)
   {
      ATLASSERT(false); // problemo in calling routine - let's not crash
      return m_LLDFs.front();
   }
   else if (nGirders <= igs)
   {
      ATLASSERT(false); // problemo in calling routine - let's not crash
      return m_LLDFs.back();
   }
   else
   {
      return m_LLDFs[igs];
   }
}

GirderIndexType CPierData2::GetLldfGirderCount() const
{
   GirderIndexType nGirdersAhead(0), nGirdersBack(0);

   const CSpanData2* pAhead = GetSpan(pgsTypes::Ahead);
   if ( pAhead )
   {
      const CGirderGroupData* pGroup = pAhead->GetBridgeDescription()->GetGirderGroup(pAhead);
      nGirdersAhead = pGroup->GetGirderCount();
   }

   const CSpanData2* pBack = GetSpan(pgsTypes::Back);
   if ( pBack )
   {
      const CGirderGroupData* pGroup = pBack->GetBridgeDescription()->GetGirderGroup(pBack);
      nGirdersBack = pGroup->GetGirderCount();
   }

   if (pBack == nullptr && pAhead == nullptr)
   {
      ATLASSERT(false); // function called before bridge tied together - no good
      return 0;
   }
   else
   {
      return Max(nGirdersAhead, nGirdersBack);
   }
}

HRESULT CPierData2::LoadOldPierData(Float64 version,IStructuredLoad* pStrLoad,IProgress* pProgress,const std::_tstring& strUnitName)
{
   // Input is in an old format (the format is PGSuper before version 3.0 when we added PGSplice)
   // Use the old pier data object to load the data

   HRESULT hr = pStrLoad->BeginUnit(_T("PierData")); 

   CPierData pd;
   HRESULT hr2 = pd.Load(version,pStrLoad,pProgress,strUnitName);
   if ( FAILED(hr2))
   {
      return hr2;
   }

   if ( SUCCEEDED(hr) )
   {
      pStrLoad->EndUnit(); // PierData
   }

   // The data was loaded correctly, now convert the old data into the proper format for this class
   SetPierData(&pd);

   return S_OK;
}

void CPierData2::SetPierData(CPierData* pPier)
{
   ATLASSERT(pPier->GetPierIndex() == m_PierIdx );

   SetOrientation(pPier->m_strOrientation.c_str());
   SetStation(pPier->m_Station);
   SetBoundaryConditionType(pPier->m_ConnectionType);
   //SetSegmentConnectionType(pPier->m_SegmentConnectionType);

   SetGirderEndDistance(pgsTypes::Back, pPier->m_GirderEndDistance[pgsTypes::Back], pPier->m_EndDistanceMeasurementType[pgsTypes::Back]);
   SetGirderEndDistance(pgsTypes::Ahead,pPier->m_GirderEndDistance[pgsTypes::Ahead],pPier->m_EndDistanceMeasurementType[pgsTypes::Ahead]);
   
   SetBearingOffset(pgsTypes::Back, pPier->m_GirderBearingOffset[pgsTypes::Back], pPier->m_BearingOffsetMeasurementType[pgsTypes::Back]);
   SetBearingOffset(pgsTypes::Ahead,pPier->m_GirderBearingOffset[pgsTypes::Ahead],pPier->m_BearingOffsetMeasurementType[pgsTypes::Ahead]);

   m_BearingData[pgsTypes::Back].clear();
   m_BearingData[pgsTypes::Ahead].clear();

   // older data only had support width (which is now bearing length)
   ATLASSERT(m_pBridgeDesc==nullptr || m_pBridgeDesc->GetBearingType()==pgsTypes::brtPier);
   CBearingData2 bd;
   bd.Length = pPier->m_SupportWidth[pgsTypes::Back];
   m_BearingData[pgsTypes::Back].push_back(bd);
   bd.Length = pPier->m_SupportWidth[pgsTypes::Ahead];
   m_BearingData[pgsTypes::Ahead].push_back(bd);

   SetDiaphragmHeight(pgsTypes::Back,pPier->m_DiaphragmHeight[pgsTypes::Back]);
   SetDiaphragmWidth(pgsTypes::Back,pPier->m_DiaphragmWidth[pgsTypes::Back]);
   SetDiaphragmLoadType(pgsTypes::Back,pPier->m_DiaphragmLoadType[pgsTypes::Back]);
   SetDiaphragmLoadLocation(pgsTypes::Back,pPier->m_DiaphragmLoadLocation[pgsTypes::Back]);

   SetDiaphragmHeight(pgsTypes::Ahead,pPier->m_DiaphragmHeight[pgsTypes::Ahead]);
   SetDiaphragmWidth(pgsTypes::Ahead,pPier->m_DiaphragmWidth[pgsTypes::Ahead]);
   SetDiaphragmLoadType(pgsTypes::Ahead,pPier->m_DiaphragmLoadType[pgsTypes::Ahead]);
   SetDiaphragmLoadLocation(pgsTypes::Ahead,pPier->m_DiaphragmLoadLocation[pgsTypes::Ahead]);

   std::vector<CPierData::LLDF>::iterator iter(pPier->m_LLDFs.begin());
   std::vector<CPierData::LLDF>::iterator iterEnd(pPier->m_LLDFs.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CPierData::LLDF lldf = *iter;
      CPierData2::LLDF lldf2;
      lldf2.gM[0] = lldf.gM[0];
      lldf2.gM[1] = lldf.gM[1];

      m_LLDFs.push_back(lldf2);
   }

   m_bDistributionFactorsFromOlderVersion = pPier->m_DistributionFactorsFromOlderVersion;
}

void CPierData2::ValidateBoundaryConditionType()
{
   // make sure the connection type is valid
   std::vector<pgsTypes::BoundaryConditionType> vConnectionTypes = m_pBridgeDesc->GetBoundaryConditionTypes(m_PierIdx);
   std::vector<pgsTypes::BoundaryConditionType>::iterator found = std::find(vConnectionTypes.begin(),vConnectionTypes.end(),m_BoundaryConditionType);
   if ( found == vConnectionTypes.end() )
   {
      // the current connection type isn't valid... updated it
      m_BoundaryConditionType = vConnectionTypes.front();
   }
}

#if defined _DEBUG
void CPierData2::AssertValid() const
{
   // Girder spacing is either attached to this pier or nothing
   // can't be attached to temporary support
   ATLASSERT(m_GirderSpacing[pgsTypes::Back].GetTemporarySupport()  == nullptr);
   ATLASSERT(m_GirderSpacing[pgsTypes::Ahead].GetTemporarySupport() == nullptr);

   // Spacing owned by this pier must reference this pier
   ATLASSERT(m_GirderSpacing[pgsTypes::Back].GetPier()  == this);
   ATLASSERT(m_GirderSpacing[pgsTypes::Ahead].GetPier() == this);

   // must also be attached to the same bridge
   ATLASSERT(m_GirderSpacing[pgsTypes::Back].GetPier()->GetBridgeDescription()  == m_pBridgeDesc);
   ATLASSERT(m_GirderSpacing[pgsTypes::Ahead].GetPier()->GetBridgeDescription() == m_pBridgeDesc);

   // check pointers
   if ( m_pBridgeDesc )
   {
      if ( m_pPrevSpan )
      {
         _ASSERT(m_pPrevSpan->GetNextPier() == this);
      }

      if ( m_pNextSpan )
      {
         _ASSERT(m_pNextSpan->GetPrevPier() == this );
      }
   }
   else
   {
      _ASSERT(m_pPrevSpan == nullptr);
      _ASSERT(m_pNextSpan == nullptr);
   }
}
#endif
