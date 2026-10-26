///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
// DrawPierLayoutControl.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperPluginApp.h"
#include "PGSuperColors.h"
#include "DrawPierLayoutControl.h"

#include "XBeamDisplayObjectFactory.h"

#include <WBFLGenericBridge.h>

#include "XBeamSectionCut.h"

#include <WBFLGeometry/GeomHelpers.h>

#include <EAF\EAFDisplayUnits.h>

#include <IFace/Tools.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Alignment.h>
#include <PsgLib\PierData2.h>
#include <PsgLib\BridgeDescription2.h>

#define XBEAM_LINE_COLOR               GREY50
#define XBEAM_FILL_COLOR               GREY70

#define ROADWAY_DISPLAY_LIST_ID        0
#define CROSSBEAM_DISPLAY_LIST_ID      1
#define COLUMN_DISPLAY_LIST_ID         2
#define DIMENSIONS_DISPLAY_LIST_ID     3
#define SECTION_CUT_DISPLAY_LIST_ID    4

#define SECTION_CUT_ID                500

// The End/Section view of the pier is offset from the Elevation
// view by this amount.
const Float64 EndOffset = WBFL::Units::ConvertToSysUnits(10, WBFL::Units::Measure::Feet);

const WBFL::DManip::SelectionType g_selectionType = WBFL::DManip::SelectionType::None; // nothing is selectable, except for the section cut object

UINT WM_PIER_LAYOUT_POPOUT_CLOSED = ::RegisterWindowMessage(_T("PGSuper.PierLayoutPopoutClosed"));

IMPLEMENT_DYNAMIC(CDrawPierLayoutControl, CDisplayWnd) //I guess I manually create the object then?

CDrawPierLayoutControl::CDrawPierLayoutControl()
{
    m_pSource = nullptr;
    m_DisplayObjectID = 0;
}

CDrawPierLayoutControl::~CDrawPierLayoutControl()
{
}

BEGIN_MESSAGE_MAP(CDrawPierLayoutControl, CDisplayWnd)
    ON_WM_PAINT()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

void CDrawPierLayoutControl::OnDraw(CDC* pDC)
{
    CDisplayWnd::OnDraw(pDC);
}

void CDrawPierLayoutControl::CustomInit(IPierLayoutDataSource* pSource)
{
    CDisplayWnd::CustomInit();

    m_pSource = pSource;
    const CPierData2* pPier = m_pSource->GetPierData();

    auto doFactory = std::make_shared<CXBeamDisplayObjectFactory>();
    m_pDispMgr->AddDisplayObjectFactory(doFactory);

    m_pDispMgr->CreateDisplayList(ROADWAY_DISPLAY_LIST_ID);
    m_pDispMgr->CreateDisplayList(DIMENSIONS_DISPLAY_LIST_ID);
    m_pDispMgr->CreateDisplayList(CROSSBEAM_DISPLAY_LIST_ID);
    m_pDispMgr->CreateDisplayList(COLUMN_DISPLAY_LIST_ID);
    //m_pDispMgr->CreateDisplayList(SECTION_CUT_DISPLAY_LIST_ID);

    Float64 xMin = -pPier->GetXBeamLength() / 2.0;
    Float64 xLoc = 1.0;
    Float64 xMax = pPier->GetXBeamLength() / 2.0;
    m_pCutLoc = new CXBeamCutLocation(0,xLoc, xMax);

	UpdateDisplayObjects();
    ScaleToFit();
    
}


void CDrawPierLayoutControl::UpdateDisplayObjects()
{
    CDManipClientDC dc2(this);
    SetMappingMode(WBFL::DManip::MapMode::Isotropic, false);

    CWaitCursor wait;

    // Capture the current selection before blasting all the 
    // display objects
    auto vCurSel = m_pDispMgr->GetSelectedObjects();
    ATLASSERT(vCurSel.size() < 2);
    IDType curSel = INVALID_ID;
    IDType listID = INVALID_ID;
    if (vCurSel.size() == 1)
    {
        auto pDO = vCurSel[0];
        curSel = pDO->GetID();
        auto pDL = pDO->GetDisplayList();
        listID = pDL->GetID();
    }

    m_pDispMgr->ClearDisplayObjects();
    m_DisplayObjectID = 0;

    UpdateRoadwayDisplayObjects();
    UpdateXBeamDisplayObjects();
    UpdateColumnDisplayObjects();
    UpdateDimensionsDisplayObjects();
    //UpdateSectionCutDisplayObjects();

    // Re-instate the current selection
    if (curSel != INVALID_ID)
    {
        auto doSel = m_pDispMgr->FindDisplayObject(curSel, listID, WBFL::DManip::AccessType::ByID);
        m_pDispMgr->SelectObject(doSel, TRUE);
    }
}

std::shared_ptr<WBFL::DManip::iLineDisplayObject> CDrawPierLayoutControl::CreateLineDisplayObject(const WBFL::Geometry::Point2d& pntStart, const WBFL::Geometry::Point2d& pntEnd)
{
    auto doPntStart = WBFL::DManip::PointDisplayObject::Create();
    doPntStart->Visible(false);
    doPntStart->SetPosition(pntStart, false, false);
    auto connectable1 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPntStart);
    auto socket1 = connectable1->AddSocket(0, pntStart);

    auto doPntEnd = WBFL::DManip::PointDisplayObject::Create();
    doPntEnd->Visible(false);
    doPntEnd->SetPosition(pntEnd, false, false);
    auto connectable2 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPntEnd);
    auto socket2 = connectable2->AddSocket(0, pntEnd);

    auto doLine = WBFL::DManip::LineDisplayObject::Create();

    auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doLine);
    auto startPlug = connector->GetStartPlug();
    auto endPlug = connector->GetEndPlug();

    connectable1->Connect(0, WBFL::DManip::AccessType::ByID, startPlug);
    connectable2->Connect(0, WBFL::DManip::AccessType::ByID, endPlug);

    return doLine;
}

