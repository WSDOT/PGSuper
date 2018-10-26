///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <PgsExt\GirderLabel.h>

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


std::_tstring GetEndDistanceMeasureString(ConnectionLibraryEntry::EndDistanceMeasurementType type,bool bAbutment)
{
   switch( type )
   {
   case ConnectionLibraryEntry::FromBearingAlongGirder:
      return _T("Measured From CL Bearing along Girder Centerline");

   case ConnectionLibraryEntry::FromBearingNormalToPier:
      return bAbutment ? _T("Measured From CL Bearing and Normal to Abutment Line") : _T("Measured From CL Bearing and Normal to Pier Line");

   case ConnectionLibraryEntry::FromPierAlongGirder:
      return bAbutment ? _T("Measured From Abutment Line and Along Girder Centerline") : _T("Measured From Pier Line and Along Girder Centerline");

   case ConnectionLibraryEntry::FromPierNormalToPier:
      return bAbutment ? _T("Measured From and Normal to Abutment Line") : _T("Measured From and Normal to Pier Line");

   default:
      ATLASSERT(0);
      return _T("");
   }
}

std::_tstring GetBearingOffsetMeasureString(ConnectionLibraryEntry::BearingOffsetMeasurementType type,bool bAbutment)
{
   switch( type )
   {
   case ConnectionLibraryEntry::AlongGirder:
      return bAbutment ? _T("Measured From Abutment Line and Along Girder Centerline") :  _T("Measured From Pier Line and Along Girder Centerline");

   case ConnectionLibraryEntry::NormalToPier:
      return bAbutment ? _T("Measured From and Normal to Abutment Line") : _T("Measured From and Normal to Pier Line");

   default:
      ATLASSERT(0);
      return _T("");
   }
}
