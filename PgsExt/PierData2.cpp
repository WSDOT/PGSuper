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
#include <PgsExt\PierData2.h>
#include <PgsExt\SpanData2.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderSpacing2.h>
#include <PgsExt\ClosureJointData.h>

#include <PierData.h>

#include <Units\SysUnits.h>
#include <StdIo.h>
#include <StrData.cpp>
#include <WBFLCogo.h>

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPierData2
****************************************************************************/

CPierData2::CPierData2()
{
   m_PierIdx = INVALID_INDEX;
   m_PierID  = INVALID_INDEX;

   m_pPrevSpan = NULL;
   m_pNextSpan = NULL;

   m_pBridgeDesc = NULL;

   m_Station     = 0.0;
   Orientation = Normal;
   Angle       = 0.0;

   m_bHasCantilever = false;
   m_CantileverLength = 0.0;

   m_PierConnectionType = pgsTypes::Hinge;
   m_SegmentConnectionType = pgsTypes::psctContinuousSegment;

   m_strOrientation = _T("Normal");

   for ( int i = 0; i < 2; i++ )
   {
      m_GirderEndDistance[i]            = ::ConvertToSysUnits(6.0,unitMeasure::Inch);
      m_EndDistanceMeasurementType[i]   = ConnectionLibraryEntry::FromBearingNormalToPier;

      m_GirderBearingOffset[i]          = ::ConvertToSysUnits(1.0,unitMeasure::Feet);
      m_BearingOffsetMeasurementType[i] = ConnectionLibraryEntry::NormalToPier;

      m_SupportWidth[i]                 = ::ConvertToSysUnits(1.0,unitMeasure::Feet);

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
   m_pPrevSpan = NULL;
   m_pNextSpan = NULL;

   m_pBridgeDesc = NULL;

   m_Station   = 0.0;
   Orientation = Normal;
   Angle       = 0.0;
   
   m_strOrientation = _T("Normal");

   m_GirderSpacing[pgsTypes::Back].SetPier(this);
   m_GirderSpacing[pgsTypes::Ahead].SetPier(this);

   MakeCopy(rOther,true /*copy data only*/);
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

   if ( m_PierConnectionType != rOther.m_PierConnectionType )
   {
      return false;
   }

   if ( IsInteriorPier() )
   {
      if ( m_SegmentConnectionType != rOther.m_SegmentConnectionType )
      {
         return false;
      }
   }

   if ( IsAbutment() )
   {
      if ( m_bHasCantilever != rOther.m_bHasCantilever )
      {
         return false;
      }

      if ( m_bHasCantilever && !IsEqual(m_CantileverLength,rOther.m_CantileverLength) )
      {
         return false;
      }
   }

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;

      if ( !m_bHasCantilever )
      {
         // not used if pier has a cantilever
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
      }

      if ( !IsEqual(m_SupportWidth[face], rOther.m_SupportWidth[face]) )
      {
         return false;
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

LPCTSTR CPierData2::AsString(pgsTypes::PierConnectionType type)
{
   switch(type)
   { 
   case pgsTypes::Hinge:
      return _T("Hinge");

   case pgsTypes::Roller:
      return _T("Roller");

   case pgsTypes::ContinuousAfterDeck:
      return _T("Continuous after deck placement");

   case pgsTypes::ContinuousBeforeDeck:
      return _T("Continuous before deck placement");

   case pgsTypes::IntegralAfterDeck:
      return _T("Integral after deck placement");

   case pgsTypes::IntegralBeforeDeck:
      return _T("Integral before deck placement");

   case pgsTypes::IntegralAfterDeckHingeBack:
      return _T("Hinged on back side; Integral on ahead side after deck placement");

   case pgsTypes::IntegralBeforeDeckHingeBack:
      return _T("Hinged on back side; Integral on ahead side before deck placement");

   case pgsTypes::IntegralAfterDeckHingeAhead:
      return _T("Integral on back side after deck placement; Hinged on ahead side");

   case pgsTypes::IntegralBeforeDeckHingeAhead:
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
         m_PierConnectionType = (pgsTypes::PierConnectionType)var.lVal;

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
            m_PierConnectionType = (pgsTypes::PierConnectionType)var.lVal;
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
         m_PierConnectionType = (pgsTypes::PierConnectionType)var.lVal;

         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("SegmentConnectionType"),&var);
         m_SegmentConnectionType = (pgsTypes::PierSegmentConnectionType)var.lVal;
      }

      if ( 12 < version )
      {
         // added in version 13
         var.vt = VT_R8;
         if ( SUCCEEDED(pStrLoad->get_Property(_T("CantileverLength"),&var)) )
         {
            m_bHasCantilever = true;
            m_CantileverLength = var.dblVal;
         }
      }
      
      hr = pStrLoad->BeginUnit(_T("Back"));

      Float64 back_version;
      pStrLoad->get_Version(&back_version);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderEndDistance"),&var);
      m_GirderEndDistance[pgsTypes::Back] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("EndDistanceMeasurementType"),&var);
      m_EndDistanceMeasurementType[pgsTypes::Back] = ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderBearingOffset"),&var);
      m_GirderBearingOffset[pgsTypes::Back] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BearingOffsetMeasurementType"),&var);
      m_BearingOffsetMeasurementType[pgsTypes::Back] = ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SupportWidth"),&var);
      m_SupportWidth[pgsTypes::Back] = var.dblVal;

      if ( back_version < 2 )
      {
         if ( !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
         {
            hr = m_GirderSpacing[pgsTypes::Back].Load(pStrLoad,pProgress);
         }
      }
      else
      {
         if ( vbIsBoundaryPier == VARIANT_TRUE && !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
         {
            hr = m_GirderSpacing[pgsTypes::Back].Load(pStrLoad,pProgress);
         }
      }

      {
         hr = pStrLoad->BeginUnit(_T("Diaphragm"));
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("DiaphragmWidth"),&var);
         m_DiaphragmWidth[pgsTypes::Back] = var.dblVal;

         hr = pStrLoad->get_Property(_T("DiaphragmHeight"),&var);
         m_DiaphragmHeight[pgsTypes::Back] = var.dblVal;

         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("DiaphragmLoadType"),&var);

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

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderEndDistance"),&var);
      m_GirderEndDistance[pgsTypes::Ahead] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("EndDistanceMeasurementType"),&var);
      m_EndDistanceMeasurementType[pgsTypes::Ahead] = ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GirderBearingOffset"),&var);
      m_GirderBearingOffset[pgsTypes::Ahead] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("BearingOffsetMeasurementType"),&var);
      m_BearingOffsetMeasurementType[pgsTypes::Ahead] = ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(OLE2T(var.bstrVal));

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SupportWidth"),&var);
      m_SupportWidth[pgsTypes::Ahead] = var.dblVal;

      if ( ahead_version < 2 )
      {
         if ( !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
         {
            hr = m_GirderSpacing[pgsTypes::Ahead].Load(pStrLoad,pProgress);
         }
      }
      else
      {
         if ( vbIsBoundaryPier == VARIANT_TRUE && !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
         {
            hr = m_GirderSpacing[pgsTypes::Ahead].Load(pStrLoad,pProgress);
         }
      }


      {
         hr = pStrLoad->BeginUnit(_T("Diaphragm"));
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("DiaphragmWidth"),&var);
         m_DiaphragmWidth[pgsTypes::Ahead] = var.dblVal;

         hr = pStrLoad->get_Property(_T("DiaphragmHeight"),&var);
         m_DiaphragmHeight[pgsTypes::Ahead] = var.dblVal;

         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("DiaphragmLoadType"),&var);

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
            Float64 gR[2][2];

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

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Interior"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gR[pgsTypes::Interior][0] = var.dblVal;
               gR[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gR[pgsTypes::Exterior][0] = var.dblVal;
               gR[pgsTypes::Exterior][1] = var.dblVal;
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

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Interior_Strength"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gR[pgsTypes::Interior][0] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior_Strength"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gR[pgsTypes::Exterior][0] = var.dblVal;

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
               if ( FAILED(pStrLoad->get_Property(_T("gR_Interior_Fatigue"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gR[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior_Fatigue"),&var)) )
               {
                  return STRLOAD_E_INVALIDFORMAT;
               }

               gR[pgsTypes::Exterior][1] = var.dblVal;
            }

            // Move interior and exterior factors into first two slots in df vector. We will 
            // need to move them into all girder slots once this object is fully connected to the bridge
            m_bDistributionFactorsFromOlderVersion = true;

            LLDF df;
            df.gM[0] = gM[pgsTypes::Exterior][0];
            df.gM[1] = gM[pgsTypes::Exterior][1];
            df.gR[0] = gR[pgsTypes::Exterior][0];
            df.gR[1] = gR[pgsTypes::Exterior][1];

            m_LLDFs.push_back(df); // First in list is exterior

            df.gM[0] = gM[pgsTypes::Interior][0];
            df.gM[1] = gM[pgsTypes::Interior][1];
            df.gR[0] = gR[pgsTypes::Interior][0];
            df.gR[1] = gR[pgsTypes::Interior][1];

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

               hr = pStrLoad->get_Property(_T("gR_Strength"),&var);
               lldf.gR[0] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gM_Fatigue"),&var);
               lldf.gM[1] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gR_Fatigue"),&var);
               lldf.gR[1] = var.dblVal;

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

   ASSERT_VALID;

   return S_OK;
}

HRESULT CPierData2::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PierDataDetails"),13.0);
   
   pStrSave->put_Property(_T("ID"),CComVariant(m_PierID));

   pStrSave->put_Property(_T("Station"),         CComVariant(m_Station) );
   pStrSave->put_Property(_T("Orientation"),     CComVariant( CComBSTR(m_strOrientation.c_str()) ) );
   pStrSave->put_Property(_T("IsBoundaryPier"),  CComVariant( IsBoundaryPier() ? VARIANT_TRUE : VARIANT_FALSE) ); // added in version 11

   pStrSave->put_Property(_T("PierConnectionType"),  CComVariant( m_PierConnectionType ) ); // changed from left and right to a single value in version 7
   if ( IsInteriorPier() )
   {
      pStrSave->put_Property(_T("SegmentConnectionType"),  CComVariant( m_SegmentConnectionType ) );
   }

   // added in version 13
   if ( m_bHasCantilever )
   {
      pStrSave->put_Property(_T("CantileverLength"),CComVariant(m_CantileverLength));
   }

   pStrSave->BeginUnit(_T("Back"),2.0);
   pStrSave->put_Property(_T("GirderEndDistance"),CComVariant( m_GirderEndDistance[pgsTypes::Back] ) );
   pStrSave->put_Property(_T("EndDistanceMeasurementType"), CComVariant(ConnectionLibraryEntry::StringForEndDistanceMeasurementType(m_EndDistanceMeasurementType[pgsTypes::Back]).c_str()) );
   pStrSave->put_Property(_T("GirderBearingOffset"),CComVariant(m_GirderBearingOffset[pgsTypes::Back]));
   pStrSave->put_Property(_T("BearingOffsetMeasurementType"),CComVariant(ConnectionLibraryEntry::StringForBearingOffsetMeasurementType(m_BearingOffsetMeasurementType[pgsTypes::Back]).c_str()) );
   pStrSave->put_Property(_T("SupportWidth"),CComVariant(m_SupportWidth[pgsTypes::Back]));

   // added IsBoundaryPier() requirement in version 2
   if ( IsBoundaryPier() && !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
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
   pStrSave->put_Property(_T("SupportWidth"),CComVariant(m_SupportWidth[pgsTypes::Ahead]));

   // added IsBoundaryPier() requirement in version 2
   if ( IsBoundaryPier() && !::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
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
      pStrSave->BeginUnit(_T("LLDF"),3.0); // Version 3 went from interior/exterior to girder by girder

      GirderIndexType ngs = GetLldfGirderCount();
      pStrSave->put_Property(_T("nLLDFGirders"),CComVariant(ngs));

      for (GirderIndexType igs=0; igs<ngs; igs++)
      {
         pStrSave->BeginUnit(_T("LLDF_Girder"),1.0);
         LLDF& lldf = GetLLDF(igs);

         pStrSave->put_Property(_T("gM_Strength"), CComVariant(lldf.gM[0]));
         pStrSave->put_Property(_T("gR_Strength"), CComVariant(lldf.gR[0]));
         pStrSave->put_Property(_T("gM_Fatigue"),  CComVariant(lldf.gM[1]));
         pStrSave->put_Property(_T("gR_Fatigue"),  CComVariant(lldf.gR[1]));
         pStrSave->EndUnit(); // LLDF_Girder
      }

      pStrSave->EndUnit(); // LLDF
   }

   pStrSave->EndUnit();

   return hr;
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

   m_bHasCantilever = rOther.m_bHasCantilever;
   m_CantileverLength = rOther.m_CantileverLength;

   for ( int i = 0; i < 2; i++ )
   {
      m_GirderEndDistance[i]            = rOther.m_GirderEndDistance[i];
      m_EndDistanceMeasurementType[i]   = rOther.m_EndDistanceMeasurementType[i];
      m_GirderBearingOffset[i]          = rOther.m_GirderBearingOffset[i];
      m_BearingOffsetMeasurementType[i] = rOther.m_BearingOffsetMeasurementType[i];
      m_SupportWidth[i]                 = rOther.m_SupportWidth[i];

      m_GirderSpacing[i] = rOther.m_GirderSpacing[i];
      m_GirderSpacing[i].SetPier(this);

      m_DiaphragmHeight[i]       = rOther.m_DiaphragmHeight[i];       
      m_DiaphragmWidth[i]        = rOther.m_DiaphragmWidth[i];
      m_DiaphragmLoadType[i]     = rOther.m_DiaphragmLoadType[i];
      m_DiaphragmLoadLocation[i] = rOther.m_DiaphragmLoadLocation[i];
   }

   m_LLDFs = rOther.m_LLDFs;
   m_bDistributionFactorsFromOlderVersion = rOther.m_bDistributionFactorsFromOlderVersion;
   
   if ( m_pBridgeDesc )
   {
      // If this pier is part of a bridge, use the SetXXXConnectionType method so
      // girder segments are split/joined as necessary for the new connection types
      if ( IsBoundaryPier() )
      {
         SetPierConnectionType(rOther.m_PierConnectionType);
      }
      else
      {
         const CClosureJointData* pClosure = rOther.GetClosureJoint(0);
         EventIndexType eventIdx = rOther.GetBridgeDescription()->GetTimelineManager()->GetCastClosureJointEventIndex(pClosure->GetID());
         SetSegmentConnectionType(rOther.m_SegmentConnectionType,eventIdx);
      }
   }
   else
   {
      // If this pier is not part of a bridge, just capture the data
      m_PierConnectionType    = rOther.m_PierConnectionType;
      m_SegmentConnectionType = rOther.m_SegmentConnectionType;
   }

   ASSERT_VALID;
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

   ValidatePierConnectionType();
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
   return m_pBridgeDesc->GetGirderGroup(m_pPrevSpan);
}

CGirderGroupData* CPierData2::GetNextGirderGroup()
{
   return m_pBridgeDesc->GetGirderGroup(m_pNextSpan);
}

CGirderGroupData* CPierData2::GetGirderGroup(pgsTypes::PierFaceType face)
{
   return (face == pgsTypes::Ahead ? GetNextGirderGroup() : GetPrevGirderGroup());
}

const CGirderGroupData* CPierData2::GetPrevGirderGroup() const
{
   return m_pBridgeDesc->GetGirderGroup(m_pPrevSpan);
}

const CGirderGroupData* CPierData2::GetNextGirderGroup() const
{
   return m_pBridgeDesc->GetGirderGroup(m_pNextSpan);
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

void CPierData2::HasCantilever(bool bHasCantilever)
{
   m_bHasCantilever = bHasCantilever;
   ValidatePierConnectionType();
}

bool CPierData2::HasCantilever() const
{
   return m_bHasCantilever;
}

void CPierData2::SetCantileverLength(Float64 Lc)
{
   m_CantileverLength = Lc;
}

Float64 CPierData2::GetCantileverLength() const
{
   return m_CantileverLength;
}

pgsTypes::PierConnectionType CPierData2::GetPierConnectionType() const
{
#if defined _DEBUG
   if ( IsInteriorPier() )
   {
      ATLASSERT(m_PierConnectionType == pgsTypes::Hinge || m_PierConnectionType == pgsTypes::Roller);
   }
#endif
   return m_PierConnectionType;
}

void CPierData2::SetPierConnectionType(pgsTypes::PierConnectionType type)
{
   m_PierConnectionType = type;
}

pgsTypes::PierSegmentConnectionType CPierData2::GetSegmentConnectionType() const
{
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
         IDType closureID = pClosure->GetID();
         CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();
         EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(closureID);
         pTimelineMgr->GetEventByIndex(eventIdx)->GetCastClosureJointActivity().RemovePier(GetID());
      }

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         pGirder->JoinSegmentsAtPier(m_PierIdx);
      }
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
         pGirder->SplitSegmentsAtPier(m_PierIdx);
      }

      // add the closure joint casting events to the timeline manager.
      CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();
      CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(castClosureJointEvent);
      ATLASSERT(pTimelineEvent != NULL);
      pTimelineEvent->GetCastClosureJointActivity().AddPier(GetID());

      m_GirderSpacing[pgsTypes::Back].SetGirderCount(nGirders);
      m_GirderSpacing[pgsTypes::Ahead].SetGirderCount(nGirders);
   }

   m_SegmentConnectionType = newType;
}

void CPierData2::SetGirderEndDistance(pgsTypes::PierFaceType face,Float64 endDist,ConnectionLibraryEntry::EndDistanceMeasurementType measure)
{
   m_GirderEndDistance[face] = endDist;
   m_EndDistanceMeasurementType[face] = measure;
}

void CPierData2::GetGirderEndDistance(pgsTypes::PierFaceType face,Float64* pEndDist,ConnectionLibraryEntry::EndDistanceMeasurementType* pMeasure) const
{
   if ( m_bHasCantilever )
   {
      *pEndDist = m_CantileverLength;
      *pMeasure = ConnectionLibraryEntry::FromBearingAlongGirder;
   }
   else
   {
      *pEndDist = m_GirderEndDistance[face];
      *pMeasure = m_EndDistanceMeasurementType[face];
   }
}

void CPierData2::SetBearingOffset(pgsTypes::PierFaceType face,Float64 offset,ConnectionLibraryEntry::BearingOffsetMeasurementType measure)
{
   m_GirderBearingOffset[face] = offset;
   m_BearingOffsetMeasurementType[face] = measure;
}

void CPierData2::GetBearingOffset(pgsTypes::PierFaceType face,Float64* pOffset,ConnectionLibraryEntry::BearingOffsetMeasurementType* pMeasure) const
{
   if ( m_bHasCantilever )
   {
      *pOffset = 0.0;
   }
   else
   {
      *pOffset = m_GirderBearingOffset[face];
   }
   *pMeasure = m_BearingOffsetMeasurementType[face];
}

void CPierData2::SetSupportWidth(pgsTypes::PierFaceType face,Float64 w)
{
   m_SupportWidth[face] = w;
}

Float64 CPierData2::GetSupportWidth(pgsTypes::PierFaceType face) const
{
   return m_SupportWidth[face];
}

void CPierData2::SetGirderSpacing(pgsTypes::PierFaceType pierFace,const CGirderSpacing2& spacing)
{
   m_GirderSpacing[pierFace] = spacing;
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
      return NULL;
   }

   if ( m_SegmentConnectionType == pgsTypes::psctContinuousSegment || m_SegmentConnectionType == pgsTypes::psctIntegralSegment )
   {
      return NULL;
   }

   // If there is a closure at this pier, then this pier is in the middle of a group
   // so the group on the ahead and back side of this pier are the same. There can't
   // be a closure here if this is an end pier
   CGirderGroupData* pGroup = GetGirderGroup(pgsTypes::Ahead); // get on ahead side... could be back side
   if ( pGroup == NULL )
   {
      return NULL;
   }

   CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      // NOTE: nSegments-1 because there is one less closure than segments
      // no need to check the right end of the last segment as there isn't a closure there)
      CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      CClosureJointData* pClosure = pSegment->GetRightClosure();
      if ( pClosure->GetPier() == this )
      {
         return pClosure;
      }
   }

   return NULL;
}

const CClosureJointData* CPierData2::GetClosureJoint(GirderIndexType gdrIdx) const
{
   if ( IsBoundaryPier() )
   {
      ATLASSERT(false); // why are you asking for a closure joint at a boundary pier? it doesn't have one
      return NULL;
   }

   if ( m_SegmentConnectionType == pgsTypes::psctContinuousSegment || m_SegmentConnectionType == pgsTypes::psctIntegralSegment )
   {
      return NULL;
   }

   // If there is a closure at this pier, then this pier is in the middle of a group
   // so the group on the ahead and back side of this pier are the same. There can't
   // be a closure here if this is an end pier
   const CGirderGroupData* pGroup = GetGirderGroup(pgsTypes::Ahead); // get on ahead side... could be back side
   if ( pGroup == NULL )
   {
      return NULL;
   }

   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      // NOTE: nSegments-1 because there is one less closure than segments
      // no need to check the right end of the last segment as there isn't a closure there)
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      const CClosureJointData* pClosure = pSegment->GetRightClosure();
      if ( pClosure->GetPier() == this )
      {
         return pClosure;
      }
   }

   return NULL;
}

