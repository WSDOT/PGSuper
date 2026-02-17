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
#include <Reporting\SectionPropertiesChapterBuilder.h>
#include <Reporting\SectionPropertiesReportSpecification.h>
#include <IFace/Bridge.h>
#include <IFace/Project.h>
#include <IFace/Intervals.h>
#include <IFace/Limits.h>
#include <IFace/PointOfInterest.h>
#include <PsgLib\BridgeDescription2.h>
#include <psgLib/GirderLibraryEntry.h>
#include <AgentTools.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFApp.h>
#include <Graphing/GraphXY.h>
#include <atlimage.h>


CSectionPropertiesChapterBuilder::CSectionPropertiesChapterBuilder(bool bSelect,bool simplifiedVersion) :
CPGSuperChapterBuilder(bSelect),
m_SimplifiedVersion(simplifiedVersion)
{
}

LPCTSTR CSectionPropertiesChapterBuilder::GetName() const
{
   return TEXT("Section Properties");
}

void CSectionPropertiesChapterBuilder::ExportCoordinatesToExcel(
    const std::vector<std::vector<std::pair<Float64, Float64>>>& primaryPoints,
    const std::_tstring& outputFilePath) const
{
    std::_tofstream file(outputFilePath);

    if (!file.is_open())
    {
        return;
    }

    CEAFApp* pApp = EAFGetApp();
    const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();

    for (IndexType i = 0; i < primaryPoints.size(); i++)
    {
        if (i == 0)
        {
            file << _T("GIRDER OUTLINE") << std::endl;
        }
        else if (i <= 2)
        {
            file << _T("EXTERNAL VOID ") << i << std::endl;
        }
        else
        {
            file << _T("INTERNAL VOID ") << (i - 2) << std::endl;
        }

        file << _T("X,Y") << std::endl;
        for (auto iter = primaryPoints[i].rbegin(); iter != primaryPoints[i].rend(); ++iter)
        {
            Float64 x = WBFL::Units::ConvertFromSysUnits(iter->first, pDispUnits->ComponentDim.UnitOfMeasure);
            Float64 y = WBFL::Units::ConvertFromSysUnits(iter->second, pDispUnits->ComponentDim.UnitOfMeasure);
            file << x << _T(",") << y << std::endl;
        }
        file << std::endl;
    }

    file.close();
}

