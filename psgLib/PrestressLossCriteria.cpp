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
#include <psgLib/PrestressLossCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include "SpecLibraryEntryImpl.h"
#include <EAF/EAFDisplayUnits.h>

inline constexpr auto operator+(WBFL::LRFD::RefinedLosses2005::RelaxationLossMethod a) noexcept { return std::underlying_type<WBFL::LRFD::RefinedLosses2005::RelaxationLossMethod>::type(a); }


bool PrestressLossCriteria::Compare(const PrestressLossCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (LossMethod != other.LossMethod)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Methods are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }
   else
   {
      if (LossMethod == LossMethodType::AASHTO_REFINED || LossMethod == LossMethodType::WSDOT_REFINED)
      {
         if (AreElasticGainsApplicable(impl.GetSpecificationCriteria().GetEdition()))
         {
            if (!::IsEqual(ShippingTime, other.ShippingTime) ||
               RelaxationLossMethod != other.RelaxationLossMethod)
            {
               bSame = false;
               vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Parameters are different"), _T(""), _T("")));
               if (bReturnOnFirstDifference) return false;
            }

            if (!::IsEqual(SlabElasticGain, other.SlabElasticGain) ||
               !::IsEqual(SlabPadElasticGain, other.SlabPadElasticGain) ||
               !::IsEqual(DiaphragmElasticGain, other.DiaphragmElasticGain) ||
               !::IsEqual(UserDCElasticGain_BeforeDeckPlacement, other.UserDCElasticGain_BeforeDeckPlacement) ||
               !::IsEqual(UserDWElasticGain_BeforeDeckPlacement, other.UserDWElasticGain_BeforeDeckPlacement) ||
               !::IsEqual(UserDCElasticGain_AfterDeckPlacement, other.UserDCElasticGain_AfterDeckPlacement) ||
               !::IsEqual(UserDWElasticGain_AfterDeckPlacement, other.UserDWElasticGain_AfterDeckPlacement) ||
               !::IsEqual(RailingSystemElasticGain, other.RailingSystemElasticGain) ||
               !::IsEqual(OverlayElasticGain, other.OverlayElasticGain) ||
               !::IsEqual(SlabShrinkageElasticGain, other.SlabShrinkageElasticGain) ||
               !::IsEqual(LiveLoadElasticGain, other.LiveLoadElasticGain))
            {
               bSame = false;
               vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Elastic Gains are different"), _T(""), _T("")));
               if (bReturnOnFirstDifference) return false;
            }
         }
         else
         {
            if (!::IsEqual(ShippingLosses, other.ShippingLosses) ||
               RelaxationLossMethod != other.RelaxationLossMethod)
            {
               bSame = false;
               vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Parameters are different"), _T(""), _T("")));
               if (bReturnOnFirstDifference) return false;
            }
         }
      }
      else if (LossMethod == LossMethodType::TXDOT_REFINED_2004)
      {
         if (impl.GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::LRFDVersionMgr::Version::ThirdEdition2004)
         {
            if (!::IsEqual(ShippingLosses, other.ShippingLosses) ||
               RelaxationLossMethod != other.RelaxationLossMethod)
            {
               bSame = false;
               vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Parameters are different"), _T(""), _T("")));
               if (bReturnOnFirstDifference) return false;
            }
         }
         else
         {
            if (!::IsEqual(ShippingTime, other.ShippingTime) ||
               RelaxationLossMethod != other.RelaxationLossMethod)
            {
               bSame = false;
               vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Parameters are different"), _T(""), _T("")));
               if (bReturnOnFirstDifference) return false;
            }
         }
      }
      else if (LossMethod == LossMethodType::TXDOT_REFINED_2013)
      {
         if (!::IsEqual(ShippingLosses, other.ShippingLosses) ||
            RelaxationLossMethod != other.RelaxationLossMethod ||
            FcgpComputationMethod != other.FcgpComputationMethod)
         {
            bSame = false;
            vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Parameters are different"), _T(""), _T("")));
            if (bReturnOnFirstDifference) return false;
         }
      }
      else if (LossMethod == LossMethodType::AASHTO_LUMPSUM)
      {
         if (AreElasticGainsApplicable(impl.GetSpecificationCriteria().GetEdition()))
         {
            if (!::IsEqual(SlabElasticGain, other.SlabElasticGain) ||
               !::IsEqual(SlabPadElasticGain, other.SlabPadElasticGain) ||
               !::IsEqual(DiaphragmElasticGain, other.DiaphragmElasticGain) ||
               !::IsEqual(UserDCElasticGain_BeforeDeckPlacement, other.UserDCElasticGain_BeforeDeckPlacement) ||
               !::IsEqual(UserDWElasticGain_BeforeDeckPlacement, other.UserDWElasticGain_BeforeDeckPlacement) ||
               !::IsEqual(UserDCElasticGain_AfterDeckPlacement, other.UserDCElasticGain_AfterDeckPlacement) ||
               !::IsEqual(UserDWElasticGain_AfterDeckPlacement, other.UserDWElasticGain_AfterDeckPlacement) ||
               !::IsEqual(RailingSystemElasticGain, other.RailingSystemElasticGain) ||
               !::IsEqual(OverlayElasticGain, other.OverlayElasticGain) ||
               !::IsEqual(LiveLoadElasticGain, other.LiveLoadElasticGain))
            {
               bSame = false;
               vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Elastic Gains are different"), _T(""), _T("")));
               if (bReturnOnFirstDifference) return false;
            }
         }
         else
         {
            if (!::IsEqual(ShippingLosses, other.ShippingLosses) ||
               RelaxationLossMethod != other.RelaxationLossMethod)
            {
               bSame = false;
               vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Parameters are different"), _T(""), _T("")));
               if (bReturnOnFirstDifference) return false;
            }
         }
      }
      else if (LossMethod == LossMethodType::WSDOT_LUMPSUM)
      {
         bSame = false;
         vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Prestress Loss Methods are different"), _T(""), _T("")));
         if (bReturnOnFirstDifference) return false;
      }
      else
      {
         CHECK(LossMethod == LossMethod::TIME_STEP);
         if (TimeDependentConcreteModel != other.TimeDependentConcreteModel)
         {
            bSame = false;
            vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Time-Dependent Models are different"), _T(""), _T("")));
            if (bReturnOnFirstDifference) return false;
         }
      }
   }

   return bSame;
}

void PrestressLossCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Losses Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true);

   bool bReportElasticGainParameters = false;
   std::_tstring relaxation_method[3] = {
      std::_tstring(_T("LRFD Equation ")) + WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c-1"),_T("5.9.3.4.2c-1")),
      std::_tstring(_T("LRFD Equation ")) + WBFL::LRFD::LrfdCw8th(_T("C5.9.5.4.2c-1"),_T("C5.9.3.4.2c-1")),
      std::_tstring(_T("1.2 ksi per LRFD ")) + WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c"),_T("C5.9.3.4.2c-1"))
   };

   switch (LossMethod)
   {
   case LossMethodType::AASHTO_REFINED:
      *pPara << _T("Losses calculated in accordance with AASHTO LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4"), _T("5.9.3.4")) << _T(" Refined Estimate") << rptNewLine;
      *pPara << _T("Relaxation Loss Method = ") << relaxation_method[+RelaxationLossMethod] << rptNewLine;
      bReportElasticGainParameters = (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() ? true : false);
      break;
   case LossMethodType::WSDOT_REFINED:
      *pPara << _T("Losses calculated in accordance with AASHTO LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4"), _T("5.9.3.4")) << _T(" Refined Estimate and WSDOT Bridge Design") << rptNewLine;
      *pPara << _T("Relaxation Loss Method = ") << relaxation_method[+RelaxationLossMethod] << rptNewLine;
      bReportElasticGainParameters = (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() ? true : false);
      break;
   case LossMethodType::TXDOT_REFINED_2004:
      *pPara << _T("Losses calculated in accordance with AASHTO LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4"), _T("5.9.3.4")) << _T(" Refined Estimate and TxDOT Bridge Design") << rptNewLine;
      break;
   case LossMethodType::TXDOT_REFINED_2013:
      *pPara << _T("Losses calculated accordance with TxDOT Bridge Research Report 0-6374-2, June, 2013") << rptNewLine;
      break;
   case LossMethodType::AASHTO_LUMPSUM:
      bReportElasticGainParameters = (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() ? true : false);
      *pPara << _T("Losses calculated in accordance with AASHTO LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.3"), _T("5.9.3.3")) << (bReportElasticGainParameters ? _T(" Approximate Estimate") : _T(" Approximate Lump Sum Estimate")) << rptNewLine;
      break;
   case LossMethodType::AASHTO_LUMPSUM_2005:
      *pPara << _T("Losses calculated in accordance with AASHTO LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.3"), _T("5.9.3.3")) << _T(" Approximate Method") << rptNewLine;
      break;
   case LossMethodType::WSDOT_LUMPSUM:
      *pPara << _T("Losses calculated in accordance with AASHTO LRFD  ") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.3"), _T("5.9.3.3")) << _T(" Approximate Method and WSDOT Bridge Design Manual") << rptNewLine;
      break;
   case LossMethodType::TIME_STEP:
      *pPara << _T("Losses calculated with a time-step method") << rptNewLine;
      if (TimeDependentConcreteModel == TimeDependentConcreteModelType::AASHTO)
      {
         *pPara << _T("Time-dependent concrete properties based on AASHTO LRFD") << rptNewLine;
      }
      else if (TimeDependentConcreteModel == TimeDependentConcreteModelType::ACI209)
      {
         *pPara << _T("Time-dependent concrete properties based on ACI 209R-92") << rptNewLine;
      }
      else if (TimeDependentConcreteModel == TimeDependentConcreteModelType::CEBFIP)
      {
         *pPara << _T("Time-dependent concrete properties based on CEB-FIP Model Code 1990") << rptNewLine;
      }
      else
      {
         ATLASSERT(false);
      }
      break;
   default:
      ATLASSERT(false); // Should never get here
   }

   if (LossMethod != LossMethodType::TXDOT_REFINED_2013 && WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
   {
      *pPara << _T("Assumed time at shipping = ") << time.SetValue(ShippingTime) << rptNewLine;
   }
   else
   {
      if (ShippingLosses < 0)
      {
         *pPara << _T("- Shipping Losses = ") << (-100.0 * ShippingLosses) << _T("% of final losses, but not less than losses immediately after prestress transfer") << rptNewLine;
      }
      else
      {
         *pPara << _T("- Shipping Losses = ") << stress.SetValue(ShippingLosses) << _T(", but not to exceed final losses") << rptNewLine;
      }
   }

   if (bReportElasticGainParameters)
   {
      *pPara << _T("Contribution to Elastic Gains") << rptNewLine;
      *pPara << _T("Slab = ") << SlabElasticGain * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("Haunch = ") << SlabPadElasticGain * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("Diaphragms = ") << DiaphragmElasticGain * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("User DC (Before Deck Placement) = ") << UserDCElasticGain_BeforeDeckPlacement * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("User DW (Before Deck Placement) = ") << UserDWElasticGain_BeforeDeckPlacement * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("Railing System = ") << RailingSystemElasticGain * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("Overlay = ") << OverlayElasticGain * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("User DC (After Deck Placement) = ") << UserDCElasticGain_AfterDeckPlacement * 100.0 << _T("%") << rptNewLine;
      *pPara << _T("User DW (After Deck Placement) = ") << UserDWElasticGain_AfterDeckPlacement * 100.0 << _T("%") << rptNewLine;

      if (IsDeckShrinkageApplicable(WBFL::LRFD::LRFDVersionMgr::GetVersion()))
      {
         *pPara << _T("Deck Shrinkage = ") << SlabShrinkageElasticGain * 100.0 << _T("%") << rptNewLine;
      }
   }
}

void PrestressLossCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("PrestressLossCriteria"), 1.0);

   pSave->Property(_T("LossMethod"), +LossMethod);
   pSave->Property(_T("RelaxationLossMethod"), +RelaxationLossMethod);
   pSave->Property(_T("TimeDependentConcreteModel"), +TimeDependentConcreteModel);
   pSave->Property(_T("FcgpComputationMethod"), +FcgpComputationMethod);
   pSave->Property(_T("ShippingLosses"), ShippingLosses);
   pSave->Property(_T("ShippingTime"), ShippingTime);
   pSave->Property(_T("SlabElasticGain"), SlabElasticGain);
   pSave->Property(_T("SlabPadElasticGain"), SlabPadElasticGain);
   pSave->Property(_T("DiaphragmElasticGain"), DiaphragmElasticGain);
   pSave->Property(_T("UserDCElasticGain_BeforeDeckPlacement"), UserDCElasticGain_BeforeDeckPlacement);
   pSave->Property(_T("UserDWElasticGain_BeforeDeckPlacement"), UserDWElasticGain_BeforeDeckPlacement);
   pSave->Property(_T("UserDCElasticGain_AfterDeckPlacement"), UserDCElasticGain_AfterDeckPlacement);
   pSave->Property(_T("UserDWElasticGain_AfterDeckPlacement"), UserDWElasticGain_AfterDeckPlacement);
   pSave->Property(_T("RailingSystemElasticGain"), RailingSystemElasticGain);
   pSave->Property(_T("OverlayElasticGain"), OverlayElasticGain);
   pSave->Property(_T("SlabShrinkageElasticGain"), SlabShrinkageElasticGain);
   pSave->Property(_T("LiveLoadElasticGain"), LiveLoadElasticGain);

   pSave->EndUnit(); // PrestressLossCriteria
}

void PrestressLossCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("PrestressLossCriteria"))) THROW_LOAD(InvalidFileFormat, pLoad);

   long value;
   if (!pLoad->Property(_T("LossMethod"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   LossMethod = (decltype(LossMethod))value;

   if (!pLoad->Property(_T("RelaxationLossMethod"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   RelaxationLossMethod = (decltype(RelaxationLossMethod))value;

   if (!pLoad->Property(_T("TimeDependentConcreteModel"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   TimeDependentConcreteModel = (decltype(TimeDependentConcreteModel))value;

   if (!pLoad->Property(_T("FcgpComputationMethod"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   FcgpComputationMethod = (decltype(FcgpComputationMethod))value;

   if (!pLoad->Property(_T("ShippingLosses"), &ShippingLosses)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("ShippingTime"), &ShippingTime)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SlabElasticGain"), &SlabElasticGain)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SlabPadElasticGain"), &SlabPadElasticGain)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("DiaphragmElasticGain"), &DiaphragmElasticGain)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("UserDCElasticGain_BeforeDeckPlacement"), &UserDCElasticGain_BeforeDeckPlacement)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("UserDWElasticGain_BeforeDeckPlacement"), &UserDWElasticGain_BeforeDeckPlacement)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("UserDCElasticGain_AfterDeckPlacement"), &UserDCElasticGain_AfterDeckPlacement)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("UserDWElasticGain_AfterDeckPlacement"), &UserDWElasticGain_AfterDeckPlacement)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("RailingSystemElasticGain"), &RailingSystemElasticGain)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("OverlayElasticGain"), &OverlayElasticGain)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SlabShrinkageElasticGain"), &SlabShrinkageElasticGain)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("LiveLoadElasticGain"), &LiveLoadElasticGain)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // PrestressLossCriteria
}

bool PrestressLossCriteria::AreElasticGainsApplicable(WBFL::LRFD::LRFDVersionMgr::Version edition) const
{
   bool bIsApplicable = false;
   if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEdition2004 < edition)
   {
      if (LossMethod == LossMethodType::AASHTO_REFINED|| LossMethod == LossMethodType::WSDOT_REFINED || LossMethod == LossMethodType::AASHTO_LUMPSUM )
      {
         bIsApplicable = true;
      }
   }

   return bIsApplicable;
}

bool PrestressLossCriteria::IsDeckShrinkageApplicable(WBFL::LRFD::LRFDVersionMgr::Version edition) const
{
   bool bIsApplicable = false;
   if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEdition2004 < edition)
   {
      if (LossMethod == LossMethodType::AASHTO_REFINED || LossMethod == LossMethodType::WSDOT_REFINED)
      {
         bIsApplicable = true;
      }
   }

   return bIsApplicable;
}
