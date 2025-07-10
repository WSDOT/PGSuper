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
#include <PsgLib\BearingCriteria.h>
#include <PsgLib/DifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool BearingCriteria::operator==(const BearingCriteria& other) const
{
   return !operator!=(other);
}

bool BearingCriteria::operator!=(const BearingCriteria& other) const
{
   return bAlertTaperedSolePlateRequirement != other.bAlertTaperedSolePlateRequirement
      or
      !IsEqual(TaperedSolePlateInclinationThreshold, other.TaperedSolePlateInclinationThreshold)
      or
      bUseImpactForBearingReactions != bUseImpactForBearingReactions;
}

bool BearingCriteria::Compare(const BearingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (bAlertTaperedSolePlateRequirement != other.bAlertTaperedSolePlateRequirement)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Alert tapered sole plate requirement are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (bAlertTaperedSolePlateRequirement && !::IsEqual(TaperedSolePlateInclinationThreshold, other.TaperedSolePlateInclinationThreshold))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Tapered Sole Plate inclination thresholds are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (bUseImpactForBearingReactions != other.bUseImpactForBearingReactions)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Bearing reactions dynamic load allowances are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void BearingCriteria::Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Bearing Design Parameters Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Tapered Sole Plate requirements of LRFD 14.8.2 are ");
   if (!bAlertTaperedSolePlateRequirement)
   {
      *pPara << _T("not ");
   }
   *pPara << _T("evaluated.") << rptNewLine;
   if (bAlertTaperedSolePlateRequirement)
   {
      *pPara << _T("Tapered Sole Plates are required when the inclination of the underside of the girder to the horizontal exceeds ") << TaperedSolePlateInclinationThreshold << _T(" rad.") << rptNewLine;
   }

   *pPara << _T("Dynamic load allowance is ");
   if (!bUseImpactForBearingReactions)
   {
      *pPara << _T("not ");
   }
   *pPara << _T("included in live load reactions and rotations.") << rptNewLine;
}

void BearingCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("BearingCriteria"), 2.0);

   pSave->Property(_T("bAlertTaperedSolePlateRequirement"), bAlertTaperedSolePlateRequirement);
   pSave->Property(_T("TaperedSolePlateInclinationThreshold"), TaperedSolePlateInclinationThreshold);
   pSave->Property(_T("bUseImpactForBearingReactions"), bUseImpactForBearingReactions);

   pSave->Property(_T("bCheck"), bCheck);

   int design_method = static_cast<int>(AnalysisMethod);
   pSave->Property(_T("BearingDesignMethod"), design_method);
   pSave->Property(_T("MinimumElastomerShearModulus"), MinimumElastomerShearModulus);
   pSave->Property(_T("MaximumElastomerShearModulus"), MaximumElastomerShearModulus);
   pSave->Property(_T("bRequiredIntermediateElastomerThickness"), bRequiredIntermediateElastomerThickness);
   pSave->Property(_T("RequiredIntermediateElastomerThickness"), RequiredIntermediateElastomerThickness);
   pSave->Property(_T("bMinimumTotalBearingHeight"), bMinimumTotalBearingHeight);
   pSave->Property(_T("MinimumTotalBearingHeight"), MinimumTotalBearingHeight);
   pSave->Property(_T("bMinimumBearingEdgeToGirderEdgeDistance"), bMinimumBearingEdgeToGirderEdgeDistance);
   pSave->Property(_T("MinimumBearingEdgeToGirderEdgeDistance"), MinimumBearingEdgeToGirderEdgeDistance);
   pSave->Property(_T("bMaximumBearingEdgeToGirderEdgeDistance"), bMaximumBearingEdgeToGirderEdgeDistance);
   pSave->Property(_T("MaximumBearingEdgeToGirderEdgeDistance"), MaximumBearingEdgeToGirderEdgeDistance);
   pSave->Property(_T("bRequiredBearingEdgeToGirderEdgeDistance"), bRequiredBearingEdgeToGirderEdgeDistance);
   pSave->Property(_T("RequiredBearingEdgeToGirderEdgeDistance"), RequiredBearingEdgeToGirderEdgeDistance);
   pSave->Property(_T("MaximumLiveLoadDeflection"), MaximumLiveLoadDeflection);
   pSave->Property(_T("bMaximumTotalLoad"), bMaximumTotalLoad);
   pSave->Property(_T("MaximumTotalLoad"), MaximumTotalLoad);

   pSave->EndUnit();

}

void BearingCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{

   PRECONDITION(83 <= pLoad->GetVersion());

    if (!pLoad->BeginUnit(_T("BearingCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

    if (!pLoad->Property(_T("bAlertTaperedSolePlateRequirement"), &bAlertTaperedSolePlateRequirement)) THROW_LOAD(InvalidFileFormat, pLoad);
    if (!pLoad->Property(_T("TaperedSolePlateInclinationThreshold"), &TaperedSolePlateInclinationThreshold)) THROW_LOAD(InvalidFileFormat, pLoad);
    if (!pLoad->Property(_T("bUseImpactForBearingReactions"), &bUseImpactForBearingReactions)) THROW_LOAD(InvalidFileFormat, pLoad);


    if (pLoad->GetVersion() > 1.0)
    {
        if (!pLoad->Property(_T("bCheck"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
        int design_method = static_cast<int>(AnalysisMethod);
        if (!pLoad->Property(_T("BearingDesignMethod"), &design_method)) THROW_LOAD(InvalidFileFormat, pLoad);
        AnalysisMethod = static_cast<WBFL::EngTools::BearingAnalysisMethod>(design_method);
        if (!pLoad->Property(_T("MinimumElastomerShearModulus"), &MinimumElastomerShearModulus)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("MaximumElastomerShearModulus"), &MaximumElastomerShearModulus)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("bRequiredIntermediateElastomerThickness"), &bRequiredIntermediateElastomerThickness)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("RequiredIntermediateElastomerThickness"), &RequiredIntermediateElastomerThickness)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("bMinimumTotalBearingHeight"), &bMinimumTotalBearingHeight)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("MinimumTotalBearingHeight"), &MinimumTotalBearingHeight)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("bMinimumBearingEdgeToGirderEdgeDistance"), &bMinimumBearingEdgeToGirderEdgeDistance)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("MinimumBearingEdgeToGirderEdgeDistance"), &MinimumBearingEdgeToGirderEdgeDistance)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("bMaximumBearingEdgeToGirderEdgeDistance"), &bMaximumBearingEdgeToGirderEdgeDistance)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("MaximumBearingEdgeToGirderEdgeDistance"), &MaximumBearingEdgeToGirderEdgeDistance)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("bRequiredBearingEdgeToGirderEdgeDistance"), &bRequiredBearingEdgeToGirderEdgeDistance)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("RequiredBearingEdgeToGirderEdgeDistance"), &RequiredBearingEdgeToGirderEdgeDistance)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("MaximumLiveLoadDeflection"), &MaximumLiveLoadDeflection)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("bMaximumTotalLoad"), &bMaximumTotalLoad)) THROW_LOAD(InvalidFileFormat, pLoad);
        if (!pLoad->Property(_T("MaximumTotalLoad"), &MaximumTotalLoad)) THROW_LOAD(InvalidFileFormat, pLoad);
    }

    if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);

}