rptRcTable* CSectionPropertiesChapterBuilder::WriteXSTable2(std::shared_ptr<WBFL::EAF::Broker> pBroker,
    pgsTypes::SectionPropertyType spType,
    const pgsPointOfInterest& poi,
    IntervalIndexType intervalIdx,
    std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
    USES_CONVERSION;
    GET_IFACE2(pBroker, IIntervals, pIntervals);
    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, ISectionProperties, pSectProp);

	const auto& segmentKey = poi.GetSegmentKey();
    IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
    IntervalIndexType firstCompositeDeckIntervalIdx = pIntervals->GetFirstCompositeDeckInterval();
    IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
    IntervalIndexType lastTendonStressingIntervalIdx = pIntervals->GetLastGirderTendonStressingInterval(segmentKey);

    std::_tostringstream os;
    os << "Interval " << LABEL_INTERVAL(intervalIdx) << _T(" : ") << pIntervals->GetDescription(intervalIdx);

    if (spType == pgsTypes::sptGross)
    {
        if (intervalIdx < firstCompositeDeckIntervalIdx)
        {
            os << _T(" - Gross non-composite properties");
        }
        else
        {
            os << _T(" - Gross composite properties");
        }
    }
    else if (spType == pgsTypes::sptGrossNoncomposite)
    {
        os << _T(" - Gross non-composite properties");
    }
    else if (spType == pgsTypes::sptTransformedNoncomposite)
    {
        os << _T(" - Transformed non-composite properties");
    }
    else if (spType == pgsTypes::sptNetGirder)
    {
        if (intervalIdx < firstCompositeDeckIntervalIdx)
        {
            os << _T(" - Net non-composite properties");
        }
        else
        {
            os << _T(" - Net composite properties");
        }
    }
    else if (spType == pgsTypes::sptTransformed)
    {
        if (intervalIdx < firstCompositeDeckIntervalIdx)
        {
            os << _T(" - Transformed non-composite properties");
        }
        else
        {
            os << _T(" - Transformed composite properties");
        }
    }

    os << _T(" for ") << pgsGirderLabel::GetSegmentLabel(segmentKey);

    bool bIsCompositeDeck = pBridge->IsCompositeDeck();
    bool bAsymmetricGirders = pBridge->HasAsymmetricGirders();
    pgsTypes::HaunchAnalysisSectionPropertiesType haunchAType = pSectProp->GetHaunchAnalysisSectionPropertiesType();


    ColumnIndexType nCol;
    if (intervalIdx < erectionIntervalIdx)
    {
        nCol = 14;
        if (bAsymmetricGirders)
        {
            nCol++;
        }
    }
    else if (pBridge->GetDeckType() != pgsTypes::sdtNone && bIsCompositeDeck && firstCompositeDeckIntervalIdx <= intervalIdx)
    {
        if (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite)
        {
            nCol = (firstCompositeDeckIntervalIdx == intervalIdx ? 13 : 12);
        }
        else
        {
            if (lastTendonStressingIntervalIdx != INVALID_INDEX && firstCompositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
            {
                nCol = 18;
            }
            else
            {
                nCol = 16;
            }
        }

        if (pgsTypes::hspZeroHaunch != haunchAType && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed))
        {
            // assumed haunch depth
            nCol++;
        }
    }
    else
    {
        nCol = 13;
    }

    rptRcTable* xs_table = rptStyleManager::CreateDefaultTable(nCol, os.str().c_str());

    if (segmentKey.groupIndex == ALL_GROUPS)
    {
        xs_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
        xs_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
    }

    // Setup column headers
    ColumnIndexType col = 0;
    if (intervalIdx < erectionIntervalIdx)
    {
        (*xs_table)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
    }
    else
    {
        (*xs_table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
    }
    (*xs_table)(0, col++) << COLHDR(_T("Area"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
    (*xs_table)(0, col++) << COLHDR(_T("Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
    (*xs_table)(0, col++) << COLHDR(RPT_IX, rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());

    if (intervalIdx < erectionIntervalIdx)
    {
        (*xs_table)(0, col++) << COLHDR(RPT_IY, rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());

        if (bAsymmetricGirders)
        {
            (*xs_table)(0, col++) << COLHDR(RPT_IXY, rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
        }
    }


    (*xs_table)(0, col++) << COLHDR(RPT_XLEFT_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
    (*xs_table)(0, col++) << COLHDR(RPT_XRIGHT_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

    if (intervalIdx < firstCompositeDeckIntervalIdx || (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite))
    {
        (*xs_table)(0, col++) << COLHDR(RPT_YTOP_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        (*xs_table)(0, col++) << COLHDR(RPT_YBOT_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        (*xs_table)(0, col++) << COLHDR(RPT_STOP_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
        (*xs_table)(0, col++) << COLHDR(RPT_SBOT_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
    }
    else
    {
        (*xs_table)(0, col++) << COLHDR(RPT_YTOP_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        (*xs_table)(0, col++) << COLHDR(RPT_YBOT_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

        if (bIsCompositeDeck)
        {
            (*xs_table)(0, col++) << COLHDR(RPT_YTOP_DECK, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*xs_table)(0, col++) << COLHDR(RPT_YBOT_DECK, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }

        (*xs_table)(0, col++) << COLHDR(RPT_STOP_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
        (*xs_table)(0, col++) << COLHDR(RPT_SBOT_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());

        if (bIsCompositeDeck)
        {
            (*xs_table)(0, col++) << COLHDR(RPT_STOP_DECK, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
            (*xs_table)(0, col++) << COLHDR(RPT_SBOT_DECK, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
        }
    }

    if (intervalIdx < firstCompositeDeckIntervalIdx ||
        (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite) ||
        (lastTendonStressingIntervalIdx != INVALID_INDEX && firstCompositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
        )
    {
        (*xs_table)(0, col++) << Sub2(_T("k"), _T("t")) << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")") << rptNewLine << _T("(Top") << rptNewLine << _T("kern") << rptNewLine << _T("point)");
        (*xs_table)(0, col++) << Sub2(_T("k"), _T("b")) << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")") << rptNewLine << _T("(Bottom") << rptNewLine << _T("kern") << rptNewLine << _T("point)");
    }

    if (firstCompositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed))
    {
        (*xs_table)(0, col++) << COLHDR(Sub2(_T("Q"), _T("deck")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
        (*xs_table)(0, col++) << COLHDR(_T("Effective") << rptNewLine << _T("Flange") << rptNewLine << _T("Width"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

        if (pgsTypes::hspZeroHaunch != haunchAType)
        {
            (*xs_table)(0, col++) << COLHDR(_T("Haunch") << rptNewLine << _T("Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
    }
    else if (intervalIdx <= firstCompositeDeckIntervalIdx)
    {
        (*xs_table)(0, col++) << COLHDR(_T("Perimeter"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
    }

    INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, l1, pDisplayUnits->GetComponentDimUnit(), false);
    INIT_UV_PROTOTYPE(rptAreaUnitValue, l2, pDisplayUnits->GetAreaUnit(), false);
    INIT_UV_PROTOTYPE(rptLength3UnitValue, l3, pDisplayUnits->GetSectModulusUnit(), false);
    INIT_UV_PROTOTYPE(rptLength4UnitValue, l4, pDisplayUnits->GetMomentOfInertiaUnit(), false);

    location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS || segmentKey.segmentIndex == ALL_SEGMENTS);

    // Get all the tabular poi's for flexure and shear
    // Merge the two vectors to form one vector to report on.
    PoiAttributeType poiRefAttribute = (intervalIdx < erectionIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT);

    RowIndexType row = xs_table->GetNumberOfHeaderRows();

    col = 0;

    Float64 depth = pSectProp->GetHg(spType, intervalIdx, poi);

    (*xs_table)(row, col++) << location.SetValue(poiRefAttribute, poi);
    (*xs_table)(row, col++) << l2.SetValue(pSectProp->GetAg(spType, intervalIdx, poi));
    (*xs_table)(row, col++) << l1.SetValue(depth);
    (*xs_table)(row, col++) << l4.SetValue(pSectProp->GetIxx(spType, intervalIdx, poi));
    if (intervalIdx < erectionIntervalIdx)
    {
        (*xs_table)(row, col++) << l4.SetValue(pSectProp->GetIyy(spType, intervalIdx, poi));
        if (bAsymmetricGirders)
        {
            (*xs_table)(row, col++) << l4.SetValue(pSectProp->GetIxy(spType, intervalIdx, poi));
        }
    }

    (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetXleft(spType, intervalIdx, poi));
    (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetXright(spType, intervalIdx, poi));

    if (intervalIdx < firstCompositeDeckIntervalIdx || (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite))
    {
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetY(spType, intervalIdx, poi, pgsTypes::TopGirder));
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetY(spType, intervalIdx, poi, pgsTypes::BottomGirder));
        (*xs_table)(row, col++) << l3.SetValue(pSectProp->GetS(spType, intervalIdx, poi, pgsTypes::TopGirder));
        (*xs_table)(row, col++) << l3.SetValue(pSectProp->GetS(spType, intervalIdx, poi, pgsTypes::BottomGirder));
    }
    else
    {
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetY(spType, intervalIdx, poi, pgsTypes::TopGirder));
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetY(spType, intervalIdx, poi, pgsTypes::BottomGirder));
        if (bIsCompositeDeck)
        {
            (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetY(spType, intervalIdx, poi, pgsTypes::TopDeck));
            (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetY(spType, intervalIdx, poi, pgsTypes::BottomDeck));
        }

        (*xs_table)(row, col++) << l3.SetValue(pSectProp->GetS(spType, intervalIdx, poi, pgsTypes::TopGirder));
        (*xs_table)(row, col++) << l3.SetValue(pSectProp->GetS(spType, intervalIdx, poi, pgsTypes::BottomGirder));
        if (bIsCompositeDeck)
        {
            (*xs_table)(row, col++) << l3.SetValue(pSectProp->GetS(spType, intervalIdx, poi, pgsTypes::TopDeck));
            (*xs_table)(row, col++) << l3.SetValue(pSectProp->GetS(spType, intervalIdx, poi, pgsTypes::BottomDeck));
        }
    }

    if ((intervalIdx < firstCompositeDeckIntervalIdx) ||
        (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite) ||
        (lastTendonStressingIntervalIdx != INVALID_INDEX && lastCompositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
        )
    {
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetKt(spType, intervalIdx, poi));
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetKb(spType, intervalIdx, poi));
    }

    if (firstCompositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed))
    {
        (*xs_table)(row, col++) << l3.SetValue(pSectProp->GetQSlab(intervalIdx, poi));
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetEffectiveFlangeWidth(poi));
    }
    else if (intervalIdx <= firstCompositeDeckIntervalIdx)
    {
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetPerimeter(poi));
    }

    if (lastCompositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck && pgsTypes::hspZeroHaunch != haunchAType && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed))
    {
        (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetStructuralHaunchDepth(poi, haunchAType));
    }
    

    return xs_table;
}

rptChapter* CSectionPropertiesChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pSectPropRptSpec = std::dynamic_pointer_cast<const CSectionPropertiesReportSpecification>(pRptSpec);

   std::shared_ptr<WBFL::EAF::Broker> pBroker = pSectPropRptSpec->GetBroker();

   const auto& poi = pSectPropRptSpec->GetPOI();
   const auto& segmentKey = poi.GetSegmentKey();
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   const auto& pierID = pPoi->GetPier(poi);
   IntervalIndexType intervalIdx = pSectPropRptSpec->GetInterval();

   GET_IFACE2(pBroker, IShapes, pShapes);

   CComPtr<IShape> shape;
   pShapes->GetSegmentShape(intervalIdx, poi, false, pgsTypes::scGirder, &shape);

   std::vector<CComPtr<IPoint2dCollection>> vGrossGirderShapePoints;
   CComPtr<IPoint2dCollection> GrossGirderShapePoints;
   vGrossGirderShapePoints.emplace_back(GrossGirderShapePoints);

   CComPtr<IPoint2dCollection> deckShapePoints;
   std::vector<Float64> vSteelElasticProperties;
   std::vector<CComPtr<IShapeProperties>> vSteelShapeProperties;
   CComPtr<IPoint2dCollection> vStraightStrandPositions;
   CComPtr<IPoint2dCollection> vHarpedStrandPositions;
   CComPtr<IPoint2dCollection> vTempStrandPositions;

   CComQIPtr<ICompositeShape> compShape(shape);

   CComPtr<IShape> pGrossGirderShape;
   CComPtr<IShape> pDeckShape;

   CComPtr<IShapeProperties> vShapeProps;
   std::vector<CComPtr<IShapeProperties>> voidShapeProperties;

   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   SectProp steelElasticProp = pSectProp->GetSectionProperties(intervalIdx, poi, pgsTypes::SectionPropertyType::sptTransformed);
   CComQIPtr<ICompositeSectionEx> steelCompositeAdapter(steelElasticProp.Section);
   

   if (compShape)
   {
       IndexType nShapes;
       compShape->get_Count(&nShapes);

       CComPtr<ICompositeShapeItem> item;
       compShape->get_Item(0, &item);
       item->get_Shape(&pGrossGirderShape);

       shape->get_PolyPoints(&vGrossGirderShapePoints[0]);

       CComQIPtr<ICompositeShape> voidedShape(pGrossGirderShape);

       if (voidedShape)
       {
           IndexType nVoidShapes;
           voidedShape->get_Count(&nVoidShapes);

           for (IndexType i = 1; i < nVoidShapes; i++)
           {
               CComPtr<IPoint2dCollection> pi;
               vGrossGirderShapePoints.emplace_back(pi);

               CComPtr<ICompositeShapeItem> voidItem;
               voidedShape->get_Item(i, &voidItem);

               CComPtr<IShape> s;
               voidItem->get_Shape(&s);

               s->get_PolyPoints(&vGrossGirderShapePoints[i]);

               s->get_ShapeProperties(&vShapeProps);

               voidShapeProperties.emplace_back(vShapeProps);

           }
       }

       if (nShapes > 1)
       {
		   for (IndexType i = 1; i < nShapes; i++)
           {
               item.Release();
               compShape->get_Item(i, &item);
               CComPtr<IShape> sShape;
               item->get_Shape(&sShape);
               CComPtr<IPoint2dCollection> secondaryShapePoints;
               sShape->get_PolyPoints(&secondaryShapePoints);
               IndexType nPtCount = 0;
               secondaryShapePoints->get_Count(&nPtCount);
               if (nPtCount == 4) // must be a deck shape
               {
				   deckShapePoints = secondaryShapePoints;
				   pDeckShape = sShape;
               }
               else
               {
                   CComPtr<ICompositeSectionItemEx> comp_item;
                   steelCompositeAdapter->get_Item(i, &comp_item);

                   Float64 E;
                   comp_item->get_Efg(&E);

                   vSteelElasticProperties.emplace_back(E);

                   CComPtr<IShapeProperties> steelShapeProps;
                   sShape->get_ShapeProperties(&steelShapeProps);
				   vSteelShapeProperties.emplace_back(steelShapeProps);
               }
           }
       }
   }
   else
   {
       shape->get_PolyPoints(&vGrossGirderShapePoints[0]);

       CComQIPtr<ICompositeShape> voidedShape(pGrossGirderShape);

       if (voidedShape)
       {
           IndexType nVoidShapes;
           voidedShape->get_Count(&nVoidShapes);

           for (IndexType i = 0; i < nVoidShapes; i++)
           {
               CComPtr<ICompositeShapeItem> voidItem;
               voidedShape->get_Item(i, &voidItem);

               CComPtr<IShape> s;
               voidItem->get_Shape(&s);

               s->get_PolyPoints(&vGrossGirderShapePoints[i + 1]);

           }
       }
   }

   CComPtr<IShape> leftJointShape, rightJointShape;
   pShapes->GetJointShapes(intervalIdx, poi, false, pgsTypes::scGirder, &leftJointShape, &rightJointShape);
   if (leftJointShape)
   {
       CComPtr<IPoint2dCollection> points;
       leftJointShape->get_PolyPoints(&points);

       CComPtr<IEnumPoint2d> enumPoints;
       points->get__Enum(&enumPoints);
       CComPtr<IPoint2d> point;
       while (enumPoints->Next(1, &point, nullptr) != S_FALSE)
       {
           deckShapePoints->Add(point);
           point.Release();
       }
   }

   if (rightJointShape)
   {
       CComPtr<IPoint2dCollection> points;
       rightJointShape->get_PolyPoints(&points);

       CComPtr<IEnumPoint2d> enumPoints;
       points->get__Enum(&enumPoints);
       CComPtr<IPoint2d> point;
       while (enumPoints->Next(1, &point, nullptr) != S_FALSE)
       {
           deckShapePoints->Add(point);
           point.Release();
       }
   }


   GET_IFACE2(pBroker, IMaterials, pMaterials);
   Float64 EcGdr;
   if (poi.HasAttribute(POI_CLOSURE))
   {
       EcGdr = pMaterials->GetClosureJointEc(poi.GetSegmentKey(), intervalIdx);
   }
   else
   {
       EcGdr = pMaterials->GetSegmentEc(poi.GetSegmentKey(), intervalIdx);
   }

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   Float64 EcDeck;
   if (IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType()))
   {
       if (pBridgeDesc->HasStructuralLongitudinalJoints())
       {
           EcDeck = pMaterials->GetLongitudinalJointEc(intervalIdx);
       }
       else
       {
           EcDeck = 0.0;
       }
   }
   else
   {
       IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
       ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
       EcDeck = pMaterials->GetDeckEc(deckCastingRegionIdx, intervalIdx);
   }

   std::vector <std::vector<std::pair<Float64, Float64>>> vGrossGirderShapeXYPoints;
   vGrossGirderShapeXYPoints.resize(vGrossGirderShapePoints.size());
   std::vector<std::pair<Float64, Float64>> secondaryPoints;

   IndexType nPoints;

   for (IndexType j = 0; j < vGrossGirderShapePoints.size(); j++)
   {
       if (vGrossGirderShapePoints[j])
       {
           vGrossGirderShapePoints[j]->get_Count(&nPoints);
           for (IndexType i = 0; i < nPoints; i++)
           {
               CComPtr<IPoint2d> pnt;
               vGrossGirderShapePoints[j]->get_Item(i, &pnt);
               Float64 x, y;
               pnt->Location(&x, &y);

               x = IsZero(x) ? 0 : x;
               y = IsZero(y) ? 0 : y;

               vGrossGirderShapeXYPoints[j].emplace_back(x, y);

           }
       }
   }

   if (deckShapePoints)
   {
       deckShapePoints->get_Count(&nPoints);
       for (IndexType i = 0; i < nPoints; i++)
       {
           CComPtr<IPoint2d> pnt;
           deckShapePoints->get_Item(i, &pnt);
           Float64 x, y;
           pnt->Location(&x, &y);

           x = IsZero(x) ? 0 : x;
           y = IsZero(y) ? 0 : y;

           secondaryPoints.emplace_back(x, y);

       }
   }

   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();
   INIT_UV_PROTOTYPE(rptStressUnitValue, modE, pDispUnits->ModE, true);
   INIT_UV_PROTOTYPE(rptLength2UnitValue, area, pDispUnits->Area, true);

   // Organize Tables
   rptRcTable* pParentLayoutTable = rptStyleManager::CreateLayoutTable(7);
   (*pParentLayoutTable)(0, 0) << Bold(_T("Girder"));
   rptRcTable* pNonCompositeLayoutTable = rptStyleManager::CreateLayoutTable(2);
   rptRcTable* pPrimaryPointsTable = rptStyleManager::CreateDefaultTable(2);
   (*pNonCompositeLayoutTable)(0, 0) << pPrimaryPointsTable;
   CComPtr<IShapeProperties> pShapeProps;
   pGrossGirderShape->get_ShapeProperties(&pShapeProps);
   WriteSectionProperties((*pNonCompositeLayoutTable)(0, 0), pShapeProps);
   modE.SetValue(EcGdr);
   (*pNonCompositeLayoutTable)(0, 0) << Sub2(_T("E"), _T("c")) << _T(" = ") << modE << rptNewLine;

   (*pParentLayoutTable)(0, 0) << pNonCompositeLayoutTable;

   rptRcTable* pSecondaryPointsTable = rptStyleManager::CreateDefaultTable(2);
   rptRcTable* pSteelComponentPropertiesTable = rptStyleManager::CreateDefaultTable(4);
   rptRcTable* pStraightStrandPropertiesTable = rptStyleManager::CreateDefaultTable(3);
   rptRcTable* pHarpedStrandPropertiesTable = rptStyleManager::CreateDefaultTable(3);
   rptRcTable* pTemporaryStrandPropertiesTable = rptStyleManager::CreateDefaultTable(3);
   rptRcTable* pRebarPropertiesTable = rptStyleManager::CreateDefaultTable(3);


   if (pDeckShape)
   {
       (*pParentLayoutTable)(0, 1) << Bold(_T("Deck"));
       rptRcTable* pCompositeLayoutTable = rptStyleManager::CreateLayoutTable(2);
       (*pCompositeLayoutTable)(0, 0) << pSecondaryPointsTable;
	   modE.SetValue(EcDeck);
       CComPtr<IShapeProperties> cShapeProps;
       pDeckShape->get_ShapeProperties(&cShapeProps);
       WriteSectionProperties((*pCompositeLayoutTable)(0, 0), cShapeProps);
       (*pCompositeLayoutTable)(0, 0) << Sub2(_T("E"),_T("c")) << _T(" = ") << modE << rptNewLine;
       (*pParentLayoutTable)(0, 1) << pCompositeLayoutTable;
   }

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeom);
   pStrandGeom->GetStrandPositions(poi, pgsTypes::StrandType::Straight, &vStraightStrandPositions);
   pStrandGeom->GetStrandPositions(poi, pgsTypes::StrandType::Harped, &vHarpedStrandPositions);
   pStrandGeom->GetStrandPositions(poi, pgsTypes::StrandType::Temporary, &vTempStrandPositions);
   StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::StrandType::Straight);
   StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::StrandType::Harped);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::StrandType::Temporary);

   GET_IFACE2(pBroker, ILongRebarGeometry, pRebarGeom);
   pgsPointOfInterest barCutPoi(poi);
   CComPtr<IRebarSection> rebar_section;
   pRebarGeom->GetRebars(barCutPoi, &rebar_section);

   IndexType nBars;
   rebar_section->get_Count(&nBars);

	if (!vSteelShapeProperties.empty())
    {
        (*pParentLayoutTable)(0, 2) << Bold(_T("Strands, Long. Rebar & Ducts"));
        (*pParentLayoutTable)(0, 2) << pSteelComponentPropertiesTable;
        
    }
    else
    {

        if (Ns > 0)
        {
            (*pParentLayoutTable)(0, 2) << Bold(_T("Straight Strands"));
            (*pParentLayoutTable)(0, 2) << pStraightStrandPropertiesTable;
            const auto& Eps = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::StrandType::Straight)->GetE();
            modE.SetValue(Eps);
            (*pParentLayoutTable)(0, 2) << Sub2(_T("E"), _T("ps")) << _T(" = ") << modE << rptNewLine;
            const auto& apss = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::StrandType::Straight)->GetNominalArea();
            area.SetValue(apss);
            (*pParentLayoutTable)(0, 2) << Sub2(_T("A"), _T("ps")) << _T(" = ") << area << rptNewLine;
        }

        if (Nh > 0)
        {
            (*pParentLayoutTable)(0, 3) << Bold(_T("Harped Strands"));
            (*pParentLayoutTable)(0, 3) << pHarpedStrandPropertiesTable;
            const auto& Eph = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::StrandType::Harped)->GetE();
            modE.SetValue(Eph);
            (*pParentLayoutTable)(0, 3) << Sub2(_T("E"), _T("ps")) << _T(" = ") << modE << rptNewLine;
            const auto& apsh = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::StrandType::Harped)->GetNominalArea();
            area.SetValue(apsh);
            (*pParentLayoutTable)(0, 3) << Sub2(_T("A"), _T("ps")) << _T(" = ") << area << rptNewLine;
        }

		if (Nt > 0)
        {
            (*pParentLayoutTable)(0, 4) << Bold(_T("Temporary Strands"));
            (*pParentLayoutTable)(0, 4) << pTemporaryStrandPropertiesTable;
            const auto& Ept = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::StrandType::Temporary)->GetE();
            modE.SetValue(Ept);
            (*pParentLayoutTable)(0, 4) << Sub2(_T("E"), _T("ps")) << _T(" = ") << modE << rptNewLine;
            const auto& apst = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::StrandType::Temporary)->GetNominalArea();
            area.SetValue(apst);
            (*pParentLayoutTable)(0, 4) << Sub2(_T("A"), _T("ps")) << _T(" = ") << area << rptNewLine;
        }

		if (nBars > 0)
        {
            (*pParentLayoutTable)(0, 5) << Bold(_T("Longitudinal Rebar"));
            (*pParentLayoutTable)(0, 5) << pRebarPropertiesTable;

            CClosureKey closureKey;
            bool bIsInClosure = pPoi->IsInClosureJoint(poi, &closureKey);

            Float64 EDeckRebar, EGirderRebar, Fy, Fu;
            pMaterials->GetDeckRebarProperties(&EDeckRebar, &Fy, &Fu);

            if (bIsInClosure)
            {
                pMaterials->GetClosureJointLongitudinalRebarProperties(closureKey, &EGirderRebar, &Fy, &Fu);
            }
            else
            {
                pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey, &EGirderRebar, &Fy, &Fu);
            }
            modE.SetValue(EGirderRebar);
            (*pParentLayoutTable)(0, 5) << _T("Girder ") << Sub2(_T("E"), _T("s")) << _T(" = ") << modE << rptNewLine;
            modE.SetValue(EDeckRebar);
            (*pParentLayoutTable)(0, 5) << _T("Deck ") << Sub2(_T("E"), _T("s")) << _T(" = ") << modE << rptNewLine;
        }
    }

    

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDispUnits->ComponentDim, false);
   area.ShowUnitTag(false);


   (*pPrimaryPointsTable)(0, 0) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
   (*pPrimaryPointsTable)(0, 1) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);

   RowIndexType row = pPrimaryPointsTable->GetNumberOfHeaderRows();

   std::vector<std::pair<Float64, Float64>>::const_iterator iter;
   std::vector<std::pair<Float64, Float64>>::const_iterator end;

   auto pVoidPropertiesTable = rptStyleManager::CreateDefaultTable(7);

   (*pVoidPropertiesTable)(0, 0) << Bold(_T("Void ID"));
   (*pVoidPropertiesTable)(0, 1) << COLHDR(Sub2(_T("A"), _T("g")), rptLength2UnitTag, pDispUnits->Area);
   (*pVoidPropertiesTable)(0, 2) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
   (*pVoidPropertiesTable)(0, 3) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);
   (*pVoidPropertiesTable)(0, 4) << COLHDR(Sub2(_T("I"), _T("x")), rptLength4UnitTag, pDispUnits->MomentOfInertia);
   (*pVoidPropertiesTable)(0, 5) << COLHDR(Sub2(_T("I"), _T("y")), rptLength4UnitTag, pDispUnits->MomentOfInertia);
   (*pVoidPropertiesTable)(0, 6) << COLHDR(Sub2(_T("I"), _T("xy")), rptLength4UnitTag, pDispUnits->MomentOfInertia);

   for (IndexType i = 0; i < vGrossGirderShapeXYPoints.size(); i++)
   {

       iter = vGrossGirderShapeXYPoints[i].begin();
       end = vGrossGirderShapeXYPoints[i].end();

       if (i > 0)
       {
           CString voidStr;
           ColumnIndexType col = (ColumnIndexType)(i - 1);

           if (i <= 2)
           {
               voidStr.Format(_T("Ext. Void %d"), i);
           }
           else
           { 
               voidStr.Format(_T("Int. Void %d"), i - 2);
           }

           CComPtr<IPoint2d> pntCG;
           voidShapeProperties[col]->get_Centroid(&pntCG);

           Float64 xcg, ycg;
           pntCG->Location(&xcg, &ycg);

           Float64 Area;
           voidShapeProperties[col]->get_Area(&Area);
           Float64 Ixx;
           voidShapeProperties[col]->get_Ixx(&Ixx);
           Float64 Iyy;
           voidShapeProperties[col]->get_Iyy(&Iyy);
           Float64 Ixy;
           voidShapeProperties[col]->get_Ixy(&Ixy);

           INIT_UV_PROTOTYPE(rptLength2UnitValue, area, pDispUnits->Area, false);
           INIT_UV_PROTOTYPE(rptLength4UnitValue, momentOfInertia, pDispUnits->MomentOfInertia, false);
           
           row = pVoidPropertiesTable->GetNumberOfHeaderRows();
           IndexType c = 0;
           (*pVoidPropertiesTable)(row + col, c++) << Bold(voidStr);
           (*pVoidPropertiesTable)(row + col, c++) << length.SetValue(xcg);
           (*pVoidPropertiesTable)(row + col, c++) << length.SetValue(ycg);
           (*pVoidPropertiesTable)(row + col, c++) << area.SetValue(Area);
           (*pVoidPropertiesTable)(row + col, c++) << momentOfInertia.SetValue(Ixx);
           (*pVoidPropertiesTable)(row + col, c++) << momentOfInertia.SetValue(Iyy);
           (*pVoidPropertiesTable)(row + col, c++) << momentOfInertia.SetValue(Ixy);


       }
       else
       {
           for (; iter != end; iter++, row++)
           {
               Float64 x = iter->first;
               Float64 y = iter->second;


               (*pPrimaryPointsTable)(row, 0) << length.SetValue(x);
               (*pPrimaryPointsTable)(row, 1) << length.SetValue(y);

           }
       }
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   rptHeading* pHeading = rptStyleManager::CreateHeading(2);
   (*pChapter) << pHeading;
   pHeading->SetName(_T("Computed Bridge Section Properties"));
   *pHeading << _T("Computed Bridge Section Properties");

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker, IGirder, pGirder);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2_NOCHECK(pBroker, IIntervals, pIntervals);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   bool bIsPrismatic_CastingYard = pGirder->IsPrismatic(releaseIntervalIdx, segmentKey);
   bool bIsPrismatic_Final = pGirder->IsPrismatic(lastIntervalIdx, segmentKey);

   bool bComposite = pBridge->IsCompositeDeck();
   if (pGirder->HasStructuralLongitudinalJoints())
   {
       bComposite = true;
   }

   if (bIsPrismatic_CastingYard && !bIsPrismatic_Final)
   {

       if (bComposite)
       {
           // there is a deck so we have composite, non-prismatic results
           GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
           rptRcTable* pTable = WriteXSTable2(pBroker, pgsTypes::sptGross, poi, intervalIdx, pDisplayUnits);
           *pPara << pTable << rptNewLine;
       }
   }
   else if (!bIsPrismatic_CastingYard && !bIsPrismatic_Final)
   {
       pgsTypes::SectionPropertyType spType = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);

       GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
       rptRcTable* pTable = WriteXSTable2(pBroker, spType, poi, intervalIdx, pDisplayUnits);
       *pPara << pTable << rptNewLine;

       if (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmTransformed)
       {
           GET_IFACE2(pBroker, ILossParameters, pLossParams);
           if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
           {
               rptRcTable* pTable = WriteXSTable2(pBroker, pgsTypes::sptGross, poi, intervalIdx, pDisplayUnits);
               *pPara << pTable << rptNewLine;
           }
       }

   }
   else if (!bIsPrismatic_CastingYard && bIsPrismatic_Final)
   {
       ATLASSERT(false); // this is an impossible case
   }

   //*pPara << rptNewPage;

   pHeading = rptStyleManager::CreateHeading(2);
   (*pChapter) << pHeading;
   pHeading->SetName(_T("Geometry Formulas"));
   *pHeading << _T("Geometry Formulas");

   pPara = new rptParagraph();
   *pChapter << pPara;

   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Area.png"),
       _T("A = \\sum{ (x_{i + 1} - x_i)y_i + \\frac{1}{2}(x_{i + 1} - x_i)(y_{i + 1} - y_i) }")) << rptNewLine << rptNewLine;

   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ix.png"),
       _T("I_x = \\sum{\\frac{{y_i} ^ 3(x_{i + 1} - x_i)}{12} + (x_{i + 1} - x_i)y_i { \\frac{y_i}{2} } ^ 2 + \\frac{(y_{i + 1} - y_i) ^ 3 (x_{i + 1} - x_i)}{36}+ \\frac{1}{2} (x_{i + 1} - x_i)(y_{i + 1} - y_i) \\left[\\frac{(y_{i + 1} - y_i)}{3} + y_i \\right] ^ 3}")) << rptNewLine << rptNewLine;
   
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Iy.png"),
       _T("I_y = \\sum{ \\frac{ y_i(x_{i + 1} - x_i) ^ 3 }{12} + (x_{ i + 1 } - x_i)y_i \\left[x_i + \\frac{ (x_{i + 1} - x_i) }{2} \\right] ^ 2 + \\frac{ (y_{i + 1} - y_i)(x_{i + 1} - x_i) ^ 3 }{36} + \\frac{ 1 }{2} (x_{ i + 1 } - x_i)(y_{ i + 1 } - y_i) \\left[\\frac{ 2(x_{i + 1} - x_i) }{3} + x_i \\right] ^ 3}")) << rptNewLine << rptNewLine;
       
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ixy.png"),
       _T("I_{ xy } = \\sum{(x_{ i + 1 } - x_i)\\frac{ {y_i} ^ 2 }{2} \\left(x_i + \\frac{ x_{i + 1} - x_i }{2} \\right) + \\frac{ 1 }{2} (x_{ i + 1 } - x_i)(y_{ i + 1 } - y_i) \\left(\\frac{ y_{i + 1} - y_i }{3} - y_i  \\right) \\left[\\frac{ 2(x_{i + 1} - x_i) }{3} + x_i \\right] + \\frac{ (x_{i + 1} - x_i) ^ 2(y_{i + 1} - y_i) ^ 2 }{72}}")) << rptNewLine << rptNewLine;

   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Xbar.png"),
       _T("\\overline{ x } = \\frac{ \\sum{ \\frac{1}{2}y_i(x_{i + 1} - x_i)(x_{i + 1} + x_i) + \\frac{1}{2}(x_{i + 1} - x_i)(y_{i + 1} - y_i) \\left[\\frac{2}{3}(x_{i + 1} + x_i) + x_i \\right] } }{A}")) << rptNewLine << rptNewLine;
  
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ybar.png"),
       _T("\\overline{y} = \\frac{ \\sum{ \\frac{1}{2}{y_i}^2(x_{i+1} - x_i) + \\frac{1}{2}(x_{i+1} - x_i)(y_{i+1} - y_i) \\left[ \\frac{1}{3}(y_{i+1} + y_i) + y_i \\right] } }{A}")) << rptNewLine << rptNewLine;

   //Properties

   if (pDeckShape)
   {

       (*pSecondaryPointsTable)(0, 0) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
       (*pSecondaryPointsTable)(0, 1) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);

       row = pSecondaryPointsTable->GetNumberOfHeaderRows();
       iter = secondaryPoints.begin();
       end = secondaryPoints.end();
       for (; iter != end; iter++, row++)
       {
           Float64 x = iter->first;
           Float64 y = iter->second;

           (*pSecondaryPointsTable)(row, 0) << length.SetValue(x);
           (*pSecondaryPointsTable)(row, 1) << length.SetValue(y);
       }

   }

   if (!vSteelShapeProperties.empty())
   {
       INIT_UV_PROTOTYPE(rptLength2UnitValue, area, pDispUnits->Area, false);
       INIT_UV_PROTOTYPE(rptLength4UnitValue, momentOfInertia, pDispUnits->MomentOfInertia, true);

       (*pSteelComponentPropertiesTable)(0, 0) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
       (*pSteelComponentPropertiesTable)(0, 1) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);
       (*pSteelComponentPropertiesTable)(0, 2) << COLHDR(Sub2(_T("A"), _T("s")), rptLength2UnitTag, pDispUnits->Area);
       (*pSteelComponentPropertiesTable)(0, 3) << COLHDR(_T("E"), rptStressUnitTag, pDispUnits->ModE);

       row = pSteelComponentPropertiesTable->GetNumberOfHeaderRows();
       for (IndexType idx = 0; idx < vSteelShapeProperties.size() ; idx++)
       {
            CComPtr<IPoint2d> pntCG;
            vSteelShapeProperties[idx]->get_Centroid(&pntCG);

            Float64 xcg, ycg;
            pntCG->Location(&xcg, &ycg);

            Float64 Area;
            vSteelShapeProperties[idx][0].get_Area(&Area);

            (*pSteelComponentPropertiesTable)(row + idx, 0) << length.SetValue(xcg);
            (*pSteelComponentPropertiesTable)(row + idx, 1) << length.SetValue(ycg);
            (*pSteelComponentPropertiesTable)(row + idx, 2) << area.SetValue(Area);

            Float64 E = vSteelElasticProperties[idx];

			modE.ShowUnitTag(false);
            
            (*pSteelComponentPropertiesTable)(row + idx, 3) << modE.SetValue(E);

	   }
   }
   else
   {
       
       Float64 x, y;
       
	   if (Ns > 0)
       {
           pStraightStrandPropertiesTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_CENTER));
           pStraightStrandPropertiesTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CJ_CENTER));
           (*pStraightStrandPropertiesTable)(0, 0) << _T("Strand") << rptNewLine << _T("No.");
           (*pStraightStrandPropertiesTable)(0, 1) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
           (*pStraightStrandPropertiesTable)(0, 2) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);

           row = pStraightStrandPropertiesTable->GetNumberOfHeaderRows();

           for (StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++)
           {
               CComPtr<IPoint2d> strandPoint;
               vStraightStrandPositions->get_Item(strandIdx, &strandPoint);
               strandPoint->Location(&x, &y);
               (*pStraightStrandPropertiesTable)(row + strandIdx, 0) << strandIdx + 1;
               (*pStraightStrandPropertiesTable)(row + strandIdx, 1) << length.SetValue(x);
               (*pStraightStrandPropertiesTable)(row + strandIdx, 2) << length.SetValue(y);
           }
       }

	   if (Nh > 0)
       {
           pHarpedStrandPropertiesTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_CENTER));
           pHarpedStrandPropertiesTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CJ_CENTER));
           (*pHarpedStrandPropertiesTable)(0, 0) << _T("Strand") << rptNewLine << _T("No.");
           (*pHarpedStrandPropertiesTable)(0, 1) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
           (*pHarpedStrandPropertiesTable)(0, 2) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);

           row = pHarpedStrandPropertiesTable->GetNumberOfHeaderRows();

           for (StrandIndexType strandIdx = 0; strandIdx < Nh; strandIdx++)
           {
               CComPtr<IPoint2d> strandPoint;
               vHarpedStrandPositions->get_Item(strandIdx, &strandPoint);
               strandPoint->Location(&x, &y);
               (*pHarpedStrandPropertiesTable)(row + strandIdx, 0) << strandIdx + 1;
               (*pHarpedStrandPropertiesTable)(row + strandIdx, 1) << length.SetValue(x);
               (*pHarpedStrandPropertiesTable)(row + strandIdx, 2) << length.SetValue(y);
           }
       }

	   if (Nt > 0)
       {
           pTemporaryStrandPropertiesTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_CENTER));
           pTemporaryStrandPropertiesTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CJ_CENTER));
           (*pTemporaryStrandPropertiesTable)(0, 0) << _T("Strand") << rptNewLine << _T("No.");
           (*pTemporaryStrandPropertiesTable)(0, 1) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
           (*pTemporaryStrandPropertiesTable)(0, 2) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);

           row = pTemporaryStrandPropertiesTable->GetNumberOfHeaderRows();

           for (StrandIndexType strandIdx = 0; strandIdx < Nt; strandIdx++)
           {
               CComPtr<IPoint2d> strandPoint;
               vTempStrandPositions->get_Item(strandIdx, &strandPoint);
               strandPoint->Location(&x, &y);
               (*pTemporaryStrandPropertiesTable)(row + strandIdx, 0) << strandIdx + 1;
               (*pTemporaryStrandPropertiesTable)(row + strandIdx, 1) << length.SetValue(x);
               (*pTemporaryStrandPropertiesTable)(row + strandIdx, 2) << length.SetValue(y);
           }
       }

	   if (nBars > 0)
       {
           pRebarPropertiesTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_CENTER));
           pRebarPropertiesTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CJ_CENTER));
           (*pRebarPropertiesTable)(0, 0) << COLHDR(Sub2(_T("X"), _T("c.g.")), rptLengthUnitTag, pDispUnits->ComponentDim);
           (*pRebarPropertiesTable)(0, 1) << COLHDR(Sub2(_T("Y"), _T("c.g.")), rptLengthUnitTag, pDispUnits->ComponentDim);
           (*pRebarPropertiesTable)(0, 2) << COLHDR(Sub2(_T("A"), _T("s")), rptAreaUnitTag, pDispUnits->Area);

           CComPtr<IEnumRebarSectionItem> enumItems;
           rebar_section->get__EnumRebarSectionItem(&enumItems);

           CComPtr<IRebarSectionItem> item;
           IndexType idx = 0;
           while (enumItems->Next(1, &item, nullptr) != S_FALSE)
           {
               CComPtr<IPoint2d> location;
               item->get_Location(&location);

               Float64 x, y;
               location->get_X(&x);
               location->get_Y(&y);

               CComPtr<IRebar> rebar;
               item->get_Rebar(&rebar);
               Float64 as;
               rebar->get_NominalArea(&as);

               (*pRebarPropertiesTable)(row + idx, 0) << length.SetValue(x);
               (*pRebarPropertiesTable)(row + idx, 1) << length.SetValue(y);
               area.ShowUnitTag(false);
               (*pRebarPropertiesTable)(row + idx, 2) << area.SetValue(as);

               idx++;

               item.Release();
           }
       }
       
   }

   pHeading = rptStyleManager::CreateHeading(2);
   (*pChapter) << pHeading;
   pHeading->SetName(_T("Indivisual Section Coordinates & Properties"));
   *pHeading << _T("Individual Section Coordinates & Properties");

   pPara = new rptParagraph();
   *pChapter << pPara;

   *pPara << pParentLayoutTable;
   
   IndexType numVoids = vGrossGirderShapeXYPoints.size() - 1;
   if (numVoids > 0)
   {
       pHeading = rptStyleManager::CreateHeading(2);
       (*pChapter) << pHeading;
       pHeading->SetName(_T("Voids"));
       *pHeading << _T("Voids");

       pPara = new rptParagraph();
       *pChapter << pPara;

       *pPara << pVoidPropertiesTable;
   }

   *pPara << rptNewPage;


   CComPtr<IPoint2d> sectCG;
   pSectProp->GetCentroid(intervalIdx, poi, &sectCG);
   Float64 xcg, ycg;
   sectCG->Location(&xcg, &ycg);
   const std::pair<Float64, Float64> cg_pair = { xcg, ycg };

   *pPara << CreateImage(vGrossGirderShapeXYPoints, secondaryPoints, cg_pair) << rptNewLine;

   // Export coordinates to CSV (FOR DEBUGGING PURPOSE ONLY)
   //TCHAR szPath[MAX_PATH];
   //if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, szPath)))
   //{
   //    std::_tstring outputFile(szPath);
   //    outputFile += _T("\\SectionCoordinates.csv");
   //    ExportCoordinatesToExcel(vGrossGirderShapeXYPoints, outputFile);
   //}

   return pChapter;
}

