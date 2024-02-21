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

#ifndef INCLUDED_PGSEXT_LOADFACTORS_H_
#define INCLUDED_PGSEXT_LOADFACTORS_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <array>
// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

class PGSEXTCLASS CLoadFactors
{
public:
   CLoadFactors();
   CLoadFactors(const CLoadFactors& rOther);
   CLoadFactors& operator=(const CLoadFactors& rOther);

   bool operator==(const CLoadFactors& rOther) const; 
   bool operator!=(const CLoadFactors& rOther) const;

   void SetDC(pgsTypes::LimitState limitState,Float64 min, Float64 max);
   void GetDC(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const;
   Float64 GetDCMin(pgsTypes::LimitState limitState) const;
   Float64 GetDCMax(pgsTypes::LimitState limitState) const;

   void SetDW(pgsTypes::LimitState limitState, Float64 min, Float64 max);
   void GetDW(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const;
   Float64 GetDWMin(pgsTypes::LimitState limitState) const;
   Float64 GetDWMax(pgsTypes::LimitState limitState) const;

   void SetCR(pgsTypes::LimitState limitState, Float64 min, Float64 max);
   void GetCR(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const;
   Float64 GetCRMin(pgsTypes::LimitState limitState) const;
   Float64 GetCRMax(pgsTypes::LimitState limitState) const;

   void SetSH(pgsTypes::LimitState limitState, Float64 min, Float64 max);
   void GetSH(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const;
   Float64 GetSHMin(pgsTypes::LimitState limitState) const;
   Float64 GetSHMax(pgsTypes::LimitState limitState) const;

   void SetRE(pgsTypes::LimitState limitState, Float64 min, Float64 max);
   void GetRE(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const;
   Float64 GetREMin(pgsTypes::LimitState limitState) const;
   Float64 GetREMax(pgsTypes::LimitState limitState) const;

   void SetPS(pgsTypes::LimitState limitState, Float64 min, Float64 max);
   void GetPS(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const;
   Float64 GetPSMin(pgsTypes::LimitState limitState) const;
   Float64 GetPSMax(pgsTypes::LimitState limitState) const;

   void SetLLIM(pgsTypes::LimitState limitState, Float64 min, Float64 max);
   void GetLLIM(pgsTypes::LimitState limitState, Float64* pMin, Float64* pMax) const;
   Float64 GetLLIMMin(pgsTypes::LimitState limitState) const;
   Float64 GetLLIMMax(pgsTypes::LimitState limitState) const;

   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);

protected:
   void MakeCopy(const CLoadFactors& rOther);
   void MakeAssignment(const CLoadFactors& rOther);

   static constexpr int nLimitStates = ((int)pgsTypes::FatigueI) + 1;
   std::array<Float64, nLimitStates> m_DCmin;   // index is one of pgsTypes::LimitState constants (except for CLLIM)
   std::array<Float64, nLimitStates> m_DWmin;
   std::array<Float64, nLimitStates> m_CRmin;
   std::array<Float64, nLimitStates> m_SHmin;
   std::array<Float64, nLimitStates> m_REmin;
   std::array<Float64, nLimitStates> m_PSmin;
   std::array<Float64, nLimitStates> m_LLIMmin;
   std::array<Float64, nLimitStates> m_DCmax;
   std::array<Float64, nLimitStates> m_DWmax;
   std::array<Float64, nLimitStates> m_CRmax;
   std::array<Float64, nLimitStates> m_SHmax;
   std::array<Float64, nLimitStates> m_REmax;
   std::array<Float64, nLimitStates> m_PSmax;
   std::array<Float64, nLimitStates> m_LLIMmax;
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


#endif // LOADFACTORS

