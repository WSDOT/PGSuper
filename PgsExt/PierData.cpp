///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include "BridgeDescription.h"

#include <PgsExt\PierData2.h>

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

/****************************************************************************
CLASS
   CPierData
****************************************************************************/

CPierData::CPierData()
{
   m_PierIdx = INVALID_INDEX;
   m_pPrevSpan = nullptr;
   m_pNextSpan = nullptr;

   m_pBridgeDesc = nullptr;

   m_Station     = 0.0;
   Orientation = Normal;
   Angle       = 0.0;

   m_ConnectionType                    = pgsTypes::bctHinge;
   m_strOrientation = _T("Normal");

   for ( int i = 0; i < 2; i++ )
   {
      m_GirderEndDistance[i]            = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Inch);
      m_EndDistanceMeasurementType[i]   = ConnectionLibraryEntry::FromBearingNormalToPier;

      m_GirderBearingOffset[i]          = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet);
      m_BearingOffsetMeasurementType[i] = ConnectionLibraryEntry::NormalToPier;

      m_SupportWidth[i]                 = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet);

      m_DiaphragmHeight[i] = 0;
      m_DiaphragmWidth[i] = 0;
      m_DiaphragmLoadType[i] = ConnectionLibraryEntry::DontApply;
      m_DiaphragmLoadLocation[i] = 0;
   }

   m_DistributionFactorsFromOlderVersion = false;
}

CPierData::CPierData(const CPierData& rOther)
{
   m_pPrevSpan = nullptr;
   m_pNextSpan = nullptr;

   m_pBridgeDesc = nullptr;

   m_Station   = 0.0;
   Orientation = Normal;
   Angle       = 0.0;
   
   m_strOrientation = _T("Normal");

   MakeCopy(rOther);
}

CPierData::~CPierData()
{
}

CPierData& CPierData::operator= (const CPierData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CPierData::operator==(const CPierData& rOther) const
{
   if ( m_PierIdx != rOther.m_PierIdx )
      return false;

   if ( m_Station != rOther.m_Station )
      return false;

   if ( m_strOrientation != rOther.m_strOrientation )
      return false;

   if ( m_ConnectionType != rOther.m_ConnectionType )
      return false;

   for ( int i = 0; i < 2; i++ )
   {
      if ( !IsEqual(m_GirderEndDistance[i], rOther.m_GirderEndDistance[i]) )
         return false;

      if ( m_EndDistanceMeasurementType[i] != rOther.m_EndDistanceMeasurementType[i] )
         return false;

      if ( !IsEqual(m_GirderBearingOffset[i], rOther.m_GirderBearingOffset[i]) )
         return false;

      if ( m_BearingOffsetMeasurementType[i] != rOther.m_BearingOffsetMeasurementType[i] )
         return false;

      if ( !IsEqual(m_SupportWidth[i], rOther.m_SupportWidth[i]) )
         return false;
   }


   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      if (m_LLDFs != rOther.m_LLDFs)
         return false;
   }

   return true;
}

bool CPierData::operator!=(const CPierData& rOther) const
{
   return !operator==(rOther);
}

LPCTSTR CPierData::AsString(pgsTypes::BoundaryConditionType type)
{
   switch(type)
   { 
   case pgsTypes::bctHinge:
      return _T("Hinged");

   case pgsTypes::bctRoller:
      return _T("Roller");

   case pgsTypes::bctContinuousAfterDeck:
      return _T("Continuous after deck placement");

   case pgsTypes::bctContinuousBeforeDeck:
      return _T("Continuous before deck placement");

   case pgsTypes::bctIntegralAfterDeck:
      return _T("Integral after deck placement");

   case pgsTypes::bctIntegralBeforeDeck:
      return _T("Integral before deck placement");

   case pgsTypes::bctIntegralAfterDeckHingeBack:
      return _T("Hinged on back side; Integral on ahead side after deck placement");

   case pgsTypes::bctIntegralBeforeDeckHingeBack:
      return _T("Hinged on back side; Integral on ahead side before deck placement");

   case pgsTypes::bctIntegralAfterDeckHingeAhead:
      return _T("Integral on back side after deck placement; Hinged on ahead side");

   case pgsTypes::bctIntegralBeforeDeckHingeAhead:
      return _T("Integral on back side before deck placement; Hinged on ahead side");
   
   default:
      ATLASSERT(false);

   };

   return _T("");
}

HRESULT CPierData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   // Use this version of Load if you are directly loading an old version
   std::_tstring strUnitName(_T("PierDataDetails"));
   HRESULT hr2 = pStrLoad->BeginUnit(strUnitName.c_str());

   if ( FAILED(hr2) )
   {
      strUnitName = _T("PierData");
      hr2 = pStrLoad->BeginUnit(strUnitName.c_str());
   }

   if ( FAILED(hr2) )
      return STRLOAD_E_INVALIDFORMAT;

   Float64 version;
   pStrLoad->get_Version(&version);

   if ( FAILED(Load(version,pStrLoad,pProgress,strUnitName)) )
      return STRLOAD_E_INVALIDFORMAT;

   if ( FAILED(pStrLoad->EndUnit()) )
      return STRLOAD_E_INVALIDFORMAT;

   return S_OK;
}

