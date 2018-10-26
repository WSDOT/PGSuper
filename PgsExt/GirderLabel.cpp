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
#include <PgsExt\TemporarySupportData.h>
#include <EAF\EAFDisplayUnits.h>

// Includes for ConcreteDescription
#include <PgsExt\GirderLabel.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool pgsGirderLabel::ms_bUseAlpha = true;

std::_tstring pgsGirderLabel::GetGirderLabel(GirderIndexType gdrIdx)
{
   std::_tstring strLabel;
   if ( ms_bUseAlpha )
   {
      strLabel += ((TCHAR)((gdrIdx % 26) + _T('A')));
      gdrIdx = ((gdrIdx - (gdrIdx % 26))/26);
      if ( 0 < gdrIdx )
      {
         std::_tstring strTemp = strLabel;
         strLabel = pgsGirderLabel::GetGirderLabel(gdrIdx-1);
         strLabel += strTemp;
      }
   }
   else
   {
      std::_tostringstream os;
      os << (long)(gdrIdx+1L);
      strLabel = os.str();
   }

   return strLabel;
}

bool pgsGirderLabel::UseAlphaLabel()
{
   return ms_bUseAlpha;
}

bool pgsGirderLabel::UseAlphaLabel(bool bUseAlpha)
{
   bool bOldValue = ms_bUseAlpha;
   ms_bUseAlpha = bUseAlpha;
   return bOldValue;
}

pgsGirderLabel::pgsGirderLabel(void)
{
}

pgsGirderLabel::~pgsGirderLabel(void)
{
}


LPCTSTR GetEndDistanceMeasureString(ConnectionLibraryEntry::EndDistanceMeasurementType type,bool bAbutment,bool bAbbreviation)
{
   switch( type )
   {
   case ConnectionLibraryEntry::FromBearingAlongGirder:
      if (bAbbreviation)
      {
         return _T("FCLB-AG");
      }
      else
      {
         return _T("Measured From CL Bearing along Girder Centerline");
      }

   case ConnectionLibraryEntry::FromBearingNormalToPier:
      if ( bAbbreviation )
      {
         return bAbutment ? _T("FCLB-NA") : _T("FCLB-NP");
      }
      else
      {
         return bAbutment ? _T("Measured From CL Bearing and Normal to Abutment Line") : _T("Measured From CL Bearing and Normal to Pier Line");
      }

   case ConnectionLibraryEntry::FromPierAlongGirder:
      if ( bAbbreviation )
      {
         return bAbutment ? _T("FA-AG") : _T("FP-AG");
      }
      else
      {
         return bAbutment ? _T("Measured From Abutment Line and Along Girder Centerline") : _T("Measured From Pier Line and Along Girder Centerline");
      }

   case ConnectionLibraryEntry::FromPierNormalToPier:
      if ( bAbbreviation )
      {
         return bAbutment ? _T("FA-NA") : _T("FP-NP");
      }
      else
      {
         return bAbutment ? _T("Measured From and Normal to Abutment Line") : _T("Measured From and Normal to Pier Line");
      }

   default:
      ATLASSERT(false);
      return _T("");
   }
}

LPCTSTR GetBearingOffsetMeasureString(ConnectionLibraryEntry::BearingOffsetMeasurementType type,bool bAbutment,bool bAbbreviation)
{
   switch( type )
   {
   case ConnectionLibraryEntry::AlongGirder:
      if ( bAbbreviation )
      {
         return bAbutment ? _T("FA-AG") : _T("FP-AG");
      }
      else
      {
         return bAbutment ? _T("Measured From Abutment Line and Along Girder Centerline") :  _T("Measured From Pier Line and Along Girder Centerline");
      }

   case ConnectionLibraryEntry::NormalToPier:
      if ( bAbbreviation )
      {
         return bAbutment ? _T("FA-NA") : _T("FP-NP");
      }
      else
      {
         return bAbutment ? _T("Measured From and Normal to Abutment Line") : _T("Measured From and Normal to Pier Line");
      }

   default:
      ATLASSERT(false);
      return _T("");
   }
}

CString GetLabel(const CPierData2* pPier,IEAFDisplayUnits* pDisplayUnits)
{
   CString strLabel;
   strLabel.Format(_T("Pier %d, %s"),LABEL_PIER(pPier->GetIndex()),
                                     FormatStation(pDisplayUnits->GetStationFormat(),pPier->GetStation()));

   return strLabel;
}

CString GetLabel(const CTemporarySupportData* pTS,IEAFDisplayUnits* pDisplayUnits)
{
   CString strLabel;
   strLabel.Format(_T("TS %d, %s, %s"),LABEL_TEMPORARY_SUPPORT(pTS->GetIndex()),
                                       pTS->GetSupportType() == pgsTypes::ErectionTower ? _T("Erection Tower") : _T("Strong Back"),
                                       FormatStation(pDisplayUnits->GetStationFormat(),pTS->GetStation()));

   return strLabel;
}

CString ConcreteDescription(const CConcreteMaterial& concrete)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   CString strLabel;
   if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      if ( pLossParameters->GetTimeDependentModel() == pgsTypes::tdmAASHTO || 
           pLossParameters->GetTimeDependentModel() == pgsTypes::tdmACI209 )
      {
         if ( concrete.bACIUserParameters )
         {
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

            strLabel.Format(_T("%s, ACI 209R-92, A=%s, B=%4.2f"),
               matConcrete::GetTypeName((matConcrete::Type)concrete.Type,true).c_str(),
               ::FormatDimension(concrete.A,pDisplayUnits->GetWholeDaysUnit()),
               concrete.B);
         }
         else
         {
            strLabel.Format(_T("%s, ACI 209R-92, %s cured, %s cement"),
               matConcrete::GetTypeName((matConcrete::Type)concrete.Type,true).c_str(),
               concrete.CureMethod == pgsTypes::Steam ? _T("Steam") : _T("Moist"),
               concrete.ACI209CementType == pgsTypes::TypeI ? _T("Type I") : _T("Type III"));
         }
      }
      else
      {
         strLabel.Format(_T("%s, CEB-FIP, Type %s cement"),
               matConcrete::GetTypeName((matConcrete::Type)concrete.Type,true).c_str(),
               matCEBFIPConcrete::GetCementType((matCEBFIPConcrete::CementType)concrete.CEBFIPCementType));
      }
   }
   else
   {
      strLabel.Format(_T("%s"),matConcrete::GetTypeName((matConcrete::Type)concrete.Type,true).c_str());
   }

   return strLabel;
}