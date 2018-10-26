///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\LoadFactors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLoadFactors
****************************************************************************/


CLoadFactors::CLoadFactors()
{
   DCmin[pgsTypes::ServiceI]   = 1.0;       DCmax[pgsTypes::ServiceI]   = 1.0;
   DWmin[pgsTypes::ServiceI]   = 1.0;       DWmax[pgsTypes::ServiceI]   = 1.0;
   CRmin[pgsTypes::ServiceI]   = 1.0;       CRmax[pgsTypes::ServiceI]   = 1.0;
   SHmin[pgsTypes::ServiceI]   = 1.0;       SHmax[pgsTypes::ServiceI]   = 1.0;
   REmin[pgsTypes::ServiceI]   = 1.0;       REmax[pgsTypes::ServiceI]   = 1.0;
   PSmin[pgsTypes::ServiceI]   = 1.0;       PSmax[pgsTypes::ServiceI]   = 1.0;
   LLIMmin[pgsTypes::ServiceI] = 1.0;       LLIMmax[pgsTypes::ServiceI] = 1.0;

   DCmin[pgsTypes::ServiceIA]   = 0.5;      DCmax[pgsTypes::ServiceIA]   = 0.5;
   DWmin[pgsTypes::ServiceIA]   = 0.5;      DWmax[pgsTypes::ServiceIA]   = 0.5;
   CRmin[pgsTypes::ServiceIA]   = 0.0;      CRmax[pgsTypes::ServiceIA]   = 0.0;
   SHmin[pgsTypes::ServiceIA]   = 0.0;      SHmax[pgsTypes::ServiceIA]   = 0.0;
   REmin[pgsTypes::ServiceIA]   = 0.0;      REmax[pgsTypes::ServiceIA]   = 0.0;
   PSmin[pgsTypes::ServiceIA]   = 0.0;      PSmax[pgsTypes::ServiceIA]   = 0.0;
   LLIMmin[pgsTypes::ServiceIA] = 1.0;      LLIMmax[pgsTypes::ServiceIA] = 1.0;

   DCmin[pgsTypes::ServiceIII]   = 1.0;     DCmax[pgsTypes::ServiceIII]   = 1.0;
   DWmin[pgsTypes::ServiceIII]   = 1.0;     DWmax[pgsTypes::ServiceIII]   = 1.0;
   CRmin[pgsTypes::ServiceIII]   = 1.0;     CRmax[pgsTypes::ServiceIII]   = 1.0;
   SHmin[pgsTypes::ServiceIII]   = 1.0;     SHmax[pgsTypes::ServiceIII]   = 1.0;
   REmin[pgsTypes::ServiceIII]   = 1.0;     REmax[pgsTypes::ServiceIII]   = 1.0;
   PSmin[pgsTypes::ServiceIII]   = 1.0;     PSmax[pgsTypes::ServiceIII]   = 1.0;
   LLIMmin[pgsTypes::ServiceIII] = 0.8;     LLIMmax[pgsTypes::ServiceIII] = 0.8;

   DCmin[pgsTypes::StrengthI]   = 0.90;     DCmax[pgsTypes::StrengthI]   = 1.25;
   DWmin[pgsTypes::StrengthI]   = 0.65;     DWmax[pgsTypes::StrengthI]   = 1.50;
   CRmin[pgsTypes::StrengthI]   = 1.00;     CRmax[pgsTypes::StrengthI]   = 1.00;
   SHmin[pgsTypes::StrengthI]   = 1.00;     SHmax[pgsTypes::StrengthI]   = 1.00;
   REmin[pgsTypes::StrengthI]   = 1.00;     REmax[pgsTypes::StrengthI]   = 1.00;
   PSmin[pgsTypes::StrengthI]   = 1.00;     PSmax[pgsTypes::StrengthI]   = 1.00;
   LLIMmin[pgsTypes::StrengthI] = 1.75;     LLIMmax[pgsTypes::StrengthI] = 1.75;

   DCmin[pgsTypes::StrengthII]   = 0.90;    DCmax[pgsTypes::StrengthII]   = 1.25;
   DWmin[pgsTypes::StrengthII]   = 0.65;    DWmax[pgsTypes::StrengthII]   = 1.50;
   CRmin[pgsTypes::StrengthII]   = 1.00;    CRmax[pgsTypes::StrengthII]   = 1.00;
   SHmin[pgsTypes::StrengthII]   = 1.00;    SHmax[pgsTypes::StrengthII]   = 1.00;
   REmin[pgsTypes::StrengthII]   = 1.00;    REmax[pgsTypes::StrengthII]   = 1.00;
   PSmin[pgsTypes::StrengthII]   = 1.00;    PSmax[pgsTypes::StrengthII]   = 1.00;
   LLIMmin[pgsTypes::StrengthII] = 1.35;    LLIMmax[pgsTypes::StrengthII] = 1.35;

   DCmin[pgsTypes::FatigueI]   = 0.5;      DCmax[pgsTypes::FatigueI]   = 0.5;
   DWmin[pgsTypes::FatigueI]   = 0.5;      DWmax[pgsTypes::FatigueI]   = 0.5;
   CRmin[pgsTypes::FatigueI]   = 0.0;      CRmax[pgsTypes::FatigueI]   = 0.0;
   SHmin[pgsTypes::FatigueI]   = 0.0;      SHmax[pgsTypes::FatigueI]   = 0.0;
   REmin[pgsTypes::FatigueI]   = 0.0;      REmax[pgsTypes::FatigueI]   = 0.0;
   PSmin[pgsTypes::FatigueI]   = 0.0;      PSmax[pgsTypes::FatigueI]   = 0.0;
   LLIMmin[pgsTypes::FatigueI] = 1.5;      LLIMmax[pgsTypes::FatigueI] = 1.5;
}