HRESULT CPierData::Load(Float64 version,IStructuredLoad* pStrLoad,IProgress* pProgress,const std::_tstring& strUnitName)
{
   // Use this version of you've already started the "PierData" or "PierDataDetails" data block
   // This is done by the newer bridge description object

   USES_CONVERSION;
   HRESULT hr = S_OK;

   std::_tstring strConnection[2];

   if ( version == 1.0 )
   {
      CComVariant var;
      var.vt = VT_R8;

      if ( FAILED(pStrLoad->get_Property(_T("Station"),&var)) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         m_Station = var.dblVal;

      var.Clear();
      var.vt = VT_I4;
      if (FAILED(pStrLoad->get_Property(_T("Orientation"), &var )) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         Orientation = (PierOrientation)var.lVal;

      var.vt = VT_R8;

      if ( FAILED(pStrLoad->get_Property(_T("Angle"),&var)) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         Angle = var.dblVal;


      var.Clear();
      var.vt = VT_BSTR;
      if (FAILED(pStrLoad->get_Property(_T("Connection"), &var )) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         strConnection[pgsTypes::Back] = OLE2T(var.bstrVal);

      strConnection[pgsTypes::Ahead] = strConnection[pgsTypes::Back];
      m_ConnectionType  = pgsTypes::bctHinge;

      // Convert old input into a bearing string
      CComPtr<IDirectionDisplayUnitFormatter> dirFormatter;
      CComPtr<IAngleDisplayUnitFormatter> angleFormatter;
      CComBSTR bstrOrientation;
      switch(Orientation)
      {
      case Normal:
         m_strOrientation = _T("Normal");
         break;

      case Skew:
         angleFormatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
         angleFormatter->put_CondensedFormat(VARIANT_TRUE);
         angleFormatter->put_Signed(VARIANT_FALSE);

         angleFormatter->Format(Angle,nullptr,&bstrOrientation);
         m_strOrientation = OLE2T(bstrOrientation);
         break;
      
      case Bearing:
         dirFormatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
         dirFormatter->put_CondensedFormat(VARIANT_TRUE);
         dirFormatter->put_BearingFormat(VARIANT_TRUE);

         dirFormatter->Format(Angle,nullptr,&bstrOrientation);
         m_strOrientation = OLE2T(bstrOrientation);
         break;

      default:
         ATLASSERT(false);
         return STRLOAD_E_INVALIDFORMAT;
      }

      if ( strUnitName == std::_tstring(_T("PierData")) )
      {
         // Data block unit name is PierData. This means the file was
         // created before changing out the COGO engine in bmfBridgeModeling.
         //
         // The reference for bearings has changed. Do a conversion here.
         if ( Orientation == Bearing )
         {
            Angle = PI_OVER_2 - Angle;
            if ( Angle < 0 )
               Angle += 2*M_PI;
         }
      }

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,ILibrary, pLib );
      const ConnectionLibraryEntry* pConnEntry = pLib->GetConnectionEntry(strConnection[pgsTypes::Ahead].c_str());
      m_GirderEndDistance[pgsTypes::Ahead]            = pConnEntry->GetGirderEndDistance();
      m_EndDistanceMeasurementType[pgsTypes::Ahead]   = pConnEntry->GetEndDistanceMeasurementType();
      m_GirderBearingOffset[pgsTypes::Ahead]          = pConnEntry->GetGirderBearingOffset();
      m_BearingOffsetMeasurementType[pgsTypes::Ahead] = pConnEntry->GetBearingOffsetMeasurementType();
      m_SupportWidth[pgsTypes::Ahead]                 = pConnEntry->GetSupportWidth();

      m_DiaphragmHeight[pgsTypes::Ahead]             = pConnEntry->GetDiaphragmHeight();
      m_DiaphragmWidth[pgsTypes::Ahead]              = pConnEntry->GetDiaphragmWidth();
      m_DiaphragmLoadType[pgsTypes::Ahead]           = pConnEntry->GetDiaphragmLoadType();
      m_DiaphragmLoadLocation[pgsTypes::Ahead]       = pConnEntry->GetDiaphragmLoadLocation();

      pConnEntry = pLib->GetConnectionEntry(strConnection[pgsTypes::Back].c_str());
      m_GirderEndDistance[pgsTypes::Back]            = pConnEntry->GetGirderEndDistance();
      m_EndDistanceMeasurementType[pgsTypes::Back]   = pConnEntry->GetEndDistanceMeasurementType();
      m_GirderBearingOffset[pgsTypes::Back]          = pConnEntry->GetGirderBearingOffset();
      m_BearingOffsetMeasurementType[pgsTypes::Back] = pConnEntry->GetBearingOffsetMeasurementType();
      m_SupportWidth[pgsTypes::Back]                 = pConnEntry->GetSupportWidth();

      m_DiaphragmHeight[pgsTypes::Back]             = pConnEntry->GetDiaphragmHeight();
      m_DiaphragmWidth[pgsTypes::Back]              = pConnEntry->GetDiaphragmWidth();
      m_DiaphragmLoadType[pgsTypes::Back]           = pConnEntry->GetDiaphragmLoadType();
      m_DiaphragmLoadLocation[pgsTypes::Back]       = pConnEntry->GetDiaphragmLoadLocation();
   }
   else
   {
      // greater than version 1
      CComVariant var;
      var.vt = VT_R8;
      if ( FAILED(pStrLoad->get_Property(_T("Station"),&var)) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         m_Station = var.dblVal;

      var.Clear();
      var.vt = VT_BSTR;
      if (FAILED(pStrLoad->get_Property(_T("Orientation"), &var )) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         m_strOrientation = OLE2T(var.bstrVal);

      if ( 4.0 <= version )
      {
         var.Clear();

         if ( 6.0 <= version && version < 8 )
         {
            var.vt = VT_R8;
            if ( FAILED(pStrLoad->get_Property(_T("AlignmentOffset"),&var)))
               return STRLOAD_E_INVALIDFORMAT;
//            else // removed in version 8
//               m_AlignmentOffset = var.dblVal;
         }

         // prior to version 7 we had left and right boundary conditions with an option to make both same

         bool use_same_both = false;
         pgsTypes::BoundaryConditionType back_conn_type, ahead_conn_type;
         if ( 5.0 <= version && version < 7.0 )
         {
            var.vt = VT_BOOL;
            if (FAILED(pStrLoad->get_Property(_T("UseSameConnectionOnBothSides"),&var)))
               return STRLOAD_E_INVALIDFORMAT;
            else
               use_same_both = (var.boolVal == VARIANT_TRUE);
         }


         if ( version < 9 )
         {
            var.vt = VT_BSTR;
            if (FAILED(pStrLoad->get_Property(_T("LeftConnection"), &var )) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               strConnection[pgsTypes::Back] = OLE2T(var.bstrVal);
         }

         if ( version < 7.0 )
         {
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property(_T("LeftConnectionType"),&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               back_conn_type = (pgsTypes::BoundaryConditionType)var.lVal;
         }

         if ( version < 9 )
         {
            var.Clear();
            var.vt = VT_BSTR;
            if (FAILED(pStrLoad->get_Property(_T("RightConnection"), &var )) )
               return STRLOAD_E_INVALIDFORMAT;
            else
            {
               if (!use_same_both)
               {
                  strConnection[pgsTypes::Ahead] = OLE2T(var.bstrVal);
               }
               else
               {
                  strConnection[pgsTypes::Ahead] = strConnection[pgsTypes::Back]; 
               }
            }
         }

         if ( 7.0 <= version )
         {
            // After version 7.0, we have single definition for boundary conditions over pier
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property(_T("ConnectionType"),&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               m_ConnectionType = (pgsTypes::BoundaryConditionType)var.lVal;
         }
         else
         {
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property(_T("RightConnectionType"),&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               ahead_conn_type = (pgsTypes::BoundaryConditionType)var.lVal;

            // Pre-version 7.0, we must resolve separate bc's for ahead and back
            // Tricky: this can be ambiguous
            m_ConnectionType = back_conn_type;

            if (!use_same_both)
            {
               // Only some conditions where different bc's made sense (main reason why we got rid of ahead/back bc concept)
               if ( back_conn_type < 0 )
               {
                  // there isn't a back side to this pier
                  m_ConnectionType = ahead_conn_type;
               }
               else if ( ahead_conn_type < 0 )
               {
                  // there isn't an ahead side to this pier
                  m_ConnectionType = back_conn_type;
               }
               else if (back_conn_type == pgsTypes::bctContinuousAfterDeck || ahead_conn_type == pgsTypes::bctContinuousAfterDeck)
               {
                  m_ConnectionType = pgsTypes::bctContinuousAfterDeck;
               }
               else if (back_conn_type == pgsTypes::bctContinuousBeforeDeck || ahead_conn_type == pgsTypes::bctContinuousBeforeDeck)
               {
                  m_ConnectionType = pgsTypes::bctContinuousBeforeDeck;
               }
               else if (back_conn_type == pgsTypes::bctIntegralAfterDeck)
               {
                  if (ahead_conn_type == pgsTypes::bctHinge || ahead_conn_type == pgsTypes::bctRoller)
                  {
                     m_ConnectionType = pgsTypes::bctIntegralAfterDeckHingeAhead;
                  }
               }
               else if (back_conn_type == pgsTypes::bctIntegralBeforeDeck)
               {
                  if (ahead_conn_type == pgsTypes::bctHinge || ahead_conn_type == pgsTypes::bctRoller)
                  {
                     m_ConnectionType = pgsTypes::bctIntegralBeforeDeckHingeAhead;
                  }
               }
               else if (ahead_conn_type == pgsTypes::bctIntegralAfterDeck)
               {
                  if (back_conn_type == pgsTypes::bctHinge || back_conn_type == pgsTypes::bctRoller)
                  {
                     m_ConnectionType = pgsTypes::bctIntegralAfterDeckHingeBack;
                  }
               }
               else if (ahead_conn_type == pgsTypes::bctIntegralBeforeDeck)
               {
                  if (back_conn_type == pgsTypes::bctHinge || back_conn_type == pgsTypes::bctRoller)
                  {
                     m_ConnectionType = pgsTypes::bctIntegralBeforeDeckHingeBack;
                  }
               }
            }
         }

      }
      else
      {
         var.Clear();
         var.vt = VT_BSTR;
         if (FAILED(pStrLoad->get_Property(_T("Connection"), &var )) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            strConnection[pgsTypes::Back] = OLE2T(var.bstrVal);

         strConnection[pgsTypes::Ahead] = strConnection[pgsTypes::Back];

         if ( 3.0 <= version )
         {
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property(_T("ConnectionType"),&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               m_ConnectionType = (pgsTypes::BoundaryConditionType)var.lVal;
         }
      }

      if ( 8 < version )
      {
         // added in version 9
         pStrLoad->BeginUnit(_T("Back"));

         var.vt = VT_R8;
         if ( FAILED( pStrLoad->get_Property(_T("GirderEndDistance"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else 
            m_GirderEndDistance[pgsTypes::Back] = var.dblVal;

         var.vt = VT_BSTR;
         if ( FAILED(pStrLoad->get_Property(_T("EndDistanceMeasurementType"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_EndDistanceMeasurementType[pgsTypes::Back] = ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(OLE2T(var.bstrVal));

         var.vt = VT_R8;
         if ( FAILED(pStrLoad->get_Property(_T("GirderBearingOffset"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else 
            m_GirderBearingOffset[pgsTypes::Back] = var.dblVal;

         var.vt = VT_BSTR;
         if ( FAILED(pStrLoad->get_Property(_T("BearingOffsetMeasurementType"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else 
            m_BearingOffsetMeasurementType[pgsTypes::Back] = ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(OLE2T(var.bstrVal));

         var.vt = VT_R8;
         if ( FAILED(pStrLoad->get_Property(_T("SupportWidth"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_SupportWidth[pgsTypes::Back] = var.dblVal;

         pStrLoad->BeginUnit(_T("Diaphragm"));
         var.vt = VT_R8;
         if( FAILED(pStrLoad->get_Property(_T("DiaphragmWidth"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_DiaphragmWidth[pgsTypes::Back] = var.dblVal;

         if( FAILED(pStrLoad->get_Property(_T("DiaphragmHeight"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_DiaphragmHeight[pgsTypes::Back] = var.dblVal;


         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("DiaphragmLoadType"),&var);
         if ( FAILED(hr) )
         {
            // there was a bug in version 2.8.2 that caused the DiaphragmLoadType to
            // be omitted when it was set to "DontApply". If there is a problem loading
            // the DiaphragmLoadType, assume it should be "DontApply"
            var.bstrVal = T2BSTR(_T("DontApply"));
            hr = S_OK;
         }

         std::_tstring tmp(OLE2T(var.bstrVal));
         if (tmp==_T("ApplyAtBearingCenterline"))
         {
            m_DiaphragmLoadType[pgsTypes::Back] = ConnectionLibraryEntry::ApplyAtBearingCenterline;
         }
         else if (tmp==_T("ApplyAtSpecifiedLocation"))
         {
            m_DiaphragmLoadType[pgsTypes::Back] = ConnectionLibraryEntry::ApplyAtSpecifiedLocation;

            var.vt = VT_R8;
            if ( FAILED(pStrLoad->get_Property(_T("DiaphragmLoadLocation"),&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               m_DiaphragmLoadLocation[pgsTypes::Back] = var.dblVal;
         }
         else if (tmp==_T("DontApply"))
         {
            m_DiaphragmLoadType[pgsTypes::Back] = ConnectionLibraryEntry::DontApply;
         }
         else
         {
            return STRLOAD_E_INVALIDFORMAT;
         }

         pStrLoad->EndUnit(); // Diaphragm
         pStrLoad->EndUnit(); // Back

         // added in version 9
         pStrLoad->BeginUnit(_T("Ahead"));

         var.vt = VT_R8;
         if ( FAILED( pStrLoad->get_Property(_T("GirderEndDistance"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else 
            m_GirderEndDistance[pgsTypes::Ahead] = var.dblVal;

         var.vt = VT_BSTR;
         if ( FAILED(pStrLoad->get_Property(_T("EndDistanceMeasurementType"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_EndDistanceMeasurementType[pgsTypes::Ahead] = ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(OLE2T(var.bstrVal));

         var.vt = VT_R8;
         if ( FAILED(pStrLoad->get_Property(_T("GirderBearingOffset"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else 
            m_GirderBearingOffset[pgsTypes::Ahead] = var.dblVal;

         var.vt = VT_BSTR;
         if ( FAILED(pStrLoad->get_Property(_T("BearingOffsetMeasurementType"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else 
            m_BearingOffsetMeasurementType[pgsTypes::Ahead] = ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(OLE2T(var.bstrVal));

         var.vt = VT_R8;
         if ( FAILED(pStrLoad->get_Property(_T("SupportWidth"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_SupportWidth[pgsTypes::Ahead] = var.dblVal;

         pStrLoad->BeginUnit(_T("Diaphragm"));
         var.vt = VT_R8;
         if( FAILED(pStrLoad->get_Property(_T("DiaphragmWidth"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_DiaphragmWidth[pgsTypes::Ahead] = var.dblVal;

         if( FAILED(pStrLoad->get_Property(_T("DiaphragmHeight"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_DiaphragmHeight[pgsTypes::Ahead] = var.dblVal;


         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("DiaphragmLoadType"),&var);
         if ( FAILED(hr) )
         {
            // there was a bug in version 2.8.2 that caused the DiaphragmLoadType to
            // be omitted when it was set to "DontApply". If there is a problem loading
            // the DiaphragmLoadType, assume it should be "DontApply"
            var.bstrVal = T2BSTR(_T("DontApply"));
            hr = S_OK;
         }

         tmp = OLE2T(var.bstrVal);
         if (tmp==_T("ApplyAtBearingCenterline"))
         {
            m_DiaphragmLoadType[pgsTypes::Ahead] = ConnectionLibraryEntry::ApplyAtBearingCenterline;
         }
         else if (tmp==_T("ApplyAtSpecifiedLocation"))
         {
            m_DiaphragmLoadType[pgsTypes::Ahead] = ConnectionLibraryEntry::ApplyAtSpecifiedLocation;

            var.vt = VT_R8;
            if ( FAILED(pStrLoad->get_Property(_T("DiaphragmLoadLocation"),&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               m_DiaphragmLoadLocation[pgsTypes::Ahead] = var.dblVal;
         }
         else if (tmp==_T("DontApply"))
         {
            m_DiaphragmLoadType[pgsTypes::Ahead] = ConnectionLibraryEntry::DontApply;
         }
         else
         {
            return STRLOAD_E_INVALIDFORMAT;
         }

         pStrLoad->EndUnit(); // Diaphragm

         pStrLoad->EndUnit(); // Ahead
      }
      else
      {
         // look up connection details and set values
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,ILibrary, pLib );

         if ( m_pBridgeDesc == nullptr || m_pBridgeDesc && m_pPrevSpan )
         {
            const ConnectionLibraryEntry* pConnEntry = pLib->GetConnectionEntry(strConnection[pgsTypes::Back].c_str());
            m_GirderEndDistance[pgsTypes::Back]            = pConnEntry->GetGirderEndDistance();
            m_EndDistanceMeasurementType[pgsTypes::Back]   = pConnEntry->GetEndDistanceMeasurementType();
            m_GirderBearingOffset[pgsTypes::Back]          = pConnEntry->GetGirderBearingOffset();
            m_BearingOffsetMeasurementType[pgsTypes::Back] = pConnEntry->GetBearingOffsetMeasurementType();
            m_SupportWidth[pgsTypes::Back]                 = pConnEntry->GetSupportWidth();

            m_DiaphragmHeight[pgsTypes::Back]              = pConnEntry->GetDiaphragmHeight();
            m_DiaphragmWidth[pgsTypes::Back]               = pConnEntry->GetDiaphragmWidth();
            m_DiaphragmLoadType[pgsTypes::Back]            = pConnEntry->GetDiaphragmLoadType();
            m_DiaphragmLoadLocation[pgsTypes::Back]        = pConnEntry->GetDiaphragmLoadLocation();

            if ( m_pNextSpan == nullptr )
            {
               m_EndDistanceMeasurementType[pgsTypes::Ahead] = m_EndDistanceMeasurementType[pgsTypes::Back];
               m_BearingOffsetMeasurementType[pgsTypes::Ahead] = m_BearingOffsetMeasurementType[pgsTypes::Back];
            }
         }

         if ( m_pBridgeDesc == nullptr || m_pBridgeDesc && m_pNextSpan )
         {
            const ConnectionLibraryEntry* pConnEntry = pLib->GetConnectionEntry(strConnection[pgsTypes::Ahead].c_str());
            m_GirderEndDistance[pgsTypes::Ahead]            = pConnEntry->GetGirderEndDistance();
            m_EndDistanceMeasurementType[pgsTypes::Ahead]   = pConnEntry->GetEndDistanceMeasurementType();
            m_GirderBearingOffset[pgsTypes::Ahead]          = pConnEntry->GetGirderBearingOffset();
            m_BearingOffsetMeasurementType[pgsTypes::Ahead] = pConnEntry->GetBearingOffsetMeasurementType();
            m_SupportWidth[pgsTypes::Ahead]                 = pConnEntry->GetSupportWidth();

            m_DiaphragmHeight[pgsTypes::Ahead]              = pConnEntry->GetDiaphragmHeight();
            m_DiaphragmWidth[pgsTypes::Ahead]               = pConnEntry->GetDiaphragmWidth();
            m_DiaphragmLoadType[pgsTypes::Ahead]            = pConnEntry->GetDiaphragmLoadType();
            m_DiaphragmLoadLocation[pgsTypes::Ahead]        = pConnEntry->GetDiaphragmLoadLocation();

            if ( m_pPrevSpan == nullptr )
            {
               m_EndDistanceMeasurementType[pgsTypes::Back] = m_EndDistanceMeasurementType[pgsTypes::Ahead];
               m_BearingOffsetMeasurementType[pgsTypes::Back] = m_BearingOffsetMeasurementType[pgsTypes::Ahead];
            }
         }
      }

      if ( 5.0 <= version )
      {
         // LLDF stuff added in version 5
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
                     return STRLOAD_E_INVALIDFORMAT;

                  gM[pgsTypes::Interior][0] = var.dblVal;
                  gM[pgsTypes::Interior][1] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gM_Exterior"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gM[pgsTypes::Exterior][0] = var.dblVal;
                  gM[pgsTypes::Exterior][1] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gR_Interior"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gR[pgsTypes::Interior][0] = var.dblVal;
                  gR[pgsTypes::Interior][1] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gR[pgsTypes::Exterior][0] = var.dblVal;
                  gR[pgsTypes::Exterior][1] = var.dblVal;
               }
               else
               {
                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gM_Interior_Strength"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gM[pgsTypes::Interior][0] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gM_Exterior_Strength"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gM[pgsTypes::Exterior][0] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gR_Interior_Strength"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gR[pgsTypes::Interior][0] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior_Strength"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gR[pgsTypes::Exterior][0] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gM_Interior_Fatigue"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gM[pgsTypes::Interior][1] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gM_Exterior_Fatigue"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gM[pgsTypes::Exterior][1] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gR_Interior_Fatigue"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gR[pgsTypes::Interior][1] = var.dblVal;

                  var.vt = VT_R8;
                  if ( FAILED(pStrLoad->get_Property(_T("gR_Exterior_Fatigue"),&var)) )
                     return STRLOAD_E_INVALIDFORMAT;

                  gR[pgsTypes::Exterior][1] = var.dblVal;
               }

               // Move interior and exterior factors into first two slots in df vector. We will 
               // need to move them into all girder slots once this object is fully connected to the bridge
               m_DistributionFactorsFromOlderVersion = true;

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

         // link stuff added in version 5
         // RAB: 10/17/2008 - got rid of the link stuff... still need to read this attribute
         var.Clear();
         var.vt = VT_BOOL;
         bool bIsLinked;
         if ( FAILED(pStrLoad->get_Property(_T("IsLinked"),&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            bIsLinked = (var.boolVal == VARIANT_TRUE ? true : false);

      }
   }

   return hr;
}

HRESULT CPierData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   ATLASSERT(false); // should never get here
   HRESULT hr = S_OK;

   //pStrSave->BeginUnit(_T("PierDataDetails"),9.0);

   //pStrSave->put_Property(_T("Station"),         CComVariant(m_Station) );
   //pStrSave->put_Property(_T("Orientation"),     CComVariant( CComBSTR(m_strOrientation.c_str()) ) );
   ////pStrSave->put_Property(_T("AlignmentOffset"), CComVariant( m_AlignmentOffset) ); // added in version 6, removed in version 8
   ////pStrSave->put_Property(_T("LeftConnection"),  CComVariant( CComBSTR(m_Connection[pgsTypes::Back].c_str()) ) ); // removed in version 9
   ////pStrSave->put_Property(_T("RightConnection"), CComVariant( CComBSTR(m_Connection[pgsTypes::Ahead].c_str()) ) ); // removed in version 9
   //pStrSave->put_Property(_T("ConnectionType"),  CComVariant( m_ConnectionType ) ); // changed from left and right to a single value in version 7

   //// added in version 9
   //pStrSave->BeginUnit(_T("Back"),1.0);
   //pStrSave->put_Property(_T("GirderEndDistance"),CComVariant( m_GirderEndDistance[pgsTypes::Back] ) );
   //pStrSave->put_Property(_T("EndDistanceMeasurementType"), CComVariant(ConnectionLibraryEntry::StringForEndDistanceMeasurementType(m_EndDistanceMeasurementType[pgsTypes::Back]).c_str()) );
   //pStrSave->put_Property(_T("GirderBearingOffset"),CComVariant(m_GirderBearingOffset[pgsTypes::Back]));
   //pStrSave->put_Property(_T("BearingOffsetMeasurementType"),CComVariant(ConnectionLibraryEntry::StringForBearingOffsetMeasurementType(m_BearingOffsetMeasurementType[pgsTypes::Back]).c_str()) );
   //pStrSave->put_Property(_T("SupportWidth"),CComVariant(m_SupportWidth[pgsTypes::Back]));
   //pStrSave->EndUnit();

   //// added in version 9
   //pStrSave->BeginUnit(_T("Ahead"),1.0);
   //pStrSave->put_Property(_T("GirderEndDistance"),CComVariant( m_GirderEndDistance[pgsTypes::Ahead] ) );
   //pStrSave->put_Property(_T("EndDistanceMeasurementType"), CComVariant(ConnectionLibraryEntry::StringForEndDistanceMeasurementType(m_EndDistanceMeasurementType[pgsTypes::Ahead]).c_str()) );
   //pStrSave->put_Property(_T("GirderBearingOffset"),CComVariant(m_GirderBearingOffset[pgsTypes::Ahead]));
   //pStrSave->put_Property(_T("BearingOffsetMeasurementType"),CComVariant(ConnectionLibraryEntry::StringForBearingOffsetMeasurementType(m_BearingOffsetMeasurementType[pgsTypes::Ahead]).c_str()) );
   //pStrSave->put_Property(_T("SupportWidth"),CComVariant(m_SupportWidth[pgsTypes::Ahead]));
   //pStrSave->EndUnit();

   //pStrSave->BeginUnit(_T("Diaphragm"),1.0);
   //pStrSave->put_Property(_T("DiaphragmWidth"),  CComVariant(m_DiaphragmWidth));
   //pStrSave->put_Property(_T("DiaphragmHeight"), CComVariant(m_DiaphragmHeight));

   //if (m_DiaphragmLoadType == ConnectionLibraryEntry::ApplyAtBearingCenterline)
   //{
   //   pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("ApplyAtBearingCenterline")));
   //}
   //else if (m_DiaphragmLoadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
   //{
   //   pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("ApplyAtSpecifiedLocation")));
   //   pStrSave->put_Property(_T("DiaphragmLoadLocation"),CComVariant(m_DiaphragmLoadLocation));
   //}
   //else if (m_DiaphragmLoadType == ConnectionLibraryEntry::DontApply)
   //{
   //   pStrSave->put_Property(_T("DiaphragmLoadType"),CComVariant(_T("DontApply")));
   //}
   //else
   //{
   //   ATLASSERT(false); // is there a new load type?
   //}
   //pStrSave->EndUnit(); // Diaphragm

   //// added in version 5
   //if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   //{
   //   pStrSave->BeginUnit(_T("LLDF"),3.0); // Version 3 went from interior/exterior to girder by girder

   //   GirderIndexType ngs = GetLldfGirderCount();
   //   pStrSave->put_Property(_T("nLLDFGirders"),CComVariant(ngs));

   //   for (GirderIndexType igs=0; igs<ngs; igs++)
   //   {
   //      pStrSave->BeginUnit(_T("LLDF_Girder"),1.0);
   //      LLDF& lldf = GetLLDF(igs);

   //      pStrSave->put_Property(_T("gM_Strength"), CComVariant(lldf.gM[0]));
   //      pStrSave->put_Property(_T("gR_Strength"), CComVariant(lldf.gR[0]));
   //      pStrSave->put_Property(_T("gM_Fatigue"),  CComVariant(lldf.gM[1]));
   //      pStrSave->put_Property(_T("gR_Fatigue"),  CComVariant(lldf.gR[1]));
   //      pStrSave->EndUnit(); // LLDF_Girder
   //   }

   //   pStrSave->EndUnit(); // LLDF
   //}

   //// added in version 5 - RAB: 10/17/2008 - not linking any more
   //pStrSave->put_Property(_T("IsLinked"),CComVariant(VARIANT_FALSE));

   //pStrSave->EndUnit();

   return hr;
}

void CPierData::MakeCopy(const CPierData& rOther)
{
   m_PierIdx             = rOther.m_PierIdx;

   m_Station               = rOther.m_Station;
   Orientation             = rOther.Orientation;
   Angle                   = rOther.Angle;
   
   m_ConnectionType        = rOther.m_ConnectionType;

   m_strOrientation        = rOther.m_strOrientation;

   for ( int i = 0; i < 2; i++ )
   {
      m_GirderEndDistance[i]            = rOther.m_GirderEndDistance[i];
      m_EndDistanceMeasurementType[i]   = rOther.m_EndDistanceMeasurementType[i];
      m_GirderBearingOffset[i]          = rOther.m_GirderBearingOffset[i];
      m_BearingOffsetMeasurementType[i] = rOther.m_BearingOffsetMeasurementType[i];
      m_SupportWidth[i]                 = rOther.m_SupportWidth[i];

      m_DiaphragmHeight[i]       = rOther.m_DiaphragmHeight[i];       
      m_DiaphragmWidth[i]        = rOther.m_DiaphragmWidth[i];
      m_DiaphragmLoadType[i]     = rOther.m_DiaphragmLoadType[i];
      m_DiaphragmLoadLocation[i] = rOther.m_DiaphragmLoadLocation[i];
   }

   m_LLDFs = rOther.m_LLDFs;
   m_DistributionFactorsFromOlderVersion = rOther.m_DistributionFactorsFromOlderVersion;
}

void CPierData::MakeAssignment(const CPierData& rOther)
{
   MakeCopy( rOther );
}

void CPierData::SetPierIndex(PierIndexType pierIdx)
{
   m_PierIdx = pierIdx;
}

PierIndexType CPierData::GetPierIndex() const
{
   return m_PierIdx;
}

void CPierData::SetBridgeDescription(CBridgeDescription* pBridge)
{
   m_pBridgeDesc = pBridge;
}

const CBridgeDescription* CPierData::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

CBridgeDescription* CPierData::GetBridgeDescription()
{
   return m_pBridgeDesc;
}

void CPierData::SetSpans(CSpanData* pPrevSpan,CSpanData* pNextSpan)
{
   m_pPrevSpan = pPrevSpan;
   m_pNextSpan = pNextSpan;
}

CSpanData* CPierData::GetPrevSpan()
{
   return m_pPrevSpan;
}

CSpanData* CPierData::GetNextSpan()
{
   return m_pNextSpan;
}

CSpanData* CPierData::GetSpan(pgsTypes::PierFaceType face)
{
   return (face == pgsTypes::Ahead ? m_pNextSpan : m_pPrevSpan);
}

const CSpanData* CPierData::GetPrevSpan() const
{
   return m_pPrevSpan;
}

const CSpanData* CPierData::GetNextSpan() const
{
   return m_pNextSpan;
}

const CSpanData* CPierData::GetSpan(pgsTypes::PierFaceType face) const
{
   return (face == pgsTypes::Ahead ? m_pNextSpan : m_pPrevSpan);
}

Float64 CPierData::GetStation() const
{
   return m_Station;
}

void CPierData::SetStation(Float64 station)
{
   m_Station = station;
}

LPCTSTR CPierData::GetOrientation() const
{
   return m_strOrientation.c_str();
}

void CPierData::SetOrientation(LPCTSTR strOrientation)
{
   m_strOrientation = strOrientation;
}

pgsTypes::BoundaryConditionType CPierData::GetConnectionType() const
{
   return m_ConnectionType;
}

void CPierData::SetConnectionType(pgsTypes::BoundaryConditionType type)
{
   pgsTypes::BoundaryConditionType oldType = m_ConnectionType;
   m_ConnectionType = type;
}

void CPierData::SetGirderEndDistance(pgsTypes::PierFaceType face,Float64 endDist,ConnectionLibraryEntry::EndDistanceMeasurementType measure)
{
   m_GirderEndDistance[face] = endDist;
   m_EndDistanceMeasurementType[face] = measure;
}

void CPierData::GetGirderEndDistance(pgsTypes::PierFaceType face,Float64* pEndDist,ConnectionLibraryEntry::EndDistanceMeasurementType* pMeasure) const
{
   *pEndDist = m_GirderEndDistance[face];
   *pMeasure = m_EndDistanceMeasurementType[face];
}

void CPierData::SetBearingOffset(pgsTypes::PierFaceType face,Float64 offset,ConnectionLibraryEntry::BearingOffsetMeasurementType measure)
{
   m_GirderBearingOffset[face] = offset;
   m_BearingOffsetMeasurementType[face] = measure;
}

void CPierData::GetBearingOffset(pgsTypes::PierFaceType face,Float64* pOffset,ConnectionLibraryEntry::BearingOffsetMeasurementType* pMeasure) const
{
   *pOffset = m_GirderBearingOffset[face];
   *pMeasure = m_BearingOffsetMeasurementType[face];
}

void CPierData::SetSupportWidth(pgsTypes::PierFaceType face,Float64 w)
{
   m_SupportWidth[face] = w;
}

Float64 CPierData::GetSupportWidth(pgsTypes::PierFaceType face) const
{
   return m_SupportWidth[face];
}

void CPierData::SetDiaphragmHeight(pgsTypes::PierFaceType face,Float64 d)
{
   m_DiaphragmHeight[face] = d;
}

Float64 CPierData::GetDiaphragmHeight(pgsTypes::PierFaceType face) const
{
   return m_DiaphragmHeight[face];
}

void CPierData::SetDiaphragmWidth(pgsTypes::PierFaceType face,Float64 w)
{
   m_DiaphragmWidth[face] = w;
}

Float64 CPierData::GetDiaphragmWidth(pgsTypes::PierFaceType face)const
{
   return m_DiaphragmWidth[face];
}

ConnectionLibraryEntry::DiaphragmLoadType CPierData::GetDiaphragmLoadType(pgsTypes::PierFaceType face) const
{
   return m_DiaphragmLoadType[face];
}

void CPierData::SetDiaphragmLoadType(pgsTypes::PierFaceType face,ConnectionLibraryEntry::DiaphragmLoadType type)
{
   m_DiaphragmLoadType[face] = type;
   m_DiaphragmLoadLocation[face]=0.0;
}

Float64 CPierData::GetDiaphragmLoadLocation(pgsTypes::PierFaceType face) const
{
   return m_DiaphragmLoadLocation[face];
}

void CPierData::SetDiaphragmLoadLocation(pgsTypes::PierFaceType face,Float64 loc)
{
   m_DiaphragmLoadLocation[face] = loc;
}

Float64 CPierData::GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gM[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CPierData::SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls, Float64 gM)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gM[ls == pgsTypes::FatigueI ? 1 : 0] = gM;
}

void CPierData::SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls, Float64 gM)
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

Float64 CPierData::GetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gR[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CPierData::SetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls, Float64 gR)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gR[ls == pgsTypes::FatigueI ? 1 : 0] = gR;
}

void CPierData::SetLLDFReaction(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls, Float64 gM)
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

bool CPierData::IsContinuous() const
{
   return m_ConnectionType == pgsTypes::bctContinuousBeforeDeck || m_ConnectionType == pgsTypes::bctContinuousAfterDeck;
}

void CPierData::IsIntegral(bool* pbLeft,bool* pbRight) const
{
   if (m_ConnectionType == pgsTypes::bctIntegralBeforeDeck || m_ConnectionType == pgsTypes::bctIntegralAfterDeck)
   {
      *pbLeft  = true;
      *pbRight = true;
   }
   else
   {
      *pbLeft  = m_ConnectionType == pgsTypes::bctIntegralAfterDeckHingeAhead || m_ConnectionType == pgsTypes::bctIntegralBeforeDeckHingeAhead;

      *pbRight = m_ConnectionType == pgsTypes::bctIntegralAfterDeckHingeBack  || m_ConnectionType == pgsTypes::bctIntegralBeforeDeckHingeBack;
   }
}

CPierData::LLDF& CPierData::GetLLDF(GirderIndexType igs) const
{
   // First: Compare size of our collection with current number of girders and resize if they don't match
   GirderIndexType ngdrs = GetLldfGirderCount();
   ATLASSERT(ngdrs>0);

   GirderIndexType ndfs = m_LLDFs.size();

   if (m_DistributionFactorsFromOlderVersion)
   {
      // data loaded from older versions should be loaded into first two entries
      if(ndfs==2)
      {
         LLDF exterior = m_LLDFs[0];
         LLDF interior = m_LLDFs[1];
         for (GirderIndexType ig=2; ig<ngdrs; ig++)
         {
            if (ig!=ngdrs-1)
            {
               m_LLDFs.push_back(interior);
            }
            else
            {
               m_LLDFs.push_back(exterior);
            }
         }

         m_DistributionFactorsFromOlderVersion = false;
         ndfs = ngdrs;
      }
      else
      {
         ATLASSERT(false); // something went wrong on load
      }
   }

   if (ndfs==0)
   {
      for (GirderIndexType i=0; i<ngdrs; i++)
      {
         m_LLDFs.push_back(LLDF());
      }
   }
   else if (ndfs<ngdrs)
   {
      // More girders than factors - move exterior to last girder and use last interior for new interiors
      LLDF exterior = m_LLDFs.back();
      GirderIndexType inter_idx = ndfs-2>0 ? ndfs-2 : 0; // one-girder bridges could otherwise give us trouble
      LLDF interior = m_LLDFs[inter_idx];

      m_LLDFs[ndfs-1] = interior;
      for (GirderIndexType i=ndfs; i<ngdrs; i++)
      {
         if (i != ngdrs-1)
         {
            m_LLDFs.push_back(interior);
         }
         else
         {
            m_LLDFs.push_back(exterior);
         }
      }
    }
   else if (ndfs>ngdrs)
   {
      // more factors than girders - truncate, then move last exterior to end
      LLDF exterior = m_LLDFs.back();
      m_LLDFs.resize(ngdrs);
      m_LLDFs.back() = exterior;
   }

   // Next: let's deal with retrieval
   if (igs<0)
   {
      ATLASSERT(false); // problemo in calling routine - let's not crash
      return m_LLDFs[0];
   }
   else if (igs>=ngdrs)
   {
      ATLASSERT(false); // problemo in calling routine - let's not crash
      return m_LLDFs.back();
   }
   else
   {
      return m_LLDFs[igs];
   }
}

GirderIndexType CPierData::GetLldfGirderCount() const
{
   GirderIndexType ahead(0), back(0);

   const CSpanData* pAhead = GetSpan(pgsTypes::Ahead);
   if (pAhead!=nullptr)
      ahead = pAhead->GetGirderCount();

   const CSpanData* pBack = GetSpan(pgsTypes::Back);
   if (pBack!=nullptr)
      back = pBack->GetGirderCount();

   if (pBack==nullptr && pAhead==nullptr)
   {
      ATLASSERT(false); // function called before bridge tied together - no good
      return 0;
   }
   else
   {
      return Max(ahead, back);
   }
}

bool CPierData::IsAbutment() const
{
   return (m_pPrevSpan == nullptr || m_pNextSpan == nullptr) ? true : false;
}

bool CPierData::IsPier() const
{
   return (m_pPrevSpan != nullptr && m_pNextSpan != nullptr) ? true : false;
}