void CDrawPierLayoutControl::UpdateRoadwayDisplayObjects()
{
    const CPierData2* pPier = m_pSource->GetPierData();

    auto displayList = m_pDispMgr->FindDisplayList(ROADWAY_DISPLAY_LIST_ID);

    auto pBroker = EAFGetBroker();

	auto pierIdx = pPier->GetIndex();

    GET_IFACE2(pBroker, IBridge, pBridge);

    CComPtr<IAngle> angle;
	pBridge->GetPierSkew(pierIdx, &angle);
    Float64 skew;
	angle->get_Value(&skew);

    Float64 cos_skew = cos(skew);

    // Model a vertical line for the alignment
    // The alignment is at X = 0 in Pier coordinates
    Float64 X = 0;

    Float64 Xcl = pBridge->ConvertPierToCurbLineCoordinate(pierIdx, X);

    Float64 Ydeck = pBridge->GetElevation(pierIdx, Xcl); // deck elevation at alignment

    Float64 Yt = Ydeck + WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Feet); // add a little so it projects over the roadway surface
    WBFL::Geometry::Point2d pnt1(X, Yt);

    Float64 Yb = Yt - pBridge->GetMaxColumnHeight(pierIdx);
    WBFL::Geometry::Point2d pnt2(X, Yb);

    auto doAlignment = CreateLineDisplayObject(pnt1, pnt2);
    auto drawStrategy = doAlignment->GetDrawLineStrategy();
    auto drawAlignmentStrategy = std::dynamic_pointer_cast<WBFL::DManip::SimpleDrawLineStrategy>(drawStrategy);
    drawAlignmentStrategy->SetWidth(ALIGNMENT_LINE_WEIGHT);
    drawAlignmentStrategy->SetColor(ALIGNMENT_COLOR);
    drawAlignmentStrategy->SetLineStyle(WBFL::DManip::LineStyleType::Centerline);

    // Don't add the object to the display list here... do it at the end
    // We want it to be drawn on top so it has to go into the display list last
    //displayList->AddDisplayObject(doAlignment);

    // Draw the bridge line if different then the alignment
    std::shared_ptr<WBFL::DManip::iLineDisplayObject> doBridgeLine;
    Float64 BLO = pBridge->GetAlignmentOffset();
    if (!IsZero(BLO))
    {
        // Model a vertical line for the bridge line
        // Let X = BLO be at the alignment and Y = the alignment elevation
        Float64 X = BLO / cos_skew;
        pnt1.Move(X, Yt);
        pnt2.Move(X, Yb);

        doBridgeLine = CreateLineDisplayObject(pnt1, pnt2);
        auto drawStrategy = doBridgeLine->GetDrawLineStrategy();
        auto drawBridgeLineStrategy = std::dynamic_pointer_cast<WBFL::DManip::SimpleDrawLineStrategy>(drawStrategy);
        drawBridgeLineStrategy->SetWidth(BRIDGELINE_LINE_WEIGHT);
        drawBridgeLineStrategy->SetColor(BRIDGE_COLOR);
        drawBridgeLineStrategy->SetLineStyle(WBFL::DManip::LineStyleType::Centerline);

        //displayList->AddDisplayObject(doBridgeLine); // do this at the end
    }

    // Draw Roadway Surface

    Float64 pierStation = pPier->GetStation();

    GET_IFACE2(pBroker, IShapes, pShapes);

    CComPtr<IDirection> pierDirection;
    pBridge->GetPierDirection(pierIdx, &pierDirection);

    GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
    const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

    pgsTypes::SupportedDeckType deckType = pDeck->GetDeckType();
    if (deckType != pgsTypes::sdtNone)
    {
        auto dispObj = WBFL::DManip::PointDisplayObject::Create(m_DisplayObjectID++);

        CComPtr<IShape> shape;
        pShapes->GetSlabShape(pierStation, pierDirection, true/*include haunch*/, &shape);

        auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();

        strategy->SetShape(geomUtil::ConvertShape(shape));
        strategy->SetSolidLineColor(IsStructuralDeck(deckType) ? DECK_BORDER_COLOR : NONSTRUCTURAL_DECK_BORDER_COLOR);
        strategy->SetSolidFillColor(IsStructuralDeck(deckType) ? DECK_FILL_COLOR : NONSTRUCTURAL_DECK_FILL_COLOR);
        strategy->SetVoidLineColor(VOID_BORDER_COLOR);
        strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
        strategy->Fill(true);

        dispObj->SetDrawingStrategy(strategy);

        auto gravity_well = WBFL::DManip::ShapeGravityWellStrategy::Create();
        gravity_well->SetShape(geomUtil::ConvertShape(shape));

        dispObj->SetGravityWellStrategy(gravity_well);

        dispObj->SetSelectionType(g_selectionType);

        displayList->AddDisplayObject(dispObj);
        

        // Left Hand Barrier
        auto left_dispObj = WBFL::DManip::PointDisplayObject::Create();

        Float64 left_curb_offset = pBridge->GetLeftCurbOffset(pierIdx);
        Float64 right_curb_offset = pBridge->GetRightCurbOffset(pierIdx);

        CComPtr<IShape> left_shape;
        pShapes->GetLeftTrafficBarrierShape(pierStation, pierDirection, &left_shape);

        if (left_shape)
        {
            auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
            strategy->SetShape(geomUtil::ConvertShape(left_shape));
            strategy->SetSolidLineColor(BARRIER_BORDER_COLOR);
            strategy->SetSolidFillColor(BARRIER_FILL_COLOR);
            strategy->SetVoidLineColor(VOID_BORDER_COLOR);
            strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
            strategy->Fill(true);
            strategy->HasBoundingShape(false);

            left_dispObj->SetDrawingStrategy(strategy);

            displayList->AddDisplayObject(left_dispObj);
        }

        // Right Hand Barrier
        auto right_dispObj = WBFL::DManip::PointDisplayObject::Create();

        CComPtr<IShape> right_shape;
        pShapes->GetRightTrafficBarrierShape(pierStation, pierDirection, &right_shape);

        if (right_shape)
        {
            auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
            strategy->SetShape(geomUtil::ConvertShape(right_shape));
            strategy->SetSolidLineColor(BARRIER_BORDER_COLOR);
            strategy->SetSolidFillColor(BARRIER_FILL_COLOR);
            strategy->SetVoidLineColor(VOID_BORDER_COLOR);
            strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
            strategy->Fill(true);
            strategy->HasBoundingShape(false);

            right_dispObj->SetDrawingStrategy(strategy);

            displayList->AddDisplayObject(right_dispObj);
        }
    }

    displayList->AddDisplayObject(doAlignment);
    if (doBridgeLine)
    {
        displayList->AddDisplayObject(doBridgeLine);
    }
}

