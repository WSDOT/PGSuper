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
   LLIMmin[pgsTypes::ServiceI] = 1.0;       LLIMmax[pgsTypes::ServiceI] = 1.0;

   DCmin[pgsTypes::ServiceIA]   = 0.5;      DCmax[pgsTypes::ServiceIA]   = 0.5;
   DWmin[pgsTypes::ServiceIA]   = 0.5;      DWmax[pgsTypes::ServiceIA]   = 0.5;
   LLIMmin[pgsTypes::ServiceIA] = 1.0;      LLIMmax[pgsTypes::ServiceIA] = 1.0;

   DCmin[pgsTypes::ServiceIII]   = 1.0;     DCmax[pgsTypes::ServiceIII]   = 1.0;
   DWmin[pgsTypes::ServiceIII]   = 1.0;     DWmax[pgsTypes::ServiceIII]   = 1.0;
   LLIMmin[pgsTypes::ServiceIII] = 0.8;     LLIMmax[pgsTypes::ServiceIII] = 0.8;

   DCmin[pgsTypes::StrengthI]   = 0.90;     DCmax[pgsTypes::StrengthI]   = 1.25;
   DWmin[pgsTypes::StrengthI]   = 0.65;     DWmax[pgsTypes::StrengthI]   = 1.50;
   LLIMmin[pgsTypes::StrengthI] = 1.75;     LLIMmax[pgsTypes::StrengthI] = 1.75;

   DCmin[pgsTypes::StrengthII]   = 0.90;    DCmax[pgsTypes::StrengthII]   = 1.25;
   DWmin[pgsTypes::StrengthII]   = 0.65;    DWmax[pgsTypes::StrengthII]   = 1.50;
   LLIMmin[pgsTypes::StrengthII] = 1.35;    LLIMmax[pgsTypes::StrengthII] = 1.35;

   DCmin[pgsTypes::FatigueI]   = 0.5;      DCmax[pgsTypes::FatigueI]   = 0.5;
   DWmin[pgsTypes::FatigueI]   = 0.5;      DWmax[pgsTypes::FatigueI]   = 0.5;
   LLIMmin[pgsTypes::FatigueI] = 1.0;      LLIMmax[pgsTypes::FatigueI] = 1.5;
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
         return false;

      if ( DWmin[i] != rOther.DWmin[i] )
         return false;

      if ( LLIMmin[i] != rOther.LLIMmin[i] )
         return false;

      if ( DCmax[i] != rOther.DCmax[i] )
         return false;

      if ( DWmax[i] != rOther.DWmax[i] )
         return false;

      if ( LLIMmax[i] != rOther.LLIMmax[i] )
         return false;
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
      LLIMmin[i] = rOther.LLIMmin[i];
      DCmax[i] = rOther.DCmax[i];
      DWmax[i] = rOther.DWmax[i];
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

   pStrSave->BeginUnit(_T("LoadFactors"),1.0);
   int nLimitStates = sizeof(strLimitState)/sizeof(std::_tstring);
   for ( int i = 0; i < nLimitStates; i++ )
   {
      pStrSave->BeginUnit(strLimitState[i].c_str(),1.0);
      
      pStrSave->put_Property(_T("DCmin"),  CComVariant(DCmin[i]));
      pStrSave->put_Property(_T("DCmax"),  CComVariant(DCmax[i]));
      pStrSave->put_Property(_T("DWmin"),  CComVariant(DWmin[i]));
      pStrSave->put_Property(_T("DWmax"),  CComVariant(DWmax[i]));
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

      pStrLoad->get_Property(_T("LLIMmin"),&var);
      LLIMmin[i] = var.dblVal;

      pStrLoad->get_Property(_T("LLIMmax"),&var);
      LLIMmax[i] = var.dblVal;

      pStrLoad->EndUnit();
   }
   pStrLoad->EndUnit();

   return S_OK;
}
