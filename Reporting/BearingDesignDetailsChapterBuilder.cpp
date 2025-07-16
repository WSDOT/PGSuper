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
#include <Reporting\BearingDesignDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\BearingReactionTable.h>
#include <Reporting\BearingRotationTable.h>
#include <Reporting\BearingShearDeformationTable.h>
#include <Reporting\PrestressRotationTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserRotationTable.h>

#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace/Tools.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\BearingDesignParameters.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>
#include <IFace\DistributionFactors.h>
#include <IFace/PointOfInterest.h>

#include <psgLib/ThermalMovementCriteria.h>
#include <PsgLib/SpecLibraryEntry.h>
#include <PsgLib\PierData2.h>
#include <Reporting/BearingDesignPropertiesTable.h>

CBearingDesignDetailsChapterBuilder::CBearingDesignDetailsChapterBuilder(bool bSelect) :
    CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CBearingDesignDetailsChapterBuilder::GetName() const
{
    return TEXT("Bearing Design Details");
}

rptChapter* CBearingDesignDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const
{
    auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
    auto pBroker = pGirderRptSpec->GetBroker();
    const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());
    rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

    GET_IFACE2(pBroker, IUserDefinedLoads, pUDL);
    GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
    GET_IFACE2(pBroker, ISpecification, pSpec);
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IProductLoads, pProductLoads);
    GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pLLDF);
    GET_IFACE2(pBroker, IEnvironment, pEnvironment);
    GET_IFACE2(pBroker, ILibrary, pLib);
    GET_IFACE2(pBroker, ILossParameters, pLossParams);
    GET_IFACE2(pBroker, IIntervals, pIntervals);
    
    
    INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true);
    INIT_UV_PROTOTYPE(rptLength4UnitValue, I, pDisplayUnits->GetMomentOfInertiaUnit(), true);
    INIT_UV_PROTOTYPE(rptLength2UnitValue, A, pDisplayUnits->GetAreaUnit(), true);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), true);
    INIT_UV_PROTOTYPE(rptTemperatureUnitValue, temperature, pDisplayUnits->GetTemperatureUnit(), true);

    bool are_user_loads = pUDL->DoUserLoadsExist(girderKey);

    bool bIncludeImpact = pBearingDesign->BearingLiveLoadReactionsIncludeImpact();

    DESIGNPROPERTIES details;
    pBearingDesignParameters->GetBearingDesignProperties(&details);

    rptParagraph* p = new rptParagraph(rptStyleManager::GetHeadingStyle());
    *pChapter << p;
    *p << _T("Bearing Movement Description") << rptNewLine;

    *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("bearing_orientation_description.png")) << rptNewLine;

    p = new rptParagraph;
    *pChapter << p;

    *p <<  Sub2(symbol(theta), _T("f")) << _T(" = flexural bearing rotation") << rptNewLine;
    *p << Sub2(symbol(theta), _T("t")) << _T(" = torsional bearing rotation = ") << Sub2(symbol(theta), _T("f")) << _T("tan") << Sub2(symbol(theta), _T("skew")) << rptNewLine << rptNewLine;

    p = new rptParagraph(rptStyleManager::GetHeadingStyle());
    *pChapter << p;

    *p << _T("Bearing Design Properties") << rptNewLine;
    p = new rptParagraph;
    *pChapter << p;
    *p << Sub2(_T("f"), _T("y")) << _T(" = ") << stress.SetValue(details.Fy);
    *p << _T(" for steel shims per AASHTO M251 (ASTM A 1011 Gr. 36)") << rptNewLine;
    *p << Sub2(_T("(") << symbol(DELTA) << _T("F)"), _T("TH")) << _T(" = ") << stress.SetValue(details.Fth);
    *p << _T(" LRFD Article 6.6 (Table 6.6.1.2.3-1 for Category A)") << rptNewLine;
    *p << _T("Method B is used per WSDOT Policy (BDM Ch. 9.2)") << rptNewLine;

    *p << CBearingReactionTable().BuildBearingReactionTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads, true, pDisplayUnits, true);
    SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
    if (1 < nSegments)
    {
        *p << _T("Erected Segment reactions are the segment self-weight simple span reactions after erection. Girder reactions are for the completed girder after post-tensioning and temporary support removal.") << rptNewLine;
    }
    *p << _T("*Live loads do not include impact") << rptNewLine;


    SHEARDEFORMATIONDETAILS sfDetails;
    pBearingDesignParameters->GetBearingParameters(girderKey, &sfDetails);


    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true,are_user_loads, true, pDisplayUnits, true, true);
    if (1 < nSegments)
    {
        *p << _T("Erected Segment rotations are the segment self-weight simple span rotations after erection. Girder rotations are for the completed girder after post-tensioning and temporary support removal.") << rptNewLine;
    }
    *p << _T("*Live loads do not include impact") << rptNewLine;

    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads, true, pDisplayUnits, true, false);

    if (1 < nSegments)
    {
        *p << _T("Erected Segment rotations are the segment self-weight simple span rotations after erection. Girder rotations are for the completed girder after post-tensioning and temporary support removal.") << rptNewLine;
    }
    *p << _T("*Live loads do not include impact") << rptNewLine;
    
    std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign, girderKey);
    IndexType j = 0;
    auto iter = strLLNames.begin();
    auto end = strLLNames.end();
    for (; iter != end; iter++, j++)
    {
        *p << _T("(D") << j << _T(") ") << *iter << rptNewLine;
    }

    *p << rptNewLine;

    
    pLLDF->ReportReactionDistributionFactors(girderKey, pChapter, true);


    p = new rptParagraph(rptStyleManager::GetHeadingStyle());
    *pChapter << p;
    *p << rptNewLine;
    *p << _T("Shear Deformation Details") << rptNewLine;

    p = new rptParagraph(rptStyleManager::GetSubheadingStyle());
    *pChapter << p;
    *p << _T("Shortening due to temperature difference:") << rptNewLine;

    p = new rptParagraph;
    *pChapter << p;

    *p << _T("Temperature range is computed based on Procedure A (Article 3.12.2.1)") << rptNewLine;

    *p << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("thermal_expansion.png"), _T("\\Delta_{\\text{thermal}} = \\Delta_0 \\times \\alpha \\times L_{pf} \\times (T_{\\max} - T_{\\min})")) << rptNewLine;

    *p << symbol(alpha) << _T(" = coefficient of thermal expansion") << rptNewLine;

    pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
    const auto pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
    const auto& thermalFactor = pSpecEntry->GetThermalMovementCriteria();

    if (thermalFactor.ThermalMovementFactor == 0.75)
    {
        *p << Sub2(symbol(DELTA), _T("0")) << _T(" = 0.75 (WSDOT BDM Ch. 9.2.5A)") << rptNewLine;
    }
    else
    {
        *p << Sub2(symbol(DELTA), _T("0")) << _T(" = 0.65 (AASHTO LRFD Sect. 14.7.5.3.2)") << rptNewLine;
    }

    *p << Sub2(_T("L"), _T("pf")) << _T(" = ") << _T("Distance from the point of longitudinal fixity to the bearing location for girders continuous over piers.") << rptNewLine;
    *p << _T("If all specified connection types are free to translate longitudinally, then the point of fixity is taken to be at the pier nearest the center.") << rptNewLine;


    
    if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
    {
        *p << Sub2(_T("T"), _T("max")) << _T(" = maximum temperature used for design") << rptNewLine;
        *p << Sub2(_T("T"), _T("min")) << _T(" = minimum temperature used for design") << rptNewLine;

        p = new rptParagraph(rptStyleManager::GetSubheadingStyle());
        *pChapter << p;
        *p << _T("Shear deformation at bearing, ") << Sub2(symbol(DELTA), _T("s"));
        *p << _T(", due to tendon shortening, ") << symbol(DELTA) << Sub2(_T(" L"), _T("ten")) << _T(" :") << rptNewLine;

        p = new rptParagraph;
        *pChapter << p;
        //CString html = "<h1>TEST</h1>";
        //*p << html;

        *p << _T("Bottom flange shortening is calculated using PCI BDM Eq. 10.8.3.8.2-6:") << rptNewLine;
        *p << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("BottomFlangeShortening.png"),
        _T("\\Delta_S = -\\Delta L_{bf} = -\\frac{\\left(1 + \\dfrac{e_p y_b}{r ^ 2} \\right)}{\\left(1 + \\dfrac{e_p ^ 2}{r ^ 2} \\right)} \\left(\\Delta L_{ten} \\right)"));
        *p << rptNewLine;

        *p << _T("where") << rptNewLine;
        *p << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("radius_of_gyration.png"), _T("r = \\sqrt{\\dfrac{I_{xx}}{A_g}}")) << rptNewLine;
    }

    bool bCold;
    
    if (pEnvironment->GetClimateCondition() == pgsTypes::ClimateCondition::Cold)
    {
        bCold = true;
    }
    else
    {
        bCold = false;
    }



    *p << CBearingShearDeformationTable().BuildBearingShearDeformationTable(pBroker, girderKey, pDisplayUnits,
        true, bCold, &sfDetails);

    if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
    {
        *p << _T("-Two-thirds of the total girder creep and shrinkage is assumed to occur before girders are erected") << rptNewLine;
        *p << _T("-It is assumed that creep and shrinkage effects cease after deck casting") << rptNewLine;
    }
    *p << _T("-Bearing reset effects are ignored") << rptNewLine;
    if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
    {
        *p << _T("-Deck shrinkage effects are not considered") << rptNewLine << rptNewLine;
    }

    p = new rptParagraph;
    *pChapter << p;

    rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9, _T("Bearing Recess Geometry"));

    std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

    INIT_FRACTIONAL_LENGTH_PROTOTYPE(recess_dimension, IS_US_UNITS(pDisplayUnits), 8, RoundOff, pDisplayUnits->GetComponentDimUnit(), false, true);

    ColumnIndexType col = 0;

    (*pTable)(0, col++) << _T("");
    (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
    (*pTable)(0, col++) << _T("Excess") << rptNewLine << _T("Camber") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
    (*pTable)(0, col++) << _T("Bearing") << rptNewLine << _T("Recess") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
    (*pTable)(0, col++) << _T("* Transverse") << rptNewLine << _T("Bearing") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
    (*pTable)(0, col++) << _T("** L") << rptNewLine << _T("Recess") << rptNewLine << _T("Length");
    (*pTable)(0, col++) << _T("D") << rptNewLine << _T("Recess") << rptNewLine << _T("Height");
    (*pTable)(0, col++) << Sub2(_T("D"), _T("1"));
    (*pTable)(0, col++) << Sub2(_T("D"), _T("2"));

    RowIndexType row = pTable->GetNumberOfHeaderRows();

    bool bCheckTaperedSolePlate;
    Float64 taperedSolePlateThreshold;
    pSpec->GetTaperedSolePlateRequirements(&bCheckTaperedSolePlate, &taperedSolePlateThreshold);
    bool bNeedTaperedSolePlateTransverse = false; // if bearing recess slope exceeds "taperedSolePlateThreshold", a tapered bearing plate is required per LRFD 14.8.2
    bool bNeedTaperedSolePlateLongitudinal = false; // if bearing recess slope exceeds "taperedSolePlateThreshold", a tapered bearing plate is required per LRFD 14.8.2

    PierIndexType startPierIdx, endPierIdx;
    pBridge->GetGirderGroupPiers(girderKey.groupIndex, &startPierIdx, &endPierIdx);

    // we want the final configuration... that would be in the last interval
    IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

    // TRICKY: use adapter class to get correct reaction interfaces
    std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, intervalIdx, girderKey));

    INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

    for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
    {
        col = 0;

        if (!pForces->DoReportAtPier(pierIdx, girderKey))
        {
            // Don't report pier if information is not available
            continue;
        }

        (*pTable)(row, col++) << LABEL_PIER_EX(pBridge->IsAbutment(pierIdx), pierIdx);

        CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pierIdx, girderKey);

        Float64 slope1 = pBridge->GetSegmentSlope(segmentKey);
        (*pTable)(row, col++) << scalar.SetValue(slope1);

        pgsPointOfInterest poi;
        pgsTypes::PierFaceType pierFace(pgsTypes::Back);
        if (pierIdx == startPierIdx)
        {
            PoiList vPoi;
            GET_IFACE2(pBroker, IPointOfInterest, pPoi);
            pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vPoi);
            poi = vPoi.front();

            pierFace = pgsTypes::Ahead;
        }
        else if (pierIdx == endPierIdx)
        {
            PoiList vPoi;
            GET_IFACE2(pBroker, IPointOfInterest, pPoi);
            pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vPoi);
            poi = vPoi.front();
        }
        else
        {
            GET_IFACE2(pBroker, IPointOfInterest, pPoi);
            poi = pPoi->GetPierPointOfInterest(girderKey, pierIdx);
        }

        GET_IFACE2(pBroker, ICamber, pCamber);
        Float64 slope2 = pCamber->GetExcessCamberRotation(poi, pgsTypes::CreepTime::Max);
        (*pTable)(row, col++) << scalar.SetValue(slope2);

        GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
        const CBearingData2* pbd = pIBridgeDesc->GetBearingData(pierIdx, pierFace, girderKey.girderIndex);

        Float64 slope3 = slope1 + slope2;
        (*pTable)(row, col++) << scalar.SetValue(slope3);

        GET_IFACE2(pBroker, IGirder, pGirder);
        Float64 transverse_slope = pGirder->GetOrientation(segmentKey);
        (*pTable)(row, col++) << scalar.SetValue(transverse_slope);

        bNeedTaperedSolePlateLongitudinal = taperedSolePlateThreshold < fabs(slope3); // see lrfd 14.8.2
        bNeedTaperedSolePlateTransverse = taperedSolePlateThreshold < fabs(transverse_slope); // see lrfd 14.8.2

        Float64 L = max(pbd->RecessLength, pbd->Length); // don't allow recess to be shorter than bearing
        Float64 D = pbd->RecessHeight;
        Float64 D1 = D + L * slope3 / 2;
        Float64 D2 = D - L * slope3 / 2;

        (*pTable)(row, col++) << recess_dimension.SetValue(L);
        (*pTable)(row, col++) << recess_dimension.SetValue(D);
        (*pTable)(row, col++) << recess_dimension.SetValue(D1);
        (*pTable)(row, col++) << recess_dimension.SetValue(D2);

        row++;
    }

    *p << pTable << rptNewLine;
    *p << _T("* Orientation of the girder cross section with respect to vertical, zero indicates plumb and positive values indicate girder is rotated clockwise") << rptNewLine;
    *p << _T("** L is maximum of input bearing length and recess length") << rptNewLine;

    if (bCheckTaperedSolePlate && (bNeedTaperedSolePlateLongitudinal || bNeedTaperedSolePlateTransverse))
    {
        *p << bgcolor(rptRiStyle::Yellow);
        *p << _T("The inclination of the underside of the girder to the horizontal exceeds ") << taperedSolePlateThreshold << _T(" rad.");
        if (bNeedTaperedSolePlateLongitudinal && !bNeedTaperedSolePlateTransverse)
        {
            *p << _T(" in the longitudinal direction.");
        }
        else if (!bNeedTaperedSolePlateLongitudinal && bNeedTaperedSolePlateTransverse)
        {
            *p << _T(" in the transverse direction.");
        }
        else
        {
            ATLASSERT(bNeedTaperedSolePlateLongitudinal && bNeedTaperedSolePlateTransverse);
            *p << _T(" in the longitudinal and transverse directions.");
        }
        *p << _T(" Per LRFD 14.8.2 a tapered sole plate shall be used in order to provide a level surface.") << rptNewLine;
        *p << bgcolor(rptRiStyle::White);
    }
    *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("BearingRecessSlope.png")) << rptNewLine;

    return pChapter;
}
