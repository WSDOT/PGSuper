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
   m_DCmin[pgsTypes::ServiceI]   = 1.0;       m_DCmax[pgsTypes::ServiceI]   = 1.0;
   m_DWmin[pgsTypes::ServiceI]   = 1.0;       m_DWmax[pgsTypes::ServiceI]   = 1.0;
   m_CRmin[pgsTypes::ServiceI]   = 1.0;       m_CRmax[pgsTypes::ServiceI]   = 1.0;
   m_SHmin[pgsTypes::ServiceI]   = 1.0;       m_SHmax[pgsTypes::ServiceI]   = 1.0;
   m_REmin[pgsTypes::ServiceI]   = 1.0;       m_REmax[pgsTypes::ServiceI]   = 1.0;
   m_PSmin[pgsTypes::ServiceI]   = 1.0;       m_PSmax[pgsTypes::ServiceI]   = 1.0;
   m_LLIMmin[pgsTypes::ServiceI] = 1.0;       m_LLIMmax[pgsTypes::ServiceI] = 1.0;

   m_DCmin[pgsTypes::ServiceIA]   = 0.5;      m_DCmax[pgsTypes::ServiceIA]   = 0.5;
   m_DWmin[pgsTypes::ServiceIA]   = 0.5;      m_DWmax[pgsTypes::ServiceIA]   = 0.5;
   m_CRmin[pgsTypes::ServiceIA]   = 0.0;      m_CRmax[pgsTypes::ServiceIA]   = 0.0;
   m_SHmin[pgsTypes::ServiceIA]   = 0.0;      m_SHmax[pgsTypes::ServiceIA]   = 0.0;
   m_REmin[pgsTypes::ServiceIA]   = 0.0;      m_REmax[pgsTypes::ServiceIA]   = 0.0;
   m_PSmin[pgsTypes::ServiceIA]   = 0.0;      m_PSmax[pgsTypes::ServiceIA]   = 0.0;
   m_LLIMmin[pgsTypes::ServiceIA] = 1.0;      m_LLIMmax[pgsTypes::ServiceIA] = 1.0;

   m_DCmin[pgsTypes::ServiceIII]   = 1.0;     m_DCmax[pgsTypes::ServiceIII]   = 1.0;
   m_DWmin[pgsTypes::ServiceIII]   = 1.0;     m_DWmax[pgsTypes::ServiceIII]   = 1.0;
   m_CRmin[pgsTypes::ServiceIII]   = 1.0;     m_CRmax[pgsTypes::ServiceIII]   = 1.0;
   m_SHmin[pgsTypes::ServiceIII]   = 1.0;     m_SHmax[pgsTypes::ServiceIII]   = 1.0;
   m_REmin[pgsTypes::ServiceIII]   = 1.0;     m_REmax[pgsTypes::ServiceIII]   = 1.0;
   m_PSmin[pgsTypes::ServiceIII]   = 1.0;     m_PSmax[pgsTypes::ServiceIII]   = 1.0;
   m_LLIMmin[pgsTypes::ServiceIII] = 0.8;     m_LLIMmax[pgsTypes::ServiceIII] = 0.8;

   m_DCmin[pgsTypes::StrengthI]   = 0.90;     m_DCmax[pgsTypes::StrengthI]   = 1.25;
   m_DWmin[pgsTypes::StrengthI]   = 0.65;     m_DWmax[pgsTypes::StrengthI]   = 1.50;
   m_CRmin[pgsTypes::StrengthI]   = 0.90;     m_CRmax[pgsTypes::StrengthI]   = 1.25;
   m_SHmin[pgsTypes::StrengthI]   = 0.90;     m_SHmax[pgsTypes::StrengthI]   = 1.25;
   m_REmin[pgsTypes::StrengthI]   = 0.90;     m_REmax[pgsTypes::StrengthI]   = 1.25;
   m_PSmin[pgsTypes::StrengthI]   = 1.00;     m_PSmax[pgsTypes::StrengthI]   = 1.00;
   m_LLIMmin[pgsTypes::StrengthI] = 1.75;     m_LLIMmax[pgsTypes::StrengthI] = 1.75;

   m_DCmin[pgsTypes::StrengthII]   = 0.90;    m_DCmax[pgsTypes::StrengthII]   = 1.25;
   m_DWmin[pgsTypes::StrengthII]   = 0.65;    m_DWmax[pgsTypes::StrengthII]   = 1.50;
   m_CRmin[pgsTypes::StrengthII]   = 0.90;    m_CRmax[pgsTypes::StrengthII]   = 1.25;
   m_SHmin[pgsTypes::StrengthII]   = 0.90;    m_SHmax[pgsTypes::StrengthII]   = 1.25;
   m_REmin[pgsTypes::StrengthII]   = 0.90;    m_REmax[pgsTypes::StrengthII]   = 1.25;
   m_PSmin[pgsTypes::StrengthII]   = 1.00;    m_PSmax[pgsTypes::StrengthII]   = 1.00;
   m_LLIMmin[pgsTypes::StrengthII] = 1.35;    m_LLIMmax[pgsTypes::StrengthII] = 1.35;

   m_DCmin[pgsTypes::FatigueI]   = 0.5;      m_DCmax[pgsTypes::FatigueI]   = 0.5;
   m_DWmin[pgsTypes::FatigueI]   = 0.5;      m_DWmax[pgsTypes::FatigueI]   = 0.5;
   m_CRmin[pgsTypes::FatigueI]   = 0.0;      m_CRmax[pgsTypes::FatigueI]   = 0.0;
   m_SHmin[pgsTypes::FatigueI]   = 0.0;      m_SHmax[pgsTypes::FatigueI]   = 0.0;
   m_REmin[pgsTypes::FatigueI]   = 0.0;      m_REmax[pgsTypes::FatigueI]   = 0.0;
   m_PSmin[pgsTypes::FatigueI]   = 0.0;      m_PSmax[pgsTypes::FatigueI]   = 0.0;
   m_LLIMmin[pgsTypes::FatigueI] = 1.5;      m_LLIMmax[pgsTypes::FatigueI] = 1.5;
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
   for ( int i = 0; i < nLimitStates; i++ )
   {
      if (m_DCmin[i] != rOther.m_DCmin[i] )
      {
         return false;
      }

      if (m_DWmin[i] != rOther.m_DWmin[i] )
      {
         return false;
      }

      if (m_CRmin[i] != rOther.m_CRmin[i] )
      {
         return false;
      }

      if (m_SHmin[i] != rOther.m_SHmin[i] )
      {
         return false;
      }

      if (m_REmin[i] != rOther.m_REmin[i] )
      {
         return false;
      }

      if (m_PSmin[i] != rOther.m_PSmin[i] )
      {
         return false;
      }

      if (m_LLIMmin[i] != rOther.m_LLIMmin[i] )
      {
         return false;
      }

      if (m_DCmax[i] != rOther.m_DCmax[i] )
      {
         return false;
      }

      if (m_DWmax[i] != rOther.m_DWmax[i] )
      {
         return false;
      }

      if (m_CRmax[i] != rOther.m_CRmax[i] )
      {
         return false;
      }

      if (m_SHmax[i] != rOther.m_SHmax[i] )
      {
         return false;
      }

      if (m_REmax[i] != rOther.m_REmax[i] )
      {
         return false;
      }

      if (m_PSmax[i] != rOther.m_PSmax[i] )
      {
         return false;
      }

      if (m_LLIMmax[i] != rOther.m_LLIMmax[i] )
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

void CLoadFactors::SetDC(pgsTypes::LimitState limitState, Float64 min, Float64 max)
{
   ATLASSERT(IsDesignLimitState(limitState));
   m_DCmin[limitState] = min;
   m_DCmax[limitState] = max;
}

void CLoadFactors::GetDC(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   *pMin = m_DCmin[limitState];
   *pMax = m_DCmax[limitState];
}

Float64 CLoadFactors::GetDCMin(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_DCmin[limitState];
}

Float64 CLoadFactors::GetDCMax(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_DCmax[limitState];
}

void CLoadFactors::SetDW(pgsTypes::LimitState limitState, Float64 min, Float64 max)
{
   ATLASSERT(IsDesignLimitState(limitState));
   m_DWmin[limitState] = min;
   m_DWmax[limitState] = max;
}

void CLoadFactors::GetDW(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   *pMin = m_DWmin[limitState];
   *pMax = m_DWmax[limitState];
}

Float64 CLoadFactors::GetDWMin(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_DWmin[limitState];
}

Float64 CLoadFactors::GetDWMax(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_DWmax[limitState];
}

void CLoadFactors::SetCR(pgsTypes::LimitState limitState, Float64 min, Float64 max)
{
   ATLASSERT(IsDesignLimitState(limitState));
   m_CRmin[limitState] = min;
   m_CRmax[limitState] = max;
}

void CLoadFactors::GetCR(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   *pMin = m_CRmin[limitState];
   *pMax = m_CRmax[limitState];
}

Float64 CLoadFactors::GetCRMin(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_CRmin[limitState];
}

Float64 CLoadFactors::GetCRMax(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_CRmax[limitState];
}

void CLoadFactors::SetSH(pgsTypes::LimitState limitState, Float64 min, Float64 max)
{
   ATLASSERT(IsDesignLimitState(limitState));
   m_SHmin[limitState] = min;
   m_SHmax[limitState] = max;
}

void CLoadFactors::GetSH(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   *pMin = m_SHmin[limitState];
   *pMax = m_SHmax[limitState];
}

Float64 CLoadFactors::GetSHMin(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_SHmin[limitState];
}

Float64 CLoadFactors::GetSHMax(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_SHmax[limitState];
}

void CLoadFactors::SetRE(pgsTypes::LimitState limitState, Float64 min, Float64 max)
{
   ATLASSERT(IsDesignLimitState(limitState));
   m_REmin[limitState] = min;
   m_REmax[limitState] = max;
}

void CLoadFactors::GetRE(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   *pMin = m_REmin[limitState];
   *pMax = m_REmax[limitState];
}

Float64 CLoadFactors::GetREMin(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_REmin[limitState];
}

Float64 CLoadFactors::GetREMax(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_REmax[limitState];
}

void CLoadFactors::SetPS(pgsTypes::LimitState limitState, Float64 min, Float64 max)
{
   ATLASSERT(IsDesignLimitState(limitState));
   m_PSmin[limitState] = min;
   m_PSmax[limitState] = max;
}

void CLoadFactors::GetPS(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   *pMin = m_PSmin[limitState];
   *pMax = m_PSmax[limitState];
}

Float64 CLoadFactors::GetPSMin(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_PSmin[limitState];
}

Float64 CLoadFactors::GetPSMax(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_PSmax[limitState];
}

void CLoadFactors::SetLLIM(pgsTypes::LimitState limitState, Float64 min, Float64 max)
{
   ATLASSERT(IsDesignLimitState(limitState));
   m_LLIMmin[limitState] = min;
   m_LLIMmax[limitState] = max;
}

void CLoadFactors::GetLLIM(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   *pMin = m_LLIMmin[limitState];
   *pMax = m_LLIMmax[limitState];
}

Float64 CLoadFactors::GetLLIMMin(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_LLIMmin[limitState];
}

Float64 CLoadFactors::GetLLIMMax(pgsTypes::LimitState limitState) const
{
   ATLASSERT(IsDesignLimitState(limitState));
   return m_LLIMmax[limitState];
}

void CLoadFactors::MakeCopy(const CLoadFactors& rOther)
{
   for ( int i = 0; i < nLimitStates; i++ )
   {
      m_DCmin[i] = rOther.m_DCmin[i];
      m_DWmin[i] = rOther.m_DWmin[i];
      m_CRmin[i] = rOther.m_CRmin[i];
      m_SHmin[i] = rOther.m_SHmin[i];
      m_REmin[i] = rOther.m_REmin[i];
      m_PSmin[i] = rOther.m_PSmin[i];
      m_LLIMmin[i] = rOther.m_LLIMmin[i];
      m_DCmax[i] = rOther.m_DCmax[i];
      m_DWmax[i] = rOther.m_DWmax[i];
      m_CRmax[i] = rOther.m_CRmax[i];
      m_SHmax[i] = rOther.m_SHmax[i];
      m_REmax[i] = rOther.m_REmax[i];
      m_PSmax[i] = rOther.m_PSmax[i];
      m_LLIMmax[i] = rOther.m_LLIMmax[i];
   }
}

void CLoadFactors::MakeAssignment(const CLoadFactors& rOther)
{
   MakeCopy(rOther);
}

HRESULT CLoadFactors::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   std::array<std::_tstring, 6> strLimitState = { _T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI") };

   pStrSave->BeginUnit(_T("LoadFactors"), 3.0);
   int nLimitStates = (int)strLimitState.size();
   for ( int i = 0; i < nLimitStates; i++ )
   {
      pStrSave->BeginUnit(strLimitState[i].c_str(),1.0);
      
      pStrSave->put_Property(_T("DCmin"),  CComVariant(m_DCmin[i]));
      pStrSave->put_Property(_T("DCmax"),  CComVariant(m_DCmax[i]));
      pStrSave->put_Property(_T("DWmin"),  CComVariant(m_DWmin[i]));
      pStrSave->put_Property(_T("DWmax"),  CComVariant(m_DWmax[i]));
      pStrSave->put_Property(_T("CRmin"),  CComVariant(m_CRmin[i]));
      pStrSave->put_Property(_T("CRmax"),  CComVariant(m_CRmax[i]));
      pStrSave->put_Property(_T("SHmin"),  CComVariant(m_SHmin[i]));
      pStrSave->put_Property(_T("SHmax"),  CComVariant(m_SHmax[i]));
      pStrSave->put_Property(_T("REmin"),  CComVariant(m_REmin[i]));
      pStrSave->put_Property(_T("REmax"),  CComVariant(m_REmax[i]));
      pStrSave->put_Property(_T("PSmin"),  CComVariant(m_PSmin[i]));
      pStrSave->put_Property(_T("PSmax"),  CComVariant(m_PSmax[i]));
      pStrSave->put_Property(_T("LLIMmin"),CComVariant(m_LLIMmin[i]));
      pStrSave->put_Property(_T("LLIMmax"),CComVariant(m_LLIMmax[i]));

      pStrSave->EndUnit();
   }
   pStrSave->EndUnit();

   return S_OK;
}

HRESULT CLoadFactors::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   std::array<std::_tstring, 6> strLimitState = { _T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI") };
   int nLimitStates = (int)strLimitState.size();

   pStrLoad->BeginUnit(_T("LoadFactors"));

   Float64 version;
   pStrLoad->get_Version(&version);

   for ( int i = 0; i < nLimitStates; i++ )
   {
      pStrLoad->BeginUnit(strLimitState[i].c_str());

      CComVariant var;
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("DCmin"),  &var);
      m_DCmin[i]   = var.dblVal;

      pStrLoad->get_Property(_T("DCmax"),  &var);
      m_DCmax[i]   = var.dblVal;

      pStrLoad->get_Property(_T("DWmin"),  &var);
      m_DWmin[i]   = var.dblVal;

      pStrLoad->get_Property(_T("DWmax"),  &var);
      m_DWmax[i]   = var.dblVal;

      if ( 1 < version )
      {
         pStrLoad->get_Property(_T("CRmin"),  &var);
         m_CRmin[i]   = var.dblVal;

         pStrLoad->get_Property(_T("CRmax"),  &var);
         m_CRmax[i]   = var.dblVal;

         pStrLoad->get_Property(_T("SHmin"),  &var);
         m_SHmin[i]   = var.dblVal;

         pStrLoad->get_Property(_T("SHmax"),  &var);
         m_SHmax[i]   = var.dblVal;

         if ( 2 < version )
         {
            pStrLoad->get_Property(_T("REmin"),  &var);
            m_REmin[i]   = var.dblVal;

            pStrLoad->get_Property(_T("REmax"),  &var);
            m_REmax[i]   = var.dblVal;
         }
 
         pStrLoad->get_Property(_T("PSmin"),  &var);
         m_PSmin[i]   = var.dblVal;

         pStrLoad->get_Property(_T("PSmax"),  &var);
         m_PSmax[i]   = var.dblVal;
     }

      pStrLoad->get_Property(_T("LLIMmin"),&var);
      m_LLIMmin[i] = var.dblVal;

      pStrLoad->get_Property(_T("LLIMmax"),&var);
      m_LLIMmax[i] = var.dblVal;

      pStrLoad->EndUnit();
   }
   pStrLoad->EndUnit();

   return S_OK;
}