CLoadFactors::CLoadFactors(const CLoadFactors& rOther)
{
   MakeCopy(rOther);
}

CLoadFactors& CLoadFactors::operator=(const CLoadFactors& rOther)
{
   MakeAssignment(rOther);
   return *this;
}

bool CLoadFactors::operator==(const CLoadFactors& rOther) const
{
   for ( int i = 0; i < 6; i++ )
   {
      if ( DCmin[i] != rOther.DCmin[i] )
      {
         return false;
      }

      if ( DWmin[i] != rOther.DWmin[i] )
      {
         return false;
      }

      if ( CRmin[i] != rOther.CRmin[i] )
      {
         return false;
      }

      if ( SHmin[i] != rOther.SHmin[i] )
      {
         return false;
      }

      if ( REmin[i] != rOther.REmin[i] )
      {
         return false;
      }

      if ( PSmin[i] != rOther.PSmin[i] )
      {
         return false;
      }

      if ( LLIMmin[i] != rOther.LLIMmin[i] )
      {
         return false;
      }

      if ( DCmax[i] != rOther.DCmax[i] )
      {
         return false;
      }

      if ( DWmax[i] != rOther.DWmax[i] )
      {
         return false;
      }

      if ( CRmax[i] != rOther.CRmax[i] )
      {
         return false;
      }

      if ( SHmax[i] != rOther.SHmax[i] )
      {
         return false;
      }

      if ( REmax[i] != rOther.REmax[i] )
      {
         return false;
      }

      if ( PSmax[i] != rOther.PSmax[i] )
      {
         return false;
      }

      if ( LLIMmax[i] != rOther.LLIMmax[i] )
      {
         return false;
      }
   }

   return true;
}

bool CLoadFactors::operator!=(const CLoadFactors& rOther) const
{
   return !CLoadFactors::operator==(rOther);
}

void CLoadFactors::MakeCopy(const CLoadFactors& rOther)
{
   for ( int i = 0; i < 6; i++ )
   {
      DCmin[i] = rOther.DCmin[i];
      DWmin[i] = rOther.DWmin[i];
      CRmin[i] = rOther.CRmin[i];
      SHmin[i] = rOther.SHmin[i];
      REmin[i] = rOther.REmin[i];
      PSmin[i] = rOther.PSmin[i];
      LLIMmin[i] = rOther.LLIMmin[i];
      DCmax[i] = rOther.DCmax[i];
      DWmax[i] = rOther.DWmax[i];
      CRmax[i] = rOther.CRmax[i];
      SHmax[i] = rOther.SHmax[i];
      REmax[i] = rOther.REmax[i];
      PSmax[i] = rOther.PSmax[i];
      LLIMmax[i] = rOther.LLIMmax[i];
   }
}

