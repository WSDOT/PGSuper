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
#include <psgLib/InterfaceShearCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool InterfaceShearCriteria::Compare(const InterfaceShearCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (ShearFlowMethod != other.ShearFlowMethod ||
      !::IsEqual(MaxInterfaceShearConnectorSpacing, other.MaxInterfaceShearConnectorSpacing) ||
      bUseDeckWeightForPc != other.bUseDeckWeightForPc)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Horizontal Interface Shear requirements are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void InterfaceShearCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Interface Shear Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   switch (ShearFlowMethod)
   {
   case pgsTypes::sfmLRFD:
      *pPara << _T("Shear stress at girder/deck interface computed using the LRFD simplified method: ") << Sub2(_T("V"), _T("ui")) << _T(" = ") << _T("V/bd") << rptNewLine;
      break;

   case pgsTypes::sfmClassical:
      *pPara << _T("Shear stress at girder/deck interface computed using the classical shear flow formula: ") << Sub2(_T("V"), _T("ui")) << _T(" = (") << Sub2(_T("V"), _T("u")) << _T("Q)") << _T("/") << _T("(Ib)") << rptNewLine;
      break;
   }

   *pPara << _T("Maximum spacing of interface shear connectors (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.8.4.2"), _T("5.7.4.5")) << _T("): ") << dim.SetValue(MaxInterfaceShearConnectorSpacing);
   if (WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition())
   {
      *pPara << _T(", or the depth of the member.");
   }
   *pPara << rptNewLine;

   *pPara << _T("Interface shear compressive force normal to shear plane, Pc, for use in LRFD Eq ") << WBFL::LRFD::LrfdCw8th(_T("5.8.4.1-3"), _T("5.7.4.3-3")) << _T(" is ");
   if (bUseDeckWeightForPc)
   {
      *pPara << _T("computed from the deck weight.") << rptNewLine;
   }
   else
   {
      *pPara << _T("conservatively taken to be zero.") << rptNewLine;
   }
}

void InterfaceShearCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("InterfaceShearCriteria"), 1.0);
 
   pSave->Property(_T("ShearFlowMethod"),ShearFlowMethod);
   pSave->Property(_T("MaxInterfaceShearConnectorSpacing"),MaxInterfaceShearConnectorSpacing);
   pSave->Property(_T("bUseDeckWeightForPc"),bUseDeckWeightForPc);

   pSave->EndUnit();
}

void InterfaceShearCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("InterfaceShearCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("ShearFlowMethod"), (long*)&ShearFlowMethod)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxInterfaceShearConnectorSpacing"), &MaxInterfaceShearConnectorSpacing)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("bUseDeckWeightForPc"), &bUseDeckWeightForPc)) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