void CSectionPropertiesChapterBuilder::WriteSectionProperties(rptParagraph& para, CComPtr<IShapeProperties>& shapeProps) const
{
    CEAFApp* pApp = EAFGetApp();
    const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();

    INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDispUnits->ComponentDim, true);
    INIT_UV_PROTOTYPE(rptLength2UnitValue, area, pDispUnits->Area, true);
    INIT_UV_PROTOTYPE(rptLength4UnitValue, momentOfInertia, pDispUnits->MomentOfInertia, true);

    Float64 Area;
    shapeProps->get_Area(&Area);
    Float64 Ixx;
    shapeProps->get_Ixx(&Ixx);
    Float64 Iyy;
    shapeProps->get_Iyy(&Iyy);
    Float64 Ixy;
    shapeProps->get_Ixy(&Ixy);
    Float64 yt;
    shapeProps->get_Ytop(&yt);
    Float64 yb;
    shapeProps->get_Ybottom(&yb);
    Float64 xl;
    shapeProps->get_Xleft(&xl);
    Float64 xr;
    shapeProps->get_Xright(&xr);

    CComPtr<IPoint2d> pntCG;
    shapeProps->get_Centroid(&pntCG);

    Float64 xcg, ycg;
    pntCG->Location(&xcg, &ycg);

    para << Sub2(_T("A"), _T("g")) << _T(" = ") << area.SetValue(Area) << rptNewLine;
    para << Overline(_T("x")) << _T(" = ") << length.SetValue(xcg) << rptNewLine;
    para << Overline(_T("y")) << _T(" = ") << length.SetValue(ycg) << rptNewLine;
    para << Sub2(_T("I"), _T("x")) << _T(" = ") << momentOfInertia.SetValue(Ixx) << rptNewLine;
    para << Sub2(_T("I"), _T("y")) << _T(" = ") << momentOfInertia.SetValue(Iyy) << rptNewLine;
    para << Sub2(_T("I"), _T("xy")) << _T(" = ") << momentOfInertia.SetValue(Ixy) << rptNewLine;
}