void CDrawPierLayoutControl::UpdateXBeamDisplayObjects()
{

    const CPierData2* pPier = m_pSource->GetPierData();

    auto displayList = m_pDispMgr->FindDisplayList(CROSSBEAM_DISPLAY_LIST_ID);

    auto pBroker = EAFGetBroker();

    GET_IFACE2(pBroker, IBridge, pBridge);

    // Model Upper Cross Beam (Elevation)
    WBFL::Geometry::Point2d point(0, 0);

    if (pBridge->GetPierType(*pPier) != ptExpansion)
    {
        auto doUpperXBeam = WBFL::DManip::PointDisplayObject::Create(m_DisplayObjectID++);
        doUpperXBeam->SetPosition(point, false, false);
        doUpperXBeam->SetSelectionType(g_selectionType);

        CComPtr<IShape> upperXBeamShape;
        pBridge->GetXBeamProfile(*pPier, pgsTypes::Stage::Stage2, &upperXBeamShape);

        auto upperXBeamDrawStrategy = WBFL::DManip::ShapeDrawStrategy::Create();
        upperXBeamDrawStrategy->SetShape(geomUtil::ConvertShape(upperXBeamShape));
        upperXBeamDrawStrategy->SetSolidLineColor(XBEAM_LINE_COLOR);
        upperXBeamDrawStrategy->SetSolidFillColor(XBEAM_FILL_COLOR);
        upperXBeamDrawStrategy->Fill(true);

        doUpperXBeam->SetDrawingStrategy(upperXBeamDrawStrategy);

        auto upper_xbeam_gravity_well = WBFL::DManip::ShapeGravityWellStrategy::Create();
        upper_xbeam_gravity_well->SetShape(geomUtil::ConvertShape(upperXBeamShape));
        doUpperXBeam->SetGravityWellStrategy(upper_xbeam_gravity_well);

        displayList->AddDisplayObject(doUpperXBeam);
    }

    // Model Lower Cross Beam (Elevation)
    auto doLowerXBeam = WBFL::DManip::PointDisplayObject::Create(m_DisplayObjectID++);
    doLowerXBeam->SetPosition(point, false, false);
    doLowerXBeam->SetSelectionType(g_selectionType);

    CComPtr<IShape> pLowerXBeamShape;
    pBridge->GetXBeamProfile(*pPier, pgsTypes::Stage::Stage1, &pLowerXBeamShape);

    auto lowerXBeamDrawStrategy = WBFL::DManip::ShapeDrawStrategy::Create();
    lowerXBeamDrawStrategy->SetShape(geomUtil::ConvertShape(pLowerXBeamShape));
    lowerXBeamDrawStrategy->SetSolidLineColor(XBEAM_LINE_COLOR);
    lowerXBeamDrawStrategy->SetSolidFillColor(XBEAM_FILL_COLOR);
    lowerXBeamDrawStrategy->Fill(true);

    doLowerXBeam->SetDrawingStrategy(lowerXBeamDrawStrategy);

    auto lower_xbeam_gravity_well = WBFL::DManip::ShapeGravityWellStrategy::Create();
    lower_xbeam_gravity_well->SetShape(geomUtil::ConvertShape(pLowerXBeamShape));
    doLowerXBeam->SetGravityWellStrategy(lower_xbeam_gravity_well);

    displayList->AddDisplayObject(doLowerXBeam);

    // Section Cut
    Float64 Lxb = pPier->GetXBeamLength();
    Lxb = pBridge->ConvertCrossBeamToPierCoordinate(*pPier, Lxb);

    Float64 XxbCut = pBridge->ConvertPierToCrossBeamCoordinate(*pPier, m_pCutLoc->GetCurrentCutLocation());

    auto doXBeamSection = WBFL::DManip::PointDisplayObject::Create(m_DisplayObjectID++);
    doXBeamSection->SetPosition(point, false, false);
    doXBeamSection->SetSelectionType(g_selectionType);

    CComPtr<IShape> xbeamShape;
    pBridge->GetXBeamShape(*pPier, pgsTypes::Stage2, XxbCut, &xbeamShape);
    CComQIPtr<IXYPosition> position(xbeamShape);
    position->Offset(EndOffset + Lxb, 0);

    auto xbeamDrawStrategy = WBFL::DManip::ShapeDrawStrategy::Create();
    xbeamDrawStrategy->SetShape(geomUtil::ConvertShape(xbeamShape));
    xbeamDrawStrategy->SetSolidLineColor(XBEAM_LINE_COLOR);
    xbeamDrawStrategy->SetSolidFillColor(XBEAM_FILL_COLOR);
    xbeamDrawStrategy->Fill(true);

    doXBeamSection->SetDrawingStrategy(xbeamDrawStrategy);

    auto xbeam_section_gravity_well = WBFL::DManip::ShapeGravityWellStrategy::Create();
    xbeam_section_gravity_well->SetShape(geomUtil::ConvertShape(xbeamShape));
    doXBeamSection->SetGravityWellStrategy(xbeam_section_gravity_well);

    displayList->AddDisplayObject(doXBeamSection);
}

