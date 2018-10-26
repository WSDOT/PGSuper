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
#include <PgsExt\PierData.h>
#include <PgsExt\SpanData.h>
#include <PgsExt\BridgeDescription.h>

#include <Units\SysUnits.h>
#include <StdIo.h>
#include <StrData.cpp>
#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPierData
****************************************************************************/


// old data storage format
BEGIN_STRSTORAGEMAP(CPierData,"PierData",1.0)
   PROPERTY("Station",     SDT_R8, m_Station )
   PROPERTY("Orientation", SDT_I4, Orientation )
   PROPERTY("Angle",       SDT_R8, Angle )
   PROPERTY("Connection",  SDT_STDSTRING, m_Connection[pgsTypes::Back] )
END_STRSTORAGEMAP

CPierData::CPierData()
{
   m_PierIdx = INVALID_INDEX;
   m_pPrevSpan = NULL;
   m_pNextSpan = NULL;

   m_pBridgeDesc = NULL;

   m_Station     = 0.0;
   Orientation = Normal;
   Angle       = 0.0;

   m_ConnectionType                    = pgsTypes::Hinged;
   m_Connection[pgsTypes::Back]        = "";
   m_Connection[pgsTypes::Ahead]       = "";
   m_pConnectionEntry[pgsTypes::Back]  = 0;
   m_pConnectionEntry[pgsTypes::Ahead] = 0;

   m_strOrientation = "Normal";

   // strength/service limit states
   m_gM[pgsTypes::Interior][0] = 1.0;
   m_gR[pgsTypes::Interior][0] = 1.0;

   m_gM[pgsTypes::Exterior][0] = 1.0;
   m_gR[pgsTypes::Exterior][0] = 1.0;

   // fatigue limit states
   m_gM[pgsTypes::Interior][1] = 1.0;
   m_gR[pgsTypes::Interior][1] = 1.0;

   m_gM[pgsTypes::Exterior][1] = 1.0;
   m_gR[pgsTypes::Exterior][1] = 1.0;
}

CPierData::CPierData(const CPierData& rOther)
{
   m_pPrevSpan = NULL;
   m_pNextSpan = NULL;

   m_pBridgeDesc = NULL;

   m_Station   = 0.0;
   Orientation = Normal;
   Angle       = 0.0;

   m_Connection[pgsTypes::Back]   = "";
   m_Connection[pgsTypes::Ahead]  = "";
   m_pConnectionEntry[pgsTypes::Back]  = 0;
   m_pConnectionEntry[pgsTypes::Ahead] = 0;
   
   m_strOrientation = "Normal";

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

   if ( m_Connection[pgsTypes::Back] != rOther.m_Connection[pgsTypes::Back] )
      return false;

   if ( m_Connection[pgsTypes::Ahead] != rOther.m_Connection[pgsTypes::Ahead] )
      return false;

   if ( m_ConnectionType != rOther.m_ConnectionType )
      return false;

   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      for ( int i = 0; i < 2; i++ )
      {
         if ( !IsEqual(m_gM[pgsTypes::Interior][i],rOther.m_gM[pgsTypes::Interior][i]) )
            return false;

         if ( !IsEqual(m_gM[pgsTypes::Exterior][i],rOther.m_gM[pgsTypes::Exterior][i]) )
            return false;

         if ( !IsEqual(m_gR[pgsTypes::Interior][i],rOther.m_gR[pgsTypes::Interior][i]) )
            return false;

         if ( !IsEqual(m_gR[pgsTypes::Exterior][i],rOther.m_gR[pgsTypes::Exterior][i]) )
            return false;
      }
   }

   return true;
}

bool CPierData::operator!=(const CPierData& rOther) const
{
   return !operator==(rOther);
}

