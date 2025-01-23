///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <psgLib\PrincipalTensionStressCriteria.h>
#include <EAF/EAFDisplayUnits.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

bool PrincipalTensionStressCriteria::operator==(const PrincipalTensionStressCriteria& other) const
{
   return !operator!=(other);
}

bool PrincipalTensionStressCriteria::operator!=(const PrincipalTensionStressCriteria& other) const
{
   return Method != other.Method
      or
      !IsEqual(Coefficient, other.Coefficient)
      or
      !IsEqual(TendonNearnessFactor, other.TendonNearnessFactor)
      or
      !IsEqual(FcThreshold, other.FcThreshold)
      or
      !IsEqual(UngroutedMultiplier, other.UngroutedMultiplier)
      or
      !IsEqual(GroutedMultiplier, other.GroutedMultiplier);
}

bool PrincipalTensionStressCriteria::Compare(const PrincipalTensionStressCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Principal Tensile Stress in Web parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void PrincipalTensionStressCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Principal Web Tension Stress") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if (Method == pgsTypes::ptsmNCHRP)
   {
      *pPara << _T("Principal web stress computed using WSDOT BDM/NCHRP Report 849, Equation 3.8") << rptNewLine;
   }
   else
   {
      *pPara << _T("Principal web stress computed using AASHTO LRFD Equation 5.9.2.3.3-1") << rptNewLine;
   }

   *pPara << _T("Allowable tension coefficient: ") << tension_coeff.SetValue(Coefficient) << symbol(lambda) << RPT_SQRT_FC << rptNewLine;
   *pPara << _T("When vertical analysis location is within ") << TendonNearnessFactor << _T(" diameters from a duct location, Reduce web width for ungrouted ducts by ") << UngroutedMultiplier << _T(" * duct diameter, and ") << GroutedMultiplier << _T(" * duct diameter, for grouted ducts.") << rptNewLine;
}

void PrincipalTensionStressCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("PrincipalTensionStressCriteria"), 1.0);
   pSave->Property(_T("Method"), Method);
   pSave->Property(_T("Coefficient"), Coefficient);
   pSave->Property(_T("TendonNearnessFactor"), TendonNearnessFactor);
   pSave->Property(_T("FcThreshold"), FcThreshold);
   pSave->Property(_T("UngroutedMultiplier"), UngroutedMultiplier);
   pSave->Property(_T("GroutedMultiplier"), GroutedMultiplier);
   pSave->EndUnit();
}

void PrincipalTensionStressCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("PrincipalTensionStressCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   int value;
   if (!pLoad->Property(_T("Method"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   Method = (pgsTypes::PrincipalTensileStressMethod)value;

   if (!pLoad->Property(_T("Coefficient"), &Coefficient)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("TendonNearnessFactor"), &TendonNearnessFactor)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("FcThreshold"), &FcThreshold)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("UngroutedMultiplier"), &UngroutedMultiplier)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("GroutedMultiplier"), &GroutedMultiplier)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