void CDrawPierLayoutControl::UpdateColumnDisplayObjects()
{
    auto displayList = m_pDispMgr->FindDisplayList(COLUMN_DISPLAY_LIST_ID);

    auto pBroker = EAFGetBroker();

   // Create a function that represents the bottom of the cross beam
   // We will use it to make the top of the column match the bottom of the
   // cross beam.
    WBFL::Math::PiecewiseFunction fn;
    CComPtr<IPoint2dCollection> points;
    GET_IFACE2(pBroker, IBridge, pBridge);
    const CPierData2* pPier = m_pSource->GetPierData();

    Float64 H1L, H2L, H1R, H2R;
    Float64 X2L, X1L, X2R, X1R;
    pPier->GetXBeamDimensions(pgsTypes::stLeft, &H1L, &H2L, &X2L, &X1L);
    pPier->GetXBeamDimensions(pgsTypes::stRight, &H1R, &H2R, &X2R, &X1R);

    pBridge->GetBottomXBeamProfile(*pPier, &points);

    CComPtr<IEnumPoint2d> enumPoints;
    points->get__Enum(&enumPoints);
    CComPtr<IPoint2d> pnt;
    while (enumPoints->Next(1, &pnt, nullptr) != S_FALSE)
    {
        Float64 x, y;
        pnt->Location(&x, &y);
        fn.AddPoint(x, y);
        pnt.Release();
    }

    IndexType nColumns = pPier->GetColumnCount();
    for (IndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        const CColumnData& columnData = pPier->GetColumnData(colIdx);
		Float64 XxbCol = pBridge->GetColumnLocation(*pPier, colIdx);
        Float64 XpCol = pBridge->ConvertCrossBeamToPierCoordinate(*pPier, XxbCol);
        Float64 Ytop = pBridge->GetTopColumnElevation(*pPier, colIdx);
        Float64 Ybot = Ytop - columnData.GetColumnHeight();

        CColumnData::ColumnShapeType colShapeType = columnData.GetColumnShape();
        Float64 d1, d2;
        columnData.GetColumnDimensions(&d1, &d2);
        CColumnData::ColumnHeightMeasurementType columnHeightType = columnData.GetColumnHeightMeasurementType();
        Float64 H = columnData.GetColumnHeight();

        WBFL::Geometry::Point2d pntTop(XpCol, Ytop);
        WBFL::Geometry::Point2d pntBot(XpCol, Ybot);

        auto doTop = WBFL::DManip::PointDisplayObject::Create();
        doTop->Visible(false);
        doTop->SetPosition(pntTop, false, false);
        auto connectable1 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doTop);
        auto socket1 = connectable1->AddSocket(0, pntTop);

        auto doBot = WBFL::DManip::PointDisplayObject::Create();
        doBot->Visible(false);
        doBot->SetPosition(pntBot, false, false);
        auto connectable2 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doBot);
        auto socket2 = connectable2->AddSocket(0, pntBot);

        // Create the shape of the column
        auto columnShape = std::make_shared<WBFL::Geometry::Polygon>();
        Float64 X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11;


        X6 = pntTop.X();
        X1 = X6 - d1 * 0.5;
        X2 = X6 - d1 * 0.4;
        X3 = X6 - d1 * 0.3;
        X4 = X6 - d1 * 0.2;
        X5 = X6 - d1 * 0.1;
        X7 = X6 + d1 * 0.1;
        X8 = X6 + d1 * 0.2;
        X9 = X6 + d1 * 0.3;
        X10 = X6 + d1 * 0.4;
        X11 = X6 + d1 * 0.5;

        pgsTypes::OffsetMeasurementType refColMeasure;
        ColumnIndexType refColIdx;
        Float64 refColOffset;
        pPier->GetTransverseOffset(&refColIdx, &refColOffset, &refColMeasure);

        Float64 Y1 = fn.Evaluate(X1);
        Float64 Y2 = fn.Evaluate(X2);
        Float64 Y3 = fn.Evaluate(X3);
        Float64 Y4 = fn.Evaluate(X4);
        Float64 Y5 = fn.Evaluate(X5);
        Float64 Y6 = fn.Evaluate(X6);
        Float64 Y7 = fn.Evaluate(X7);
        Float64 Y8 = fn.Evaluate(X8);
        Float64 Y9 = fn.Evaluate(X9);
        Float64 Y10 = fn.Evaluate(X10);
        Float64 Y11 = fn.Evaluate(X11);

        columnShape->AddPoint(X1, Y1);
        columnShape->AddPoint(X2, Y2);
        columnShape->AddPoint(X3, Y3);
        columnShape->AddPoint(X4, Y4);
        columnShape->AddPoint(X5, Y5);
        columnShape->AddPoint(X6, Y6);
        columnShape->AddPoint(X7, Y7);
        columnShape->AddPoint(X8, Y8);
        columnShape->AddPoint(X9, Y9);
        columnShape->AddPoint(X10, Y10);
        columnShape->AddPoint(X11, Y11);
        columnShape->AddPoint(X11, Ybot);
        columnShape->AddPoint(X1, Ybot);

        auto doColumn = WBFL::DManip::PointDisplayObject::Create(m_DisplayObjectID++);
        doColumn->SetPosition(pntTop, false, false);
        doColumn->SetSelectionType(g_selectionType);

        auto drawColumnStrategy = WBFL::DManip::ShapeDrawStrategy::Create();
        doColumn->SetDrawingStrategy(drawColumnStrategy);

        drawColumnStrategy->SetShape(columnShape);
        drawColumnStrategy->SetSolidLineColor(XBEAM_LINE_COLOR);
        drawColumnStrategy->SetSolidFillColor(XBEAM_LINE_COLOR);
        drawColumnStrategy->Fill(true);

        displayList->AddDisplayObject(doColumn);
    }
}