const char* CPierData::AsString(pgsTypes::PierConnectionType type)
{
   switch(type)
   { 
   case pgsTypes::Hinged:
      return "Hinged";

   case pgsTypes::Roller:
      return "Roller";

   case pgsTypes::ContinuousAfterDeck:
      return "Continuous after deck placement";

   case pgsTypes::ContinuousBeforeDeck:
      return "Continuous before deck placement";

   case pgsTypes::IntegralAfterDeck:
      return "Integral after deck placement";

   case pgsTypes::IntegralBeforeDeck:
      return "Integral before deck placement";

   case pgsTypes::IntegralAfterDeckHingeBack:
      return "Hinged on back side; Integral on ahead side after deck placement";

   case pgsTypes::IntegralBeforeDeckHingeBack:
      return "Hinged on back side; Integral on ahead side before deck placement";

   case pgsTypes::IntegralAfterDeckHingeAhead:
      return "Integral on back side after deck placement; Hinged on ahead side";

   case pgsTypes::IntegralBeforeDeckHingeAhead:
      return "Integral on back side before deck placement; Hinged on ahead side";
   
   default:
      ATLASSERT(0);

   };

   return "";
}

HRESULT CPierData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;
   HRESULT hr = S_OK;

   HRESULT hr2 = pStrLoad->BeginUnit("PierDataDetails");

   double version;
   pStrLoad->get_Version(&version);
   if ( version == 1.0 )
   {
      STRSTG_LOAD( hr, pStrLoad, pProgress );

      m_Connection[pgsTypes::Ahead] = m_Connection[pgsTypes::Back];
      m_ConnectionType  = pgsTypes::Hinged;

      // Convert old input into a bearing string
      CComPtr<IDirectionDisplayUnitFormatter> dirFormatter;
      CComPtr<IAngleDisplayUnitFormatter> angleFormatter;
      CComBSTR bstrOrientation;
      switch(Orientation)
      {
      case Normal:
         m_strOrientation = "Normal";
         break;

      case Skew:
         angleFormatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
         angleFormatter->put_CondensedFormat(VARIANT_TRUE);
         angleFormatter->put_Signed(VARIANT_FALSE);

         angleFormatter->Format(Angle,NULL,&bstrOrientation);
         m_strOrientation = OLE2A(bstrOrientation);
         break;
      
      case Bearing:
         dirFormatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
         dirFormatter->put_CondensedFormat(VARIANT_TRUE);
         dirFormatter->put_BearingFormat(VARIANT_TRUE);

         dirFormatter->Format(Angle,NULL,&bstrOrientation);
         m_strOrientation = OLE2A(bstrOrientation);
         break;

      default:
         ATLASSERT(false);
         return STRLOAD_E_INVALIDFORMAT;
      }

      if ( FAILED(hr2) )
      {
         // Failed to read "PierDataDetails" block. This means the file was
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
   }
   else
   {
      // greater than version 1
      CComVariant var;
      var.vt = VT_R8;
      if ( FAILED(pStrLoad->get_Property("Station",&var)) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         m_Station = var.dblVal;

#pragma Reminder("UPDATE: Need to validate string... make sure it is a valid pier orientation")
      var.Clear();
      var.vt = VT_BSTR;
      if (FAILED(pStrLoad->get_Property("Orientation", &var )) )
         return STRLOAD_E_INVALIDFORMAT;
      else
         m_strOrientation = OLE2A(var.bstrVal);

      if ( 4.0 <= version )
      {
         var.Clear();

         if ( 6.0 <= version && version < 8 )
         {
            var.vt = VT_R8;
            if ( FAILED(pStrLoad->get_Property("AlignmentOffset",&var)))
               return STRLOAD_E_INVALIDFORMAT;
//            else // removed in version 8
//               m_AlignmentOffset = var.dblVal;
         }

         // prior to version 7 we had left and right boundary conditions with an option to make both same

         bool use_same_both = false;
         pgsTypes::PierConnectionType back_conn_type, ahead_conn_type;
         if ( 5.0 <= version && version < 7.0 )
         {
            var.vt = VT_BOOL;
            if (FAILED(pStrLoad->get_Property("UseSameConnectionOnBothSides",&var)))
               return STRLOAD_E_INVALIDFORMAT;
            else
               use_same_both = (var.boolVal == VARIANT_TRUE);
         }

         var.vt = VT_BSTR;
         if (FAILED(pStrLoad->get_Property("LeftConnection", &var )) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_Connection[pgsTypes::Back] = OLE2A(var.bstrVal);

         if ( version < 7.0 )
         {
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property("LeftConnectionType",&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               back_conn_type = (pgsTypes::PierConnectionType)var.lVal;
         }

         var.Clear();
         var.vt = VT_BSTR;
         if (FAILED(pStrLoad->get_Property("RightConnection", &var )) )
            return STRLOAD_E_INVALIDFORMAT;
         else
         {
            if (!use_same_both)
            {
               m_Connection[pgsTypes::Ahead] = OLE2A(var.bstrVal);
            }
            else
            {
               m_Connection[pgsTypes::Ahead] = m_Connection[pgsTypes::Back]; 
            }
         }               

         if ( 7.0 <= version )
         {
            // After version 7.0, we have single definition for boundary conditions over pier
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property("ConnectionType",&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               m_ConnectionType = (pgsTypes::PierConnectionType)var.lVal;
         }
         else
         {
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property("RightConnectionType",&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               ahead_conn_type = (pgsTypes::PierConnectionType)var.lVal;

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
               else if (back_conn_type == pgsTypes::ContinuousAfterDeck || ahead_conn_type == pgsTypes::ContinuousAfterDeck)
               {
                  m_ConnectionType = pgsTypes::ContinuousAfterDeck;
               }
               else if (back_conn_type == pgsTypes::ContinuousBeforeDeck || ahead_conn_type == pgsTypes::ContinuousBeforeDeck)
               {
                  m_ConnectionType = pgsTypes::ContinuousBeforeDeck;
               }
               else if (back_conn_type == pgsTypes::IntegralAfterDeck)
               {
                  if (ahead_conn_type == pgsTypes::Hinged || ahead_conn_type == pgsTypes::Roller)
                  {
                     m_ConnectionType = pgsTypes::IntegralAfterDeckHingeAhead;
                  }
               }
               else if (back_conn_type == pgsTypes::IntegralBeforeDeck)
               {
                  if (ahead_conn_type == pgsTypes::Hinged || ahead_conn_type == pgsTypes::Roller)
                  {
                     m_ConnectionType = pgsTypes::IntegralBeforeDeckHingeAhead;
                  }
               }
               else if (ahead_conn_type == pgsTypes::IntegralAfterDeck)
               {
                  if (back_conn_type == pgsTypes::Hinged || back_conn_type == pgsTypes::Roller)
                  {
                     m_ConnectionType = pgsTypes::IntegralAfterDeckHingeBack;
                  }
               }
               else if (ahead_conn_type == pgsTypes::IntegralBeforeDeck)
               {
                  if (back_conn_type == pgsTypes::Hinged || back_conn_type == pgsTypes::Roller)
                  {
                     m_ConnectionType = pgsTypes::IntegralBeforeDeckHingeBack;
                  }
               }
            }
         }

      }
      else
      {
         var.Clear();
         var.vt = VT_BSTR;
         if (FAILED(pStrLoad->get_Property("Connection", &var )) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            m_Connection[pgsTypes::Back] = OLE2A(var.bstrVal);

         m_Connection[pgsTypes::Ahead] = m_Connection[pgsTypes::Back];

         if ( 3.0 <= version )
         {
            var.Clear();
            var.vt = VT_I4;
            if ( FAILED(pStrLoad->get_Property("ConnectionType",&var)) )
               return STRLOAD_E_INVALIDFORMAT;
            else
               m_ConnectionType = (pgsTypes::PierConnectionType)var.lVal;
         }
      }

      if ( 5.0 <= version )
      {
         // LLDF stuff added in version 5
         if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
         {
            pStrLoad->BeginUnit("LLDF");
            double lldf_version;
            pStrLoad->get_Version(&lldf_version);

            if ( lldf_version < 2 )
            {
               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gM_Interior",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gM[pgsTypes::Interior][0] = var.dblVal;
               m_gM[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gM_Exterior",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gM[pgsTypes::Exterior][0] = var.dblVal;
               m_gM[pgsTypes::Exterior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gR_Interior",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gR[pgsTypes::Interior][0] = var.dblVal;
               m_gR[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gR_Exterior",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gR[pgsTypes::Exterior][0] = var.dblVal;
               m_gR[pgsTypes::Exterior][1] = var.dblVal;
            }
            else
            {
               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gM_Interior_Strength",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gM[pgsTypes::Interior][0] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gM_Exterior_Strength",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gM[pgsTypes::Exterior][0] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gR_Interior_Strength",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gR[pgsTypes::Interior][0] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gR_Exterior_Strength",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gR[pgsTypes::Exterior][0] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gM_Interior_Fatigue",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gM[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gM_Exterior_Fatigue",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gM[pgsTypes::Exterior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gR_Interior_Fatigue",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gR[pgsTypes::Interior][1] = var.dblVal;

               var.vt = VT_R8;
               if ( FAILED(pStrLoad->get_Property("gR_Exterior_Fatigue",&var)) )
                  return STRLOAD_E_INVALIDFORMAT;

               m_gR[pgsTypes::Exterior][1] = var.dblVal;
            }

            pStrLoad->EndUnit();
         }

         // link stuff added in version 5
         // RAB: 10/17/2008 - got rid of the link stuff... still need to read this attribute
         var.Clear();
         var.vt = VT_BOOL;
         bool bIsLinked;
         if ( FAILED(pStrLoad->get_Property("IsLinked",&var)) )
            return STRLOAD_E_INVALIDFORMAT;
         else
            bIsLinked = (var.boolVal == VARIANT_TRUE ? true : false);

      }
   }

   if ( SUCCEEDED(hr2) )
      pStrLoad->EndUnit();

   return hr;
}

HRESULT CPierData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit("PierDataDetails",8.0);

   pStrSave->put_Property("Station",         CComVariant(m_Station) );
   pStrSave->put_Property("Orientation",     CComVariant( CComBSTR(m_strOrientation.c_str()) ) );
   //pStrSave->put_Property("AlignmentOffset", CComVariant( m_AlignmentOffset) ); // added in version 6, removed in version 8
   pStrSave->put_Property("LeftConnection",  CComVariant( CComBSTR(m_Connection[pgsTypes::Back].c_str()) ) );
   pStrSave->put_Property("RightConnection", CComVariant( CComBSTR(m_Connection[pgsTypes::Ahead].c_str()) ) );
   pStrSave->put_Property("ConnectionType",  CComVariant( m_ConnectionType ) ); // changed from left and right to a single value in version 7

   // added in version 5
   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pStrSave->BeginUnit("LLDF",2.0);
      pStrSave->put_Property("gM_Interior_Strength",CComVariant(m_gM[pgsTypes::Interior][0]));
      pStrSave->put_Property("gM_Exterior_Strength",CComVariant(m_gM[pgsTypes::Exterior][0]));
      pStrSave->put_Property("gR_Interior_Strength",CComVariant(m_gR[pgsTypes::Interior][0]));
      pStrSave->put_Property("gR_Exterior_Strength",CComVariant(m_gR[pgsTypes::Exterior][0]));

      pStrSave->put_Property("gM_Interior_Fatigue",CComVariant(m_gM[pgsTypes::Interior][1]));
      pStrSave->put_Property("gM_Exterior_Fatigue",CComVariant(m_gM[pgsTypes::Exterior][1]));
      pStrSave->put_Property("gR_Interior_Fatigue",CComVariant(m_gR[pgsTypes::Interior][1]));
      pStrSave->put_Property("gR_Exterior_Fatigue",CComVariant(m_gR[pgsTypes::Exterior][1]));
      pStrSave->EndUnit();
   }

   // added in version 5 - RAB: 10/17/2008 - not linking any more
   pStrSave->put_Property("IsLinked",CComVariant(VARIANT_FALSE));

   pStrSave->EndUnit();

   return hr;
}

void CPierData::MakeCopy(const CPierData& rOther)
{
   m_PierIdx             = rOther.m_PierIdx;

   m_Station               = rOther.m_Station;
   Orientation             = rOther.Orientation;
   Angle                   = rOther.Angle;
   
   m_Connection[pgsTypes::Back]        = rOther.m_Connection[pgsTypes::Back];
   m_pConnectionEntry[pgsTypes::Back]  = rOther.m_pConnectionEntry[pgsTypes::Back];

   m_Connection[pgsTypes::Ahead]       = rOther.m_Connection[pgsTypes::Ahead];
   m_pConnectionEntry[pgsTypes::Ahead] = rOther.m_pConnectionEntry[pgsTypes::Ahead];
   
   m_ConnectionType                    = rOther.m_ConnectionType;

   m_strOrientation        = rOther.m_strOrientation;

   for ( int i = 0; i < 2; i++ )
   {
      m_gM[pgsTypes::Interior][i] = rOther.m_gM[pgsTypes::Interior][i];
      m_gR[pgsTypes::Interior][i] = rOther.m_gR[pgsTypes::Interior][i];

      m_gM[pgsTypes::Exterior][i] = rOther.m_gM[pgsTypes::Exterior][i];
      m_gR[pgsTypes::Exterior][i] = rOther.m_gR[pgsTypes::Exterior][i];
   }
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

void CPierData::SetBridgeDescription(const CBridgeDescription* pBridge)
{
   m_pBridgeDesc = pBridge;
}

const CBridgeDescription* CPierData::GetBridgeDescription() const
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

double CPierData::GetStation() const
{
   return m_Station;
}

void CPierData::SetStation(double station)
{
   m_Station = station;
}

const char* CPierData::GetOrientation() const
{
   return m_strOrientation.c_str();
}

void CPierData::SetOrientation(const char* strOrientation)
{
   m_strOrientation = strOrientation;
}

pgsTypes::PierConnectionType CPierData::GetConnectionType() const
{
   return m_ConnectionType;
}

void CPierData::SetConnectionType(pgsTypes::PierConnectionType type)
{
   m_ConnectionType = type;
}

const char* CPierData::GetConnection(pgsTypes::PierFaceType pierFace) const
{
   return m_Connection[pierFace].c_str();
}

void CPierData::SetConnection(pgsTypes::PierFaceType pierFace,const char* strConnection)
{
   m_Connection[pierFace] = strConnection;
}

const ConnectionLibraryEntry* CPierData::GetConnectionLibraryEntry(pgsTypes::PierFaceType pierFace) const
{
   return m_pConnectionEntry[pierFace];
}

void CPierData::SetConnectionLibraryEntry(pgsTypes::PierFaceType pierFace,const ConnectionLibraryEntry* pLibEntry)
{
   m_pConnectionEntry[pierFace] = pLibEntry;
}

double CPierData::GetLLDFNegMoment(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc) const
{
   return m_gM[loc][ls == pgsTypes::FatigueI ? 1 : 0];
}

void CPierData::SetLLDFNegMoment(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc,double gM)
{
   m_gM[loc][ls == pgsTypes::FatigueI ? 1 : 0] = gM;
}

double CPierData::GetLLDFReaction(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc) const
{
   return m_gR[loc][ls == pgsTypes::FatigueI ? 1 : 0];
}

void CPierData::SetLLDFReaction(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc,double gR)
{
   m_gR[loc][ls == pgsTypes::FatigueI ? 1 : 0] = gR;
}

bool CPierData::IsContinuous() const
{
   return m_ConnectionType == pgsTypes::ContinuousBeforeDeck || m_ConnectionType == pgsTypes::ContinuousAfterDeck;
}

void CPierData::IsIntegral(bool* pbLeft,bool* pbRight) const
{
   if (m_ConnectionType == pgsTypes::IntegralBeforeDeck || m_ConnectionType == pgsTypes::IntegralAfterDeck)
   {
      *pbLeft  = true;
      *pbRight = true;
   }
   else
   {
      *pbLeft  = m_ConnectionType == pgsTypes::IntegralAfterDeckHingeAhead || m_ConnectionType == pgsTypes::IntegralBeforeDeckHingeAhead;

      *pbRight = m_ConnectionType == pgsTypes::IntegralAfterDeckHingeBack  || m_ConnectionType == pgsTypes::IntegralBeforeDeckHingeBack;
   }
}