void CPierData2::SetDiaphragmHeight(pgsTypes::PierFaceType pierFace,Float64 d)
{
   m_DiaphragmHeight[pierFace] = d;
}

Float64 CPierData2::GetDiaphragmHeight(pgsTypes::PierFaceType pierFace) const
{
   return m_DiaphragmHeight[pierFace];
}

void CPierData2::SetDiaphragmWidth(pgsTypes::PierFaceType pierFace,Float64 w)
{
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

Float64 CPierData2::GetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gR[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CPierData2::SetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls, Float64 gR)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gR[ls == pgsTypes::FatigueI ? 1 : 0] = gR;
}

void CPierData2::SetLLDFReaction(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls, Float64 gM)
{
   GirderIndexType ngdrs = GetLldfGirderCount();
   if (ngdrs>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<ngdrs-1; ig++)
      {
         SetLLDFReaction(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFReaction(0,ls,gM);
      SetLLDFReaction(ngdrs-1,ls,gM);
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
      if ( m_PierConnectionType == pgsTypes::ContinuousAfterDeck ||
           m_PierConnectionType == pgsTypes::ContinuousBeforeDeck ||
           m_PierConnectionType == pgsTypes::IntegralAfterDeck ||
           m_PierConnectionType == pgsTypes::IntegralBeforeDeck )
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
      return (m_PierConnectionType == pgsTypes::ContinuousBeforeDeck || m_PierConnectionType == pgsTypes::ContinuousAfterDeck);
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
      if (m_PierConnectionType == pgsTypes::IntegralBeforeDeck || m_PierConnectionType == pgsTypes::IntegralAfterDeck)
      {
         *pbLeft  = true;
         *pbRight = true;
      }
      else
      {
         *pbLeft  = m_PierConnectionType == pgsTypes::IntegralAfterDeckHingeAhead || m_PierConnectionType == pgsTypes::IntegralBeforeDeckHingeAhead;
         *pbRight = m_PierConnectionType == pgsTypes::IntegralAfterDeckHingeBack  || m_PierConnectionType == pgsTypes::IntegralBeforeDeckHingeBack;
      }
   }
}

bool CPierData2::IsAbutment() const
{
   return (m_pPrevSpan == NULL || m_pNextSpan == NULL) ? true : false;
}

bool CPierData2::IsPier() const
{
   return !IsAbutment();
}

bool CPierData2::IsInteriorPier() const
{
   // If the girder group on both sides of the pier is the same, then this pier
   // is interior to the group.
   ATLASSERT(m_pBridgeDesc != NULL); // pier data must be part of a bridge model
   return (GetPrevGirderGroup() == GetNextGirderGroup() ? true : false);
}

bool CPierData2::IsBoundaryPier() const
{
   return !IsInteriorPier();
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
   if (igs<0)
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

   if (pBack == NULL && pAhead == NULL)
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
   SetPierConnectionType(pPier->m_ConnectionType);
   //SetSegmentConnectionType(pPier->m_SegmentConnectionType);

   SetGirderEndDistance(pgsTypes::Back, pPier->m_GirderEndDistance[pgsTypes::Back], pPier->m_EndDistanceMeasurementType[pgsTypes::Back]);
   SetGirderEndDistance(pgsTypes::Ahead,pPier->m_GirderEndDistance[pgsTypes::Ahead],pPier->m_EndDistanceMeasurementType[pgsTypes::Ahead]);
   
   SetBearingOffset(pgsTypes::Back, pPier->m_GirderBearingOffset[pgsTypes::Back], pPier->m_BearingOffsetMeasurementType[pgsTypes::Back]);
   SetBearingOffset(pgsTypes::Ahead,pPier->m_GirderBearingOffset[pgsTypes::Ahead],pPier->m_BearingOffsetMeasurementType[pgsTypes::Ahead]);

   SetSupportWidth(pgsTypes::Back, pPier->m_SupportWidth[pgsTypes::Back]);
   SetSupportWidth(pgsTypes::Ahead,pPier->m_SupportWidth[pgsTypes::Ahead]);

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

      lldf2.gR[0] = lldf.gR[0];
      lldf2.gR[1] = lldf.gR[1];
      
      m_LLDFs.push_back(lldf2);
   }

   m_bDistributionFactorsFromOlderVersion = true;
}

void CPierData2::ValidatePierConnectionType()
{
   // make sure the connection type is valid
   std::vector<pgsTypes::PierConnectionType> vConnectionTypes = m_pBridgeDesc->GetPierConnectionTypes(m_PierIdx);
   std::vector<pgsTypes::PierConnectionType>::iterator found = std::find(vConnectionTypes.begin(),vConnectionTypes.end(),m_PierConnectionType);
   if ( found == vConnectionTypes.end() )
   {
      // the current connection type isn't valid... updated it
      m_PierConnectionType = vConnectionTypes.front();
   }
}

#if defined _DEBUG
void CPierData2::AssertValid() const
{
   // Girder spacing is either attached to this pier or nothing
   // can't be attached to temporary support
   ATLASSERT(m_GirderSpacing[pgsTypes::Back].GetTemporarySupport()  == NULL);
   ATLASSERT(m_GirderSpacing[pgsTypes::Ahead].GetTemporarySupport() == NULL);

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
      _ASSERT(m_pPrevSpan == NULL);
      _ASSERT(m_pNextSpan == NULL);
   }
}
#endif