void CDrawPierLayoutControl::UpdateDimensionsDisplayObjects()
{
    auto displayList = m_pDispMgr->FindDisplayList(DIMENSIONS_DISPLAY_LIST_ID);

    auto pBroker = EAFGetBroker();

    GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
    GET_IFACE2(pBroker, IBridge, pBridge);
    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
    const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

    Float64 tDeck = pDeck->GrossDepth;

    const CPierData2* pPier = m_pSource->GetPierData();
    pgsTypes::PierType pierType = pBridge->GetPierType(*pPier);

    Float64 H1L, H2L, H1R, H2R;
    Float64 X2L, X1L, X2R, X1R;
    pPier->GetXBeamDimensions(pgsTypes::stLeft, &H1L, &H2L, &X2L, &X1L);
    pPier->GetXBeamDimensions(pgsTypes::stRight, &H1R, &H2R, &X2R, &X1R);

    Float64 Dd, Hu;
    pBridge->GetUpperXBeamDimensions(*pPier, &Hu, &Dd);

    CComPtr<IPoint2dCollection> topUpperXBeamProfile, topLowerXBeamProfile, bottomXBeamProfile;
    pBridge->GetUpperXBeamProfile(*pPier, &topUpperXBeamProfile);
    pBridge->GetLowerXBeamProfile(*pPier, &topLowerXBeamProfile);
    pBridge->GetBottomXBeamProfile(*pPier, &bottomXBeamProfile);

    // Upper Cross Beam - Top Left
    CComPtr<IPoint2d> pnt;
    topUpperXBeamProfile->get_Item(0, &pnt);
    WBFL::Geometry::Point2d uxbTL(geomUtil::GetPoint(pnt));

    // we want all vertical dimensions on the left side to be at this x-location
    Float64 Xl;
    pnt->get_X(&Xl);

    // Upper Cross Beam - Bottom Left (Lower Cross Beam - Top Left)
    pnt.Release();
    topLowerXBeamProfile->get_Item(0, &pnt);
    Float64 ublc;
    pnt->get_X(&ublc);
    WBFL::Geometry::Point2d uxbBL(geomUtil::GetPoint(pnt));
    WBFL::Geometry::Point2d origin(geomUtil::GetPoint(pnt));
    uxbBL.X() = Xl;

    // Lower Cross Beam - Bottom Left
    pnt.Release();
    bottomXBeamProfile->get_Item(0, &pnt);
    Float64 lblc;
    pnt->get_X(&lblc);
    WBFL::Geometry::Point2d lxbBL(geomUtil::GetPoint(pnt));
    lxbBL.X() = Xl;

    // Upper Cross Beam - Top Right
    pnt.Release();
    IndexType nPoints;
    topUpperXBeamProfile->get_Count(&nPoints);
    topUpperXBeamProfile->get_Item(nPoints - 1, &pnt);
    WBFL::Geometry::Point2d uxbTR(geomUtil::GetPoint(pnt));

    // we want all vertical dimensions on the right side to be at this x-location
    Float64 Xr;
    pnt->get_X(&Xr);

    // Upper Cross Beam - Bottom Right (Lower Cross Beam - Top Right)
    pnt.Release();
    topLowerXBeamProfile->get_Count(&nPoints);
    topLowerXBeamProfile->get_Item(nPoints - 1, &pnt);
    Float64 ubrc;
    pnt->get_X(&ubrc);
    WBFL::Geometry::Point2d uxbBR(geomUtil::GetPoint(pnt));
    uxbBR.X() = Xr;

    // Lower Cross Beam - Bottom Right
    pnt.Release();
    bottomXBeamProfile->get_Count(&nPoints);
    bottomXBeamProfile->get_Item(nPoints - 1, &pnt);
    Float64 lbrc;
    pnt->get_X(&lbrc);
    WBFL::Geometry::Point2d lxbBR(geomUtil::GetPoint(pnt));
    lxbBR.X() = Xr;

    // Height of upper cross beam
    if (pierType != pgsTypes::pctExpansion)
    {
        BuildDimensionLine(displayList, uxbBL, uxbTL);
        BuildDimensionLine(displayList, uxbTR, uxbBR);
    }

    // Height of lower cross beam
    BuildDimensionLine(displayList, lxbBL, uxbBL); // H1L Dimension
    BuildDimensionLine(displayList, uxbBR, lxbBR); // H1R Dimension

    Float64 Ybot = lxbBL.Y();
    WBFL::Geometry::Point2d blc(lblc - X1L, Ybot);
    WBFL::Geometry::Point2d x1l(lblc, Ybot);
    BuildDimensionLine(displayList, x1l, blc);
    WBFL::Geometry::Point2d brc(lbrc + X1R, Ybot);
    WBFL::Geometry::Point2d x1r(lbrc, Ybot);
    BuildDimensionLine(displayList, brc, x1r);

    // Get the center points for the arcs at column centers
    ColumnIndexType nColumns = pPier->GetColumnCount();

    const auto& pierLayoutType = pPier->GetPierLayoutType();

	if (pierLayoutType == pgsTypes::pltCommon)
    {
        // Lower cross beam bottom taper, vertical dimensions
        WBFL::Geometry::Point2d lxbBLC, lxbBRC;
        lxbBLC.Move(lxbBL);
        lxbBLC.Offset(0, -H2L);
        lxbBRC.Move(lxbBR);
        lxbBRC.Offset(0, -H2R);

        BuildDimensionLine(displayList, lxbBLC, lxbBL); // H2L Dimension
        BuildDimensionLine(displayList, lxbBR, lxbBRC); // H2R Dimension

        // Lower cross beam bottom taper, horizontal dimensions
        pnt.Release();
        topLowerXBeamProfile->get_Item(0, &pnt);
        WBFL::Geometry::Point2d lxbBL1, lxbBR1;
        lxbBL1.Move(geomUtil::GetPoint(pnt));
        lxbBL1.Offset(0, -H1L - H2L);

        pnt.Release();
        topLowerXBeamProfile->get_Count(&nPoints);
        topLowerXBeamProfile->get_Item(nPoints - 1, &pnt);
        lxbBR1.Move(geomUtil::GetPoint(pnt));
        lxbBR1.Offset(0, -H1R - H2R);

        WBFL::Geometry::Point2d lxbBL2, lxbBR2;
        Float64 y;
        std::tie(Xl, y) = lxbBL1.GetLocation(); // TRICKY: changing Xl to now be the x-location of left dimensions for the columns
        lxbBL2.Move(Xl + X2L, y);

        std::tie(Xr, y) = lxbBR1.GetLocation();
        lxbBR2.Move(Xr - X2R, y); // TRICKY: changing Xr to now be the x-location of right dimensions for the columns

        // Horizontal Cross Beam Dimensions
        BuildDimensionLine(displayList, lxbBL2, lxbBL1); // X2L Dimension
        BuildDimensionLine(displayList, lxbBR1, lxbBR2); // X2R Dimension
    }
    else if (pierLayoutType == pgsTypes::pltScalloped)
    {
        //Radius and depth of bottom scallop
        // R = radius of bottom scallop arcs (same for both left and right)
        // D = depth of lower cross beam at column center

        for (ColumnIndexType colIdx = 0;
            colIdx + 1 < nColumns; colIdx++)
        {
            Float64 X1 = pBridge->GetColumnLocation(*pPier, colIdx);
            Float64 X2 = pBridge->GetColumnLocation(*pPier, colIdx + 1);

            Float64 Xmid = (X1 + X2) / 2.0;

            Float64 XpMid =
                pBridge->ConvertCrossBeamToPierCoordinate(*pPier, Xmid);

            Float64 XpCol =
                pBridge->ConvertCrossBeamToPierCoordinate(*pPier, X1);

            CComPtr<IShape> shape1;
            pBridge->GetXBeamShape(
                *pPier,
                pgsTypes::Stage1,
                Xmid,
                &shape1);

            CComQIPtr<IXYPosition> position1(shape1);

            CComPtr<IShape> shape2;
            pBridge->GetXBeamShape(
                *pPier,
                pgsTypes::Stage1,
                X1,
                &shape2);

            CComQIPtr<IXYPosition> position2(shape2);

            CComPtr<IPoint2d> pntTop1;
            CComPtr<IPoint2d> pntBot1;

            position1->get_LocatorPoint(lpTopCenter, &pntTop1);
            position1->get_LocatorPoint(lpBottomCenter, &pntBot1);

            Float64 Ytop1, Ybot1;
            pntTop1->get_Y(&Ytop1);
            pntBot1->get_Y(&Ybot1);

            WBFL::Geometry::Point2d top1(XpMid, Ytop1);
            WBFL::Geometry::Point2d bottom1(XpMid, Ybot1);

            BuildDimensionLine(displayList, top1, bottom1);

            CComPtr<IPoint2d> pntTop2;
            CComPtr<IPoint2d> pntBot2;

            position2->get_LocatorPoint(lpTopCenter, &pntTop2);
            position2->get_LocatorPoint(lpBottomCenter, &pntBot2);

            Float64 Ytop2, Ybot2;
            pntTop2->get_Y(&Ytop2);
            pntBot2->get_Y(&Ybot2);

            WBFL::Geometry::Point2d top2(XpCol, Ytop2);
            WBFL::Geometry::Point2d bottom2(XpCol, Ybot2);

            BuildDimensionLine(displayList, top2, bottom2);
        }

        // Depth dimension at the last column 
        Float64 Xlast = pBridge->GetColumnLocation(*pPier, nColumns - 1);

        Float64 XpLast =
            pBridge->ConvertCrossBeamToPierCoordinate(*pPier, Xlast);

        CComPtr<IShape> shapeLast;
        pBridge->GetXBeamShape(
            *pPier,
            pgsTypes::Stage1,
            Xlast,
            &shapeLast);

        CComQIPtr<IXYPosition> positionLast(shapeLast);

        CComPtr<IPoint2d> pntTopLast;
        CComPtr<IPoint2d> pntBotLast;

        positionLast->get_LocatorPoint(lpTopCenter, &pntTopLast);
        positionLast->get_LocatorPoint(lpBottomCenter, &pntBotLast);

        Float64 YtopLast, YbotLast;
        pntTopLast->get_Y(&YtopLast);
        pntBotLast->get_Y(&YbotLast);

        WBFL::Geometry::Point2d topLast(XpLast, YtopLast);
        WBFL::Geometry::Point2d bottomLast(XpLast, YbotLast);

        BuildDimensionLine(displayList, topLast, bottomLast);


        GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

        for (ColumnIndexType arcIdx = 0; arcIdx <= nColumns; ++arcIdx)
        {
            Float64 Xleft, Xright;

            if (arcIdx == 0)
            {
                // Left overhang
                Xleft = pBridge->ConvertPierToCrossBeamCoordinate(
                    *pPier, lblc);

                Xright = pBridge->GetColumnLocation(*pPier, 0);
            }
            else if (arcIdx == nColumns)
            {
                // Right overhang
                Xleft = pBridge->GetColumnLocation(
                    *pPier, nColumns - 1);

                Xright = pBridge->ConvertPierToCrossBeamCoordinate(
                    *pPier, lbrc);
            }
            else
            {
                // Between adjacent columns
                Xleft = pBridge->GetColumnLocation(*pPier, arcIdx - 1);
                Xright = pBridge->GetColumnLocation(*pPier, arcIdx);
            }

            const Float64 Xmid = (Xleft + Xright) / 2.0;

            // Obtain the bottom surface elevations at the chord endpoints.
            auto GetBottomY = [&](Float64 Xxb) -> Float64
                {
                    CComPtr<IShape> shape;

                    pBridge->GetXBeamShape(
                        *pPier,
                        pgsTypes::Stage1,
                        Xxb,
                        &shape);

                    CComQIPtr<IXYPosition> position(shape);

                    CComPtr<IPoint2d> pntBot;
                    position->get_LocatorPoint(lpBottomCenter, &pntBot);

                    Float64 Y;
                    pntBot->get_Y(&Y);

                    return Y;
                };

            const Float64 Yleft = GetBottomY(Xleft);
            const Float64 Yright = GetBottomY(Xright);

            // Midpoint of the chord, accounting for inclination.
            const Float64 Ychord = (Yleft + Yright) / 2.0;

            const Float64 XpMid =
                pBridge->ConvertCrossBeamToPierCoordinate(*pPier, Xmid);

            // Small offset below the chord.
            WBFL::Geometry::Point2d ppRad(XpMid, Ychord - 0.5);

            const Float64 radius = arcIdx == 0
                ? pPier->GetXBeamOverhangRadius(pgsTypes::stLeft)
                : arcIdx == nColumns
                    ? pPier->GetXBeamOverhangRadius(pgsTypes::stRight)
                    : pPier->GetColumnScallopRadius(arcIdx - 1);
            const CString strRadius = FormatDimension(radius, pDisplayUnits->GetSpanLengthUnit());

            CString strRadiusLabel;
            if (arcIdx == 0)
                strRadiusLabel.Format(_T("RL = %s"), strRadius.GetString());
            else if (arcIdx == nColumns)
                strRadiusLabel.Format(_T("RR = %s"), strRadius.GetString());
            else
                strRadiusLabel.Format(_T("R%d = %s"), arcIdx, strRadius.GetString());

            auto doRadiusLabel = WBFL::DManip::TextBlock::Create();
            doRadiusLabel->SetText(strRadiusLabel);
            doRadiusLabel->SetBkMode(TRANSPARENT);
            doRadiusLabel->SetTextAlign(TA_TOP | TA_CENTER);
            doRadiusLabel->SetPosition(ppRad);

            displayList->AddDisplayObject(doRadiusLabel);
        }
    }
    else if (pierLayoutType == pgsTypes::pltUserDefined)
    {
        // Function representing the top of the lower cross beam
        WBFL::Math::PiecewiseFunction fnTop;

        CComPtr<IEnumPoint2d> enumPoints;
        topLowerXBeamProfile->get__Enum(&enumPoints);

        CComPtr<IPoint2d> pntTop;
        while (enumPoints->Next(1, &pntTop, nullptr) != S_FALSE)
        {
            Float64 x, y;
            pntTop->Location(&x, &y);
            fnTop.AddPoint(x, y);
            pntTop.Release();
        }

        Float64 origX = pBridge->ConvertCrossBeamToPierCoordinate(*pPier, 0.0);
        Float64 origY = fnTop.Evaluate(origX) - H1L;
        WBFL::Geometry::Point2d origin(origX, origY);

        auto doOrigin = WBFL::DManip::PointDisplayObject::Create();
        doOrigin->SetPosition(origin, false, false);
        displayList->AddDisplayObject(doOrigin);

        CString strBlcLabel = _T("(0, 0)");

        auto doBlcLabel = WBFL::DManip::TextBlock::Create();
        doBlcLabel->SetText(strBlcLabel);
        doBlcLabel->SetBkMode(TRANSPARENT);
        doBlcLabel->SetTextAlign(TA_TOP | TA_CENTER);
        doBlcLabel->SetPosition(origin);
        displayList->AddDisplayObject(doBlcLabel);

        for (const auto& pnt : pPier->GetPierPointData())
        {
            Float64 Xxb = pnt.Get_X();

            Float64 Xp = pBridge->ConvertCrossBeamToPierCoordinate(*pPier, Xxb);

            const Float64 Ytop = fnTop.Evaluate(Xp);

            const Float64 Ypoint = Ytop - pnt.Get_Y();

            WBFL::Geometry::Point2d pp(Xp, Ypoint);

            GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

            CString strPntLabel;
            strPntLabel.Format(_T("(%s,%s)"), ::FormatDimension(Xxb, pDisplayUnits->GetSpanLengthUnit()), ::FormatDimension(pnt.Get_Y(), pDisplayUnits->GetSpanLengthUnit()));

            auto doPntLabel = WBFL::DManip::TextBlock::Create();
            doPntLabel->SetText(strPntLabel);
            doPntLabel->SetBkMode(TRANSPARENT);
            doPntLabel->SetTextAlign(TA_TOP | TA_CENTER);
            doPntLabel->SetPosition(pp);
            displayList->AddDisplayObject(doPntLabel);

            auto doPoint = WBFL::DManip::PointDisplayObject::Create();
            doPoint->SetPosition(pp, false, false);
            displayList->AddDisplayObject(doPoint);
        }

    }

    // Column Dimensions

    // Column Height
    Float64 YbotColMin = DBL_MAX;
    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        Float64 XxbCol = pBridge->GetColumnLocation(*pPier, colIdx);
        Float64 XpCol = pBridge->ConvertCrossBeamToPierCoordinate(*pPier, XxbCol);
        Float64 YtopCol = pBridge->GetTopColumnElevation(*pPier, colIdx);
        Float64 YbotCol = pBridge->GetBottomColumnElevation(*pPier, colIdx);

        WBFL::Geometry::Point2d pntTop(XpCol, YtopCol);
        WBFL::Geometry::Point2d pntBot(XpCol, YbotCol);
        BuildDimensionLine(displayList, pntTop, pntBot); // Column Height

        YbotColMin = Min(YbotColMin, YbotCol);
    }

    // Column Spacing (starts with left cross beam cantilever and
    // than proceeds with the spacing between columns at their base)
    // create the dimension line with rightpt,leftpt so the text comes
    // out on the correct side
    WBFL::Geometry::Point2d pntLeft(ublc, YbotColMin);
    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        Float64 XxbCol = pBridge->GetColumnLocation(*pPier, colIdx);
        Float64 XpCol = pBridge->ConvertCrossBeamToPierCoordinate(*pPier, XxbCol);

        WBFL::Geometry::Point2d pntRight(XpCol, YbotColMin);

        BuildDimensionLine(displayList, pntRight, pntLeft); // first time this is OHL, then S

        pntLeft = pntRight;
    }

    // Right cross beam cantilever
    WBFL::Geometry::Point2d pntRight(ubrc, YbotColMin);
    BuildDimensionLine(displayList, pntRight, pntLeft); // OHR Dimension


    // Reference column to alignment/bridge line dimension 

    pgsTypes::OffsetMeasurementType refColMeasure;
    ColumnIndexType refColIdx;
    Float64 refColOffset;
    pPier->GetTransverseOffset(&refColIdx, &refColOffset, &refColMeasure);

    Float64 XxbRefCol = pBridge->GetColumnLocation(*pPier, refColIdx);
    Float64 XpRefCol =
    pBridge->ConvertCrossBeamToPierCoordinate(*pPier, XxbRefCol);

    // Alignment/bridge line is at X = 0 in pier coordinates
    Float64 XpAlignment = 0.0;

    // Match the lower end of the alignment/bridge-line marker drawn in UpdateRoadwayDisplayObjects.
    const Float64 alignmentXcl = pBridge->ConvertPierToCurbLineCoordinate(pPier->GetIndex(), 0.0);
    const Float64 alignmentBottom = pBridge->GetElevation(pPier->GetIndex(), alignmentXcl)
        + WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Feet)
        - pBridge->GetMaxColumnHeight(pPier->GetIndex());
    // Place the reference-column dimension six inches below that marker.
    const Float64 Ydim = alignmentBottom
        - WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Feet);

    WBFL::Geometry::Point2d pntRefCol(XpRefCol, Ydim);
    WBFL::Geometry::Point2d pntAlignment(XpAlignment, Ydim);

    BuildDimensionLine(displayList, pntRefCol, pntAlignment);

    //
    // Cross section dimensions
    //

	Float64 Lxb = pPier->GetXBeamLength();
    Lxb = pBridge->ConvertCrossBeamToPierCoordinate(*pPier, Lxb);

    Float64 Z = pBridge->ConvertPierToCrossBeamCoordinate(*pPier, Lxb/2.0);

    if (pBridge->GetPierType(*pPier) != pgsTypes::pctExpansion)
    {
        // Upper Cross Beam (End View)
        CComPtr<IShape> upperXBeamShape;
		pBridge->GetUpperXBeamShape(*pPier, Lxb / 2.0, &upperXBeamShape);

        CComQIPtr<ICompositeShape> composite(upperXBeamShape);
        CComPtr<IShape> shape;
        if (composite)
        {
            CComPtr<ICompositeShapeItem> upperShapeItem;
            composite->get_Item(1, &upperShapeItem);

            upperShapeItem->get_Shape(&shape);
        }
        else
        {
            shape = upperXBeamShape;
        }

        CComQIPtr<IXYPosition> position(shape);
        CComPtr<IPoint2d> pnt_left, pnt_right;
        position->get_LocatorPoint(lpTopLeft, &pnt_left);
        position->get_LocatorPoint(lpTopRight, &pnt_right);
        pnt_left->Offset(EndOffset + Lxb, 0);
        pnt_right->Offset(EndOffset + Lxb, 0);
        BuildDimensionLine(displayList, geomUtil::GetPoint(pnt_left), geomUtil::GetPoint(pnt_right));
    }

    // Lower Cross Beam (End View)
    CComPtr<IShape> lowerXBeamShape;
    pBridge->GetLowerXBeamShape(*pPier, Lxb / 2.0, &lowerXBeamShape);

    CComQIPtr<IXYPosition> position(lowerXBeamShape);
    CComPtr<IPoint2d> pnt_left, pnt_right;
    position->get_LocatorPoint(lpBottomLeft, &pnt_left);
    position->get_LocatorPoint(lpBottomRight, &pnt_right);
    pnt_left->Offset(EndOffset + Lxb, 0);
    pnt_right->Offset(EndOffset + Lxb, 0);
    BuildDimensionLine(displayList, geomUtil::GetPoint(pnt_right), geomUtil::GetPoint(pnt_left));

    // Curb-to-curb width

    // This is a basic dimension generated from the curb line offset. It does not take
    // into account geometric effects of the roadway curvature
	Float64 skew;
    pBridge->GetSkewAngle(pPier->GetStation(), pPier->GetOrientation(), &skew);

    Float64 LCO = pBridge->GetLeftInteriorCurbOffset(pPier->GetIndex());
    Float64 RCO = pBridge->GetRightInteriorCurbOffset(pPier->GetIndex());

    Float64 Ylc = pBridge->GetElevation(pPier->GetIndex(), 0);
    Float64 Yrc = pBridge->GetElevation(pPier->GetIndex(), RCO - LCO);
    Float64 Y = Max(Ylc, Yrc);

    LCO /= cos(skew); // skew adjust
    WBFL::Geometry::Point2d pntLC(LCO, Y);

    RCO /= cos(skew); // skew adjust
    WBFL::Geometry::Point2d pntRC(RCO, Y);

    BuildDimensionLine(displayList, pntLC, pntRC, false /*don't omit if zero distance*/);
}