void CLoadFactors::MakeAssignment(const CLoadFactors& rOther)
{
   MakeCopy(rOther);
}

HRESULT CLoadFactors::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   std::_tstring strLimitState[] = {_T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI")};

   pStrSave->BeginUnit(_T("LoadFactors"),3.0);
   int nLimitStates = sizeof(strLimitState)/sizeof(std::_tstring);
   for ( int i = 0; i < nLimitStates; i++ )
   {
      pStrSave->BeginUnit(strLimitState[i].c_str(),1.0);
      
      pStrSave->put_Property(_T("DCmin"),  CComVariant(DCmin[i]));
      pStrSave->put_Property(_T("DCmax"),  CComVariant(DCmax[i]));
      pStrSave->put_Property(_T("DWmin"),  CComVariant(DWmin[i]));
      pStrSave->put_Property(_T("DWmax"),  CComVariant(DWmax[i]));
      pStrSave->put_Property(_T("CRmin"),  CComVariant(CRmin[i]));
      pStrSave->put_Property(_T("CRmax"),  CComVariant(CRmax[i]));
      pStrSave->put_Property(_T("SHmin"),  CComVariant(SHmin[i]));
      pStrSave->put_Property(_T("SHmax"),  CComVariant(SHmax[i]));
      pStrSave->put_Property(_T("REmin"),  CComVariant(REmin[i]));
      pStrSave->put_Property(_T("REmax"),  CComVariant(REmax[i]));
      pStrSave->put_Property(_T("PSmin"),  CComVariant(PSmin[i]));
      pStrSave->put_Property(_T("PSmax"),  CComVariant(PSmax[i]));
      pStrSave->put_Property(_T("LLIMmin"),CComVariant(LLIMmin[i]));
      pStrSave->put_Property(_T("LLIMmax"),CComVariant(LLIMmax[i]));

      pStrSave->EndUnit();
   }
   pStrSave->EndUnit();

   return S_OK;
}

HRESULT CLoadFactors::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   std::_tstring strLimitState[] = {_T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI")};
   int nLimitStates = sizeof(strLimitState)/sizeof(std::_tstring);

   pStrLoad->BeginUnit(_T("LoadFactors"));

   Float64 version;
   pStrLoad->get_Version(&version);

   for ( int i = 0; i < nLimitStates; i++ )
   {
      pStrLoad->BeginUnit(strLimitState[i].c_str());

      CComVariant var;
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("DCmin"),  &var);
      DCmin[i]   = var.dblVal;

      pStrLoad->get_Property(_T("DCmax"),  &var);
      DCmax[i]   = var.dblVal;

      pStrLoad->get_Property(_T("DWmin"),  &var);
      DWmin[i]   = var.dblVal;

      pStrLoad->get_Property(_T("DWmax"),  &var);
      DWmax[i]   = var.dblVal;

      if ( 1 < version )
      {
         pStrLoad->get_Property(_T("CRmin"),  &var);
         CRmin[i]   = var.dblVal;

         pStrLoad->get_Property(_T("CRmax"),  &var);
         CRmax[i]   = var.dblVal;

         pStrLoad->get_Property(_T("SHmin"),  &var);
         SHmin[i]   = var.dblVal;

         pStrLoad->get_Property(_T("SHmax"),  &var);
         SHmax[i]   = var.dblVal;

         if ( 2 < version )
         {
            pStrLoad->get_Property(_T("REmin"),  &var);
            REmin[i]   = var.dblVal;

            pStrLoad->get_Property(_T("REmax"),  &var);
            REmax[i]   = var.dblVal;
         }
 
         pStrLoad->get_Property(_T("PSmin"),  &var);
         PSmin[i]   = var.dblVal;

         pStrLoad->get_Property(_T("PSmax"),  &var);
         PSmax[i]   = var.dblVal;
     }

      pStrLoad->get_Property(_T("LLIMmin"),&var);
      LLIMmin[i] = var.dblVal;

      pStrLoad->get_Property(_T("LLIMmax"),&var);
      LLIMmax[i] = var.dblVal;

      pStrLoad->EndUnit();
   }
   pStrLoad->EndUnit();

   return S_OK;
}
