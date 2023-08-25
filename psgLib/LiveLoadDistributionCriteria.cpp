///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <psgLib\LiveLoadDistributionCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool LiveLoadDistributionCriteria::operator==(const LiveLoadDistributionCriteria& other) const
{
   return !operator!=(other);
}

bool LiveLoadDistributionCriteria::operator!=(const LiveLoadDistributionCriteria& other) const
{
   return LldfMethod != other.LldfMethod or
      bIgnoreSkewReductionForMoment != other.bIgnoreSkewReductionForMoment or
      bLimitDistributionFactorsToLanesBeams != other.bLimitDistributionFactorsToLanesBeams or
      !::IsEqual(MaxAngularDeviationBetweenGirders, other.MaxAngularDeviationBetweenGirders) or
      !::IsEqual(MinGirderStiffnessRatio, other.MinGirderStiffnessRatio) or
      !::IsEqual(GirderSpacingLocation, other.GirderSpacingLocation) or
      bUseRigidMethod != other.bUseRigidMethod or
      bExteriorBeamLiveLoadDistributionGTInteriorBeam != other.bExteriorBeamLiveLoadDistributionGTInteriorBeam;
}

bool LiveLoadDistributionCriteria::Compare(const LiveLoadDistributionCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if(operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Live Load Distribution Factors are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void LiveLoadDistributionCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Live Load Distribution Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
    
   if (LldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::LRFD)
   {
      *pPara << _T("Live load distribution factors are calculated in accordance with LRFD 4.6.2.2") << rptNewLine;
   }
   else if (LldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::WSDOT)
   {
      *pPara << _T("Live load distribution factors are calculated in accordance with WSDOT Bridge Design Manual") << rptNewLine;
   }
   else if (LldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
   {
      *pPara << _T("Live load distribution factors are calculated in accordance with TxDOT LRFD Bridge Design Manual") << rptNewLine;
   }
   else
   {
      CHECK(false); // new method?
   }

   if (bIgnoreSkewReductionForMoment)
   {
      *pPara << _T("Skew reduction factors of LRFD 4.6.2.2.2e are not applied to moments") << rptNewLine;
   }

   if (bLimitDistributionFactorsToLanesBeams)
   {
      *pPara << _T("(Number of Lanes)/(Number of Beams) is the load limit for all distribution factors") << rptNewLine;
   }

   if (bExteriorBeamLiveLoadDistributionGTInteriorBeam)
   {
      *pPara << _T("Exterior Girder Live Load Distribution Factors cannot be less than those for Adjacent Interior Girder") << rptNewLine;
   }

   *pPara << _T("When determining if girders are approximately parallel, the maximum angular deviation between girders is: ") << ToDegrees(MaxAngularDeviationBetweenGirders) << _T(" deg") << rptNewLine;
   *pPara << _T("When determining of girders are of approximately the same stiffness, the minimum ratio of (Ix min / Ix max) is: ") << MinGirderStiffnessRatio << rptNewLine;
   *pPara << _T("Location to measure girder spacing (fraction of girder's span length): ") << GirderSpacingLocation << rptNewLine;

   if (bUseRigidMethod)
   {
      *pPara << _T("The rigid method for exterior beam moment distribution factors for Type a, e, and k sections from LRFD 4.6.2.2.2d is used") << rptNewLine;
   }
}

void LiveLoadDistributionCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("LiveLoadDistributionCriteria"), 1.0);
   pSave->Property(_T("LldfMethod"), (int)LldfMethod);
   pSave->Property(_T("bIgnoreSkewReductionForMoment"), bIgnoreSkewReductionForMoment);
   pSave->Property(_T("MaxAngularDeviationBetweenGirders"), MaxAngularDeviationBetweenGirders);
   pSave->Property(_T("MinGirderStiffnessRatio"), MinGirderStiffnessRatio);
   pSave->Property(_T("GirderSpacingLocation"), GirderSpacingLocation);
   pSave->Property(_T("bLimitDistributionFactorsToLanesBeams"), bLimitDistributionFactorsToLanesBeams);
   pSave->Property(_T("bExteriorBeamLiveLoadDistributionGTInteriorBeam"), bExteriorBeamLiveLoadDistributionGTInteriorBeam);
   pSave->Property(_T("bUseRigidMethod"), bUseRigidMethod);
   pSave->EndUnit();
}

void LiveLoadDistributionCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("LiveLoadDistributionCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   int value;
   if (!pLoad->Property(_T("LldfMethod"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   LldfMethod = (pgsTypes::LiveLoadDistributionFactorMethod)value;

   if (!pLoad->Property(_T("bIgnoreSkewReductionForMoment"), &bIgnoreSkewReductionForMoment)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxAngularDeviationBetweenGirders"), &MaxAngularDeviationBetweenGirders)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MinGirderStiffnessRatio"), &MinGirderStiffnessRatio)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("GirderSpacingLocation"), &GirderSpacingLocation)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("bLimitDistributionFactorsToLanesBeams"), &bLimitDistributionFactorsToLanesBeams)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("bExteriorBeamLiveLoadDistributionGTInteriorBeam"), &bExteriorBeamLiveLoadDistributionGTInteriorBeam)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("bUseRigidMethod"), &bUseRigidMethod)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
