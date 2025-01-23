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
#include <psgLib/ThermalMovementCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
//#include <EAF/EAFDisplayUnits.h>

bool ThermalMovementCriteria::Compare(const ThermalMovementCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
    bool bSame = true;
    if (ThermalMovementFactor != other.ThermalMovementFactor)
    {
        bSame = false;
        vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Thermal Movement Factors Are Different"), _T(""), _T("")));
        if (bReturnOnFirstDifference) return false;
    }

    return bSame;
}

void ThermalMovementCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
    rptParagraph* pPara = new rptParagraph;
    *pChapter << pPara;

    *pPara << Bold(_T("Thermal Range Factor: ")) << ThermalMovementFactor << rptNewLine;

}

void ThermalMovementCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
    pSave->BeginUnit(_T("ThermalMovementCriteria"), 1.0);
    pSave->Property(_T("ThermalMovementFactor"), ThermalMovementFactor);
    pSave->EndUnit();
}

void ThermalMovementCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
    PRECONDITION(83 <= pLoad->GetVersion());

    if (!pLoad->BeginUnit(_T("ThermalMovementCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

    Float64 value;
    if (!pLoad->Property(_T("ThermalMovementFactor"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
    ThermalMovementFactor = (Float64)value;

    if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
