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
#include "StdAfx.h"
#include <psgLib\TensionStressLimit.h>
#include <EAF/EAFDisplayUnits.h>

bool TensionStressLimit::operator==(const TensionStressLimit& other) const
{
   if (!IsEqual(Coefficient, other.Coefficient))
      return false;

   if (bHasMaxValue != other.bHasMaxValue)
      return false;

   if (bHasMaxValue && !IsEqual(MaxValue, other.MaxValue))
      return false;

   return true;
}

void TensionStressLimit::Report(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits,ConcreteSymbol concrete) const
{
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, tension, pDisplayUnits->GetStressUnit(), true);

   *pPara << tension_coefficient.SetValue(Coefficient);

   if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      (*pPara) << symbol(lambda);
   }

   if (concrete == ConcreteSymbol::fci) *pPara << RPT_SQRT_FCI; else *pPara << RPT_SQRT_FC;

   if (bHasMaxValue) *pPara << _T(" ") << symbol(LTE) << _T(" ") << tension.SetValue(MaxValue);
}

void TensionStressLimit::Save(LPCTSTR strUnitName, WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(strUnitName, 1.0);
   pSave->Property(_T("Coefficient"), Coefficient);
   pSave->Property(_T("bHasMaxValue"), bHasMaxValue);
   pSave->Property(_T("MaxValue"), MaxValue);
   pSave->EndUnit();
}

void TensionStressLimit::Load(LPCTSTR strUnitName, WBFL::System::IStructuredLoad* pLoad)
{
   if (!pLoad->BeginUnit(strUnitName)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coefficient"), &Coefficient)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("bHasMaxValue"), &bHasMaxValue)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxValue"), &MaxValue)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

Float64 TensionStressLimit::GetStressLimit(Float64 lambda, Float64 fc) const
{
   Float64 f = lambda * Coefficient * sqrt(fc);
   if (bHasMaxValue)
   {
      f = Min(f, MaxValue);
   }
   return f;
}