void CDrawPierLayoutControl::BuildDimensionLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, bool bOmitForZeroDistance)
{
    Float64 distance = toPoint.Distance(fromPoint);
    if (!IsZero(distance) || !bOmitForZeroDistance)
    {
        BuildDimensionLine(pDL, fromPoint, toPoint, distance);
    }
}

void CDrawPierLayoutControl::BuildDimensionLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, Float64 dimension)
{
    // put points at locations and make them sockets
    auto from_rep = WBFL::DManip::PointDisplayObject::Create(m_DisplayObjectID++);
    from_rep->SetPosition(fromPoint, false, false);
    auto from_connectable = std::shared_ptr<WBFL::DManip::iConnectable>(from_rep);
    auto from_socket = from_connectable->AddSocket(0, fromPoint);
    from_rep->Visible(false);
    pDL->AddDisplayObject(from_rep);

    auto to_rep = WBFL::DManip::PointDisplayObject::Create(m_DisplayObjectID++);
    to_rep->SetPosition(toPoint, false, false);
    auto to_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(to_rep);
    auto to_socket = to_connectable->AddSocket(0, toPoint);
    to_rep->Visible(false);
    pDL->AddDisplayObject(to_rep);

    // Create the dimension line object
    auto dimLine = WBFL::DManip::DimensionLine::Create(m_DisplayObjectID++);

    dimLine->SetArrowHeadStyle(WBFL::DManip::ArrowHeadStyleType::Filled);

    // Attach connector (the dimension line) to the sockets 
    auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(dimLine);
    auto startPlug = connector->GetStartPlug();
    auto endPlug = connector->GetEndPlug();

    from_socket->Connect(startPlug);
    to_socket->Connect(endPlug);

    // Create the text block and attach it to the dimension line
    auto textBlock = WBFL::DManip::TextBlock::Create();

    // Format the dimension text
    auto pBroker = EAFGetBroker();

    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    CString strDimension = FormatDimension(dimension, pDisplayUnits->GetSpanLengthUnit());

    textBlock->SetText(strDimension);
    textBlock->SetBkMode(TRANSPARENT);

    dimLine->SetTextBlock(textBlock);


    pDL->AddDisplayObject(dimLine);
}