rptRcImage* CSectionPropertiesChapterBuilder::CreateImage(const std::vector<Points2D>& primaryPoints,
    const Points2D& secondaryPoints, const std::pair<Float64, Float64>& cg) const
{
    CEAFApp* pApp = EAFGetApp();
    const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();

    CImage image;
    image.Create(500, 500, 32);
    CRect rect(CPoint(0, 0), CSize(image.GetWidth(), image.GetHeight()));

    CDC* pDC = CDC::FromHandle(image.GetDC());

    // fill the background of the image (otherwise it will be black)
    CBrush brush(ALICEBLUE);
    CBrush* pOldBrush = pDC->SelectObject(&brush);
    pDC->Rectangle(rect);
    pDC->SelectObject(pOldBrush);

    WBFL::Units::LengthTool lengthTool(pDispUnits->ComponentDim);
    WBFL::Graphing::GraphXY graph(&lengthTool, &lengthTool);

    graph.SetOutputRect(rect);
    graph.SetClientAreaColor(SNOW);
    graph.SetGridPenStyle(0, 1, GRAY);

    graph.IsotropicAxes(true);
    graph.DrawGrid(true);
    graph.DrawLegend(true);
	graph.SetLegendBorderStyle(WBFL::Graphing::GraphXY::Style::None);

    //graph.SetTitle(_T("Interaction Diagram"));

    // Setup X-axis
    CString strXAxis;
    strXAxis.Format(_T("X (%s)"), pDispUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
    graph.SetXAxisTitle(strXAxis.LockBuffer());
    strXAxis.UnlockBuffer();
    graph.XAxisNiceRange(true);
    graph.SetXAxisNumberOfMinorTics(0);
    graph.SetXAxisNumberOfMajorTics(11);
    graph.SetXAxisLabelAngle(350); // 35 degrees

    // Setup Y-axis
    CString strYAxis;
    strYAxis.Format(_T("Y (%s)"), pDispUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
    graph.SetYAxisTitle(strYAxis.LockBuffer());
    strYAxis.UnlockBuffer();
    graph.YAxisNiceRange(true);
    graph.SetYAxisNumberOfMinorTics(0);
    graph.SetYAxisNumberOfMajorTics(11);

    std::vector<IndexType> primarySeries;
    primarySeries.resize(primaryPoints.size());
    for (IndexType i = 0; i < primaryPoints.size(); i++)
    {
        primarySeries[i] = graph.CreateDataSeries(_T(""), PS_SOLID, 1, BLACK);
    }
    IndexType secondarySeries = graph.CreateDataSeries(_T(""), PS_SOLID, 1, BLACK);

    std::vector<std::pair<Float64, Float64>>::const_iterator iter;
    std::vector<std::pair<Float64, Float64>>::const_iterator end;

    // Collect bounds from all points
    Float64 xMax = -Float64_Max;

    for (IndexType i = 0; i < primaryPoints.size(); i++)
    {
        iter = primaryPoints[i].begin();
        end = primaryPoints[i].end();
        for (; iter != end; iter++)
        {
            WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(iter->first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(iter->second, pDispUnits->ComponentDim.UnitOfMeasure));
            graph.AddPoint(primarySeries[i], point);

            xMax = Max(xMax, iter->first);

        }

        if (0 < primaryPoints.size())
        {
            WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(primaryPoints[i].front().first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(primaryPoints[i].front().second, pDispUnits->ComponentDim.UnitOfMeasure));
            graph.AddPoint(primarySeries[i], point);
        }
    }

    iter = secondaryPoints.begin();
    end = secondaryPoints.end();
    for (; iter != end; iter++)
    {
        WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(iter->first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(iter->second, pDispUnits->ComponentDim.UnitOfMeasure));
        graph.AddPoint(secondarySeries, point);

        xMax = Max(xMax, iter->first);
    }

    if (0 < secondaryPoints.size())
    {
        WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(secondaryPoints.front().first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(secondaryPoints.front().second, pDispUnits->ComponentDim.UnitOfMeasure));
        graph.AddPoint(secondarySeries, point);
    }

    // Create centroidal axes

    Float64 cgX = WBFL::Units::ConvertFromSysUnits(cg.first, pDispUnits->ComponentDim.UnitOfMeasure);
    Float64 cgY = WBFL::Units::ConvertFromSysUnits(cg.second, pDispUnits->ComponentDim.UnitOfMeasure);
    Float64 xMaxGraph = WBFL::Units::ConvertFromSysUnits(xMax, pDispUnits->ComponentDim.UnitOfMeasure);

    // Horizontal axis through centroid
    IndexType cgXAxisSeries = graph.CreateDataSeries(_T(""), PS_DASHDOT, 2, BLUE);
    graph.AddPoint(cgXAxisSeries, WBFL::Graphing::Point(cgX - xMaxGraph * 0.25, cgY));
    graph.AddPoint(cgXAxisSeries, WBFL::Graphing::Point(cgX + xMaxGraph * 0.25, cgY));
	graph.SetDataSeriesLabel(cgXAxisSeries, _T("Section Centroidal Axes"));

    // Vertical axis through centroid
    IndexType cgYAxisSeries = graph.CreateDataSeries(_T(""), PS_DASHDOT, 2, BLUE);
    graph.AddPoint(cgYAxisSeries, WBFL::Graphing::Point(cgX, cgY - xMaxGraph * 0.25));
    graph.AddPoint(cgYAxisSeries, WBFL::Graphing::Point(cgX, cgY + xMaxGraph * 0.25));

    // Updates the graph metrics based on "nice" axis ranges
    graph.UpdateGraphMetrics(pDC->GetSafeHdc());
    graph.Draw(pDC->GetSafeHdc());

    image.ReleaseDC();


    // get a temporary file name for the image
    TCHAR temp_path[_MAX_PATH];
    TCHAR temp_file[_MAX_PATH];
    bool should_delete = true;

    if (::GetTempPath(_MAX_PATH, temp_path) == 0)
        _tcscpy_s(temp_path, _MAX_PATH, _T("C:\\")); // Couldn't establish a temp path, just use the root drive.

    //
    // Make sure the temp path actually exists
    // We do this by looking for any file in the directory.  If nothing shows up, then the
    // path doesn't exist. (Well, this isn't exactly true, but its the best I can come up
    // with).
    CFileFind finder;
    BOOL bExist;
    CString path(temp_path);
    if (path[path.GetLength() - 1] != '\\')
        path += _T("\\");
    path += _T("*.*");
    bExist = finder.FindFile(path);
    if (!bExist)
        _tcscpy_s(temp_path, _MAX_PATH, _T("C:\\"));

    // This creates a file called _T("temp_file").TMP
    if (::GetTempFileName(temp_path, _T("gencomp_"), 0, temp_file) == 0)
    {
        // We could not get a temp name, so just use this default
        // (Use a tmp extension so it is in the same format as the one
        //  the OS would have created for us)
        _tcscpy_s(temp_file, _MAX_PATH, _T("gencomp.tmp"));
        should_delete = false;
    }

    // Replace the TMP extension with png
    std::_tstring strFilename;
    strFilename.assign(temp_file);
    strFilename.replace(strFilename.end() - 3, strFilename.end(), _T("png"));

    // We don't want the file Windows created for us
    if (should_delete)
        ::DeleteFile(temp_file);

    make_upper(strFilename.begin(), strFilename.end());

    // this is a const function so we have to cast away const-ness to save
    // the file name
    m_TemporaryFiles.push_back(strFilename);

    image.Save(strFilename.c_str(), Gdiplus::ImageFormatPNG);

    rptRcImage* pImage = new rptRcImage(strFilename.c_str(), rptRcImage::Baseline, rptRcImage::None);
    return pImage;
}