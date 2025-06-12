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
#include <PsgLib/CreepCriteria.h>
#include <PsgLib/DifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool CreepCriteria::Compare(const CreepCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (!::IsEqual(XferTime, other.XferTime) ||
      !::IsEqual(CreepDuration1Min, other.CreepDuration1Min) ||
      !::IsEqual(CreepDuration2Min, other.CreepDuration2Min) ||
      !::IsEqual(CreepDuration1Max, other.CreepDuration1Max) ||
      !::IsEqual(CreepDuration2Max, other.CreepDuration2Max) ||
      !::IsEqual(TotalCreepDuration, other.TotalCreepDuration) ||
      !::IsEqual(CamberVariability, other.CamberVariability))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Creep and Camber parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (CuringMethod != other.CuringMethod ||
      (CuringMethod == pgsTypes::CuringMethod::Accelerated && (!::IsEqual(CuringMethodTimeAdjustmentFactor, other.CuringMethodTimeAdjustmentFactor))))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Curing of Precast Concrete parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void CreepCriteria::Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Creep and Camber Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE(rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true);

   *pPara << _T("# of hours from stressing to prestress transfer : ") << WBFL::Units::ConvertFromSysUnits(XferTime, WBFL::Units::Measure::Hour) << rptNewLine;
   *pPara << _T("# of days from prestress transfer until removal of temporary strands / diaphragm casting : ") << WBFL::Units::ConvertFromSysUnits(CreepDuration1Min, WBFL::Units::Measure::Day) << _T(" Min");
   *pPara << _T(", ") << WBFL::Units::ConvertFromSysUnits(CreepDuration1Max, WBFL::Units::Measure::Day) << _T(" Max") << rptNewLine;
   *pPara << _T("# of days from prestress transfer until slab-girder continuity is achieved : ") << WBFL::Units::ConvertFromSysUnits(CreepDuration2Min, WBFL::Units::Measure::Day) << _T(" Min");
   *pPara << _T(", ") << WBFL::Units::ConvertFromSysUnits(CreepDuration2Max, WBFL::Units::Measure::Day) << _T(" Max") << rptNewLine;

   *pPara << rptNewLine;

   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());
   *pPara << _T("Variability between upper and lower bound camber : ") << percentage.SetValue(CamberVariability) << rptNewLine;

   if (CuringMethod == pgsTypes::CuringMethod::Normal)
   {
      *pPara << _T("Girder was cured using Normal method") << rptNewLine;
   }
   else if (CuringMethod == pgsTypes::CuringMethod::Accelerated)
   {
      *pPara << _T("Girder was cured using Accelerated method") << rptNewLine;
   }
   else
   {
      CHECK(false); // is there a new curing method
   }

   *pPara << _T("1 day of steam or radiant heat curing is equal to ") << CuringMethodTimeAdjustmentFactor << _T(" days of normal curing") << rptNewLine;
}

void CreepCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("CreepCriteria"), 1.0);
 
   pSave->Property(_T("CreepDuration1Min"),CreepDuration1Min);
   pSave->Property(_T("CreepDuration1Max"),CreepDuration1Max);
   pSave->Property(_T("CreepDuration2Min"),CreepDuration2Min);
   pSave->Property(_T("CreepDuration2Max"),CreepDuration2Max);
   pSave->Property(_T("XferTime"),XferTime);
   pSave->Property(_T("TotalCreepDuration"),TotalCreepDuration);
   pSave->Property(_T("CamberVariability"),CamberVariability);
   pSave->Property(_T("CuringMethod"), (Int16)CuringMethod);
   pSave->Property(_T("CuringMethodTimeAdjustmentFactor"), CuringMethodTimeAdjustmentFactor);

   pSave->EndUnit();
}

void CreepCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("CreepCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CreepDuration1Min"), &CreepDuration1Min)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("CreepDuration1Max"), &CreepDuration1Max)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("CreepDuration2Min"), &CreepDuration2Min)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("CreepDuration2Max"), &CreepDuration2Max)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("XferTime"), &XferTime)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("TotalCreepDuration"), &TotalCreepDuration)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("CamberVariability"), &CamberVariability)) THROW_LOAD(InvalidFileFormat,pLoad);

   int temp;
   if (!pLoad->Property(_T("CuringMethod"), &temp)) THROW_LOAD(InvalidFileFormat, pLoad);
   CuringMethod = (pgsTypes::CuringMethod)temp;

   if (!pLoad->Property(_T("CuringMethodTimeAdjustmentFactor"), &CuringMethodTimeAdjustmentFactor)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

Float64 CreepCriteria::GetCreepDuration1(pgsTypes::CreepTime time) const
{
   return time == pgsTypes::CreepTime::Min ? CreepDuration1Min : CreepDuration1Max;
}

Float64 CreepCriteria::GetCreepDuration2(pgsTypes::CreepTime time) const
{
   return time == pgsTypes::CreepTime::Min ? CreepDuration2Min : CreepDuration2Max;
}