void CDrawPierLayoutControl::UpdateSectionCutDisplayObjects()
{
    auto pBroker = EAFGetBroker();

    auto display_list = m_pDispMgr->FindDisplayList(SECTION_CUT_DISPLAY_LIST_ID);

    auto factory = m_pDispMgr->GetDisplayObjectFactory(0);

    auto disp_obj = factory->Create(CXBeamSectionCutDisplayImpl::ms_Format, nullptr);

    auto sink = disp_obj->GetEventSink();

    disp_obj->SetSelectionType(WBFL::DManip::SelectionType::All);

    auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(disp_obj);
    point_disp->SetMaxTipWidth(TOOLTIP_WIDTH);
    point_disp->SetToolTipText(_T("Drag me to move section cut.\r\nDouble click to enter the cut location\r\nPress CTRL + -> to move ahead\r\nPress CTRL + <- to move back"));
    point_disp->SetTipDisplayTime(TOOLTIP_DURATION);

    auto section_cut_strategy = std::dynamic_pointer_cast<iXBeamSectionCutDrawStrategy>(sink);
    const CPierData2* pPier = m_pSource->GetPierData();
    PierIndexType pierIdx = pPier->GetIndex();
    section_cut_strategy->Init(pierIdx, point_disp, m_pCutLoc);
    section_cut_strategy->SetColor(CUT_COLOR);

    point_disp->SetID(SECTION_CUT_ID);

    display_list->Clear();
    display_list->AddDisplayObject(disp_obj);
}

