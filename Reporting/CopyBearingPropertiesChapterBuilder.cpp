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
#include <Reporting\CopyBearingPropertiesChapterBuilder.h>
#include <Reporting\BrokerReportSpecification.h>
#include <Reporting\CopyBearingPropertiesReportSpecification.h>

#include <PsgLib\GirderGroupData.h>
#include <PsgLib\BridgeDescription2.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\BearingDesignParameters.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\DocumentType.h>
#include <Reporting\ReactionInterfaceAdapters.h>
#include <AgentTools.h>


CCopyBearingPropertiesChapterBuilder::CCopyBearingPropertiesChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCopyBearingPropertiesChapterBuilder::GetName() const
{
   return TEXT("Bearing Property Comparison");
}

rptChapter* CCopyBearingPropertiesChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pBrokerRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   auto pCopyGirderPropertiesMgrRptSpec = std::dynamic_pointer_cast<const CCopyBearingPropertiesReportSpecification>(pRptSpec);



   rptChapter* pChapter = new rptChapter(GetName());
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nCols = 19;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T("Bearing Comparison"));
   *pPara << p_table << rptNewLine;

   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);


   ColumnIndexType col = 0;
   (*p_table)(0, col++) << _T("Bearing Location");
   (*p_table)(0, col++) << _T("Same") << rptNewLine << _T("as") << rptNewLine << _T("From") << rptNewLine << _T("Bearing?");
   (*p_table)(0, col++) << _T("Definition") << rptNewLine << _T("Type");
   (*p_table)(0, col++) << _T("Shape");
   (*p_table)(0, col++) << _T("# of") << rptNewLine << _T("Bearings");
   (*p_table)(0, col++) << COLHDR(_T("Length or") << rptNewLine << _T("Diameter"), 
       rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << COLHDR(_T("Width"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << COLHDR(_T("Height"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << COLHDR(_T("Recess") << rptNewLine << _T("Height"),
       rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << COLHDR(_T("Recess") << rptNewLine << _T("Length"),
       rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << COLHDR(_T("Sole") << rptNewLine << _T("Plate") << rptNewLine << _T("Height"),
       rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << COLHDR(_T("Intermediate") << rptNewLine << _T("Elastomer") << rptNewLine << _T("Thickness"),
       rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << _T("No. Int.") << rptNewLine << _T("Elastomer") << rptNewLine << _T("Layers");
   (*p_table)(0, col++) << COLHDR(_T("Elastomer") << rptNewLine << _T("Cover") << rptNewLine << _T("Thickness"),
       rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << COLHDR(_T("Steel") << rptNewLine << _T("Shim") << rptNewLine << _T("Thickness"),
       rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*p_table)(0, col++) << _T("Fixed X") << rptNewLine << _T("Translation");
   (*p_table)(0, col++) << _T("Fixed Y") << rptNewLine << _T("Translation");
   (*p_table)(0, col++) << _T("Has Externally") << rptNewLine << _T("Bonded Plates");


   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();


   RowIndexType row = 1;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
       const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(grpIdx);
       const CGirderGroupData* pFromGroup = pIBridgeDesc->GetGirderGroup(m_FromReactionLocation.GirderKey.groupIndex);

       GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
       for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
       {
           CGirderKey girderKey(grpIdx, gdrIdx);


           IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
           std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

           const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(girderKey.groupIndex);
           PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);

           ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

           // Use iterator to walk locations
           for (iter.First(); !iter.IsDone(); iter.Next())
           {
               const ReactionLocation& reactionLocation(iter.CurrentItem());

               bool isFrom = m_FromReactionLocation == reactionLocation;
               if (isFrom)
               {

                   // Color background of From bearing row
                   for (ColumnIndexType col = 0; col < nCols; col++)
                   {
                       rptRiStyle::FontColor color = rptRiStyle::Yellow;
                       (*p_table)(row, col) << new rptRcBgColor(color);
                       (*p_table)(row, col).SetFillBackGroundColor(color);
                   }

               }

               col = 0;
               CString str;
               str.Format(_T("Girder %s %s"), LABEL_GIRDER(gdrIdx), reactionLocation.PierLabel.c_str());
               (*p_table)(row, col++) << str;

               const auto& pPier = pBridgeDesc->GetPier(reactionLocation.PierIdx);

               auto face = pgsTypes::PierFaceType::Ahead;
               if (reactionLocation.Face == PierReactionFaceType::rftBack)
                   face = pgsTypes::PierFaceType::Back;

               const CBearingData2* pBearing = pPier->GetBearingData(gdrIdx, face);

               auto fromFace = pgsTypes::PierFaceType::Ahead;
               if (m_FromReactionLocation.Face == PierReactionFaceType::rftBack)
                   fromFace = pgsTypes::PierFaceType::Back;


               const auto& pFromBearing = pIBridgeDesc->GetBearingData(m_FromReactionLocation.PierIdx,
                   fromFace, m_FromReactionLocation.GirderKey.girderIndex);


               if (!isFrom)
               {
                   if (*pBearing == *pFromBearing)
                   {
                       (*p_table)(row, col++) << color(Green) << _T("Yes") << color(Black);
                   }
                   else
                   {
                       (*p_table)(row, col++) << color(Red) << _T("No") << color(Black);
                   }
               }
               else
               {
                   (*p_table)(row, col++) << symbol(NBSP);
               }


               INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetDeflectionUnit(), false);


               (*p_table)(row, col++) << (pBearing->DefinitionType == 1? _T("Detailed"): _T("Basic"));
               (*p_table)(row, col++) << (pBearing->Shape == 1? _T("Round"): _T("Rectangular"));
               (*p_table)(row, col++) << (pBearing->BearingCount);
               (*p_table)(row, col++) << (length.SetValue(pBearing->Length));
               (*p_table)(row, col++) << (length.SetValue(pBearing->Width));
               (*p_table)(row, col++) << (length.SetValue(pBearing->Height));
               (*p_table)(row, col++) << (length.SetValue(pBearing->RecessHeight));
               (*p_table)(row, col++) << (length.SetValue(pBearing->RecessLength));
               (*p_table)(row, col++) << (length.SetValue(pBearing->SolePlateHeight));
               (*p_table)(row, col++) << (length.SetValue(pBearing->ElastomerThickness));
               (*p_table)(row, col++) << (pBearing->NumIntLayers);
               (*p_table)(row, col++) << (length.SetValue(pBearing->CoverThickness));
               (*p_table)(row, col++) << (length.SetValue(pBearing->ShimThickness));
               (*p_table)(row, col++) << (pBearing->FixedX == 1? _T("Yes") : _T("No"));
               (*p_table)(row, col++) << (pBearing->FixedY == 1 ? _T("Yes") : _T("No"));
               (*p_table)(row, col++) << (pBearing->UseExtPlates == 1 ? _T("Yes") : _T("No"));

               row++;

           }
       }
   }

   return pChapter;
}

void CCopyBearingPropertiesChapterBuilder::SetCopyBearingProperties(const ReactionLocation& fromReactionLocation)
{
    m_FromReactionLocation = fromReactionLocation;
}