BOOL CDrawPierLayoutControl::CreatePopout(IPierLayoutDataSource* pSource, CWnd* pOwner)
{
    CString className = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(nullptr, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        nullptr);

    // Use WS_EX_APPWINDOW so this is treated as a normal app window (not a tool window).
    DWORD dwExStyle = WS_EX_APPWINDOW;
    // Use WS_OVERLAPPEDWINDOW (or WS_POPUPWINDOW|WS_CAPTION|WS_SYSMENU) for a regular top-level window
    DWORD dwStyle = WS_OVERLAPPEDWINDOW/* | WS_VISIBLE*/;

    HWND hOwner = (pOwner != nullptr) ? pOwner->GetSafeHwnd() : nullptr;
    m_hPopoutOwner = hOwner;

    BOOL ok = CreateEx(
        dwExStyle,
        className,
        _T("Pier Layout"),
        dwStyle,
        CRect(100, 100, 1000, 800),
        CWnd::FromHandle(hOwner), // supply the owner window if provided
        0);

    if (!ok)
        return FALSE;

    // Initialize control after creation
    CustomInit(pSource);

    // Optionally keep the popout above the owner without making it topmost for all apps:
    // if (hOwner) SetWindowPos(hOwner, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);

    ShowWindow(SW_SHOW);
    UpdateWindow();

    return TRUE;
}

void CDrawPierLayoutControl::OnPaint()
{

	CDisplayWnd::OnPaint();

}

void CDrawPierLayoutControl::OnDestroy()
{
    // The native owner can be the property sheet, rather than the layout page.
    if (::IsWindow(m_hPopoutOwner))
        ::SendMessage(m_hPopoutOwner, WM_PIER_LAYOUT_POPOUT_CLOSED, 0, 0);
    m_hPopoutOwner = nullptr;
    CDisplayWnd::OnDestroy();
}
