///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed p SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include "stdafx.h"
#include "CopyPierPropertiesCallbacks.h"

#include <EAF\EAFUtilities.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include <IFace\DocumentType.h>

#include <PgsExt\BridgeDescription2.h>

#include <Reporter\ReportingUtils.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////// reporting utilities
inline void ColorFromRow(rptRcTable* p_table, RowIndexType row, ColumnIndexType nCols)
{
   // Color background of From pier row
   for (ColumnIndexType col = 0; col < nCols; col++)
   {
      rptRiStyle::FontColor color = rptRiStyle::Yellow;
      (*p_table)(row, col) << new rptRcBgColor(color);
      (*p_table)(row, col).SetFillBackGroundColor(color);
   }
}

// Common function to write comparison cell
inline void WriteCompareCell(rptRcTable* p_table, RowIndexType row, ColumnIndexType col, bool isFrom, bool isEqual)
{
   if (!isFrom)
   {
      if (isEqual)
      {
         (*p_table)(row, col) << color(Green) << _T("Yes") << color(Black);
      }
      else
      {
         (*p_table)(row, col) << color(Red) << _T("No") << color(Black);
      }
   }
   else
   {
      (*p_table)(row, col) << symbol(NBSP);
   }
}

static PierConnectionData MakePierConnectionData(PierCBLocType locType, const CPierData2* pPier)
{
   pgsTypes::BoundaryConditionType bcType = pPier->GetBoundaryConditionType();

   auto [backBrgOffset,backBrgOffsetMeasure] = pPier->GetBearingOffset(pgsTypes::Back, true);
   auto [aheadBrgOffset, aheadBrgOffsetMeasure] = pPier->GetBearingOffset(pgsTypes::Ahead,true);

   auto [backEndDist,backEndDistMeasure] = pPier->GetGirderEndDistance(pgsTypes::Back, true);
   auto [aheadEndDist,aheadEndDistMeasure] = pPier->GetGirderEndDistance(pgsTypes::Ahead, true);

   return PierConnectionData(locType, bcType,
                                      backEndDist,  backEndDistMeasure,  backBrgOffset,  backBrgOffsetMeasure,
                                      aheadEndDist, aheadEndDistMeasure, aheadBrgOffset, aheadBrgOffsetMeasure);
}

static PierDiaphragmData MakePierDiaphragmData(PierCBLocType locType, const CPierData2* pPier)
{
   Float64 backWidth = pPier->GetDiaphragmWidth(pgsTypes::Back);
   Float64 backHeight = pPier->GetDiaphragmHeight(pgsTypes::Back);
   Float64 backLoadLocation = pPier->GetDiaphragmLoadLocation(pgsTypes::Back);
   ConnectionLibraryEntry::DiaphragmLoadType backDiaphragmLoadType = pPier->GetDiaphragmLoadType(pgsTypes::Back);

   Float64 aheadWidth = pPier->GetDiaphragmWidth(pgsTypes::Ahead);
   Float64 aheadHeight = pPier->GetDiaphragmHeight(pgsTypes::Ahead);
   Float64 aheadLoadLocation = pPier->GetDiaphragmLoadLocation(pgsTypes::Ahead);
   ConnectionLibraryEntry::DiaphragmLoadType aheadDiaphragmLoadType = pPier->GetDiaphragmLoadType(pgsTypes::Ahead);


   return PierDiaphragmData(locType,backHeight,backWidth,backDiaphragmLoadType,backLoadLocation,aheadHeight,aheadWidth,aheadDiaphragmLoadType,aheadLoadLocation);
}

static PierModelData MakePierModelData(const CPierData2* pPier)
{
   if (pPier->GetPierModelType() == pgsTypes::pmtIdealized)
   {
      return PierModelData();
   }
   else
   {
      const CConcreteMaterial& concrete = pPier->GetConcrete();

      ColumnIndexType refColumnIdx;
      Float64 transverseOffset;
      pgsTypes::OffsetMeasurementType transverseOffsetMeasurement;
      pPier->GetTransverseOffset(&refColumnIdx, &transverseOffset, &transverseOffsetMeasurement);

      Float64 leftXBeamHeight;
      Float64 leftXBeamTaperHeight;
      Float64 leftXBeamTaperLength;
      Float64 leftXBeamEndSlopeOffset;
      Float64 leftXBeamOverhang;
      pPier->GetXBeamDimensions(pgsTypes::stLeft, &leftXBeamHeight, &leftXBeamTaperHeight, &leftXBeamTaperLength, &leftXBeamEndSlopeOffset);
      leftXBeamOverhang = pPier->GetXBeamOverhang(pgsTypes::stLeft);

      Float64 rightXBeamHeight;
      Float64 rightXBeamTaperHeight;
      Float64 rightXBeamTaperLength;
      Float64 rightXBeamEndSlopeOffset;
      Float64 rightXBeamOverhang;
      pPier->GetXBeamDimensions(pgsTypes::stRight, &rightXBeamHeight, &rightXBeamTaperHeight, &rightXBeamTaperLength, &rightXBeamEndSlopeOffset);
      rightXBeamOverhang = pPier->GetXBeamOverhang(pgsTypes::stRight);

      Float64 XBeamWidth = pPier->GetXBeamWidth();

      pgsTypes::ColumnLongitudinalBaseFixityType columnFixity = pPier->GetColumnFixity();

      CColumnData columnData = pPier->GetColumnData(0);
      // use height measurement type from first column since we are assuming they are all the same
      CColumnData::ColumnHeightMeasurementType columnHeightType = columnData.GetColumnHeightMeasurementType(); 

      PierModelData pmData(concrete, refColumnIdx, transverseOffset, transverseOffsetMeasurement,
                           leftXBeamHeight, leftXBeamTaperHeight, leftXBeamTaperLength, leftXBeamEndSlopeOffset, leftXBeamOverhang,
                           rightXBeamHeight, rightXBeamTaperHeight, rightXBeamTaperLength, rightXBeamEndSlopeOffset, rightXBeamOverhang,
                           XBeamWidth, columnHeightType, columnFixity, columnData);

      ColumnIndexType nCols = pPier->GetColumnCount();
      for (ColumnIndexType icol = 1; icol < nCols; icol++)
      {
         columnData = pPier->GetColumnData(icol);
         Float64 spacing = pPier->GetColumnSpacing(icol - 1);
         pmData.AddColumn(spacing, columnData);
      }

      return pmData;
   }
}

static PierAllData MakePierAllData(PierCBLocType locType, const CPierData2* pPier)
{
   // Build all pier comparison using make functions for each component
   PierAllData pierAllData;
   pierAllData.m_PierConnectionData = MakePierConnectionData(locType, pPier);
   pierAllData.m_PierDiaphragmData = MakePierDiaphragmData(locType, pPier);
   pierAllData.m_PierModelData = MakePierModelData(pPier);

   return pierAllData;
}


static PierCBLocType GetPierLocType(PierIndexType iPier, const CPierData2* pPier, PierIndexType nPiers)
{
   if (iPier == 0)
   {
      return pcblLeftBoundaryPier;
   }
   else if (iPier >= nPiers-1)
   {
      return pcblRightBoundaryPier;
   }
   else
   {
      // deal with case where there are no connection dimensions (continous segeents)
      if (pPier->IsInteriorPier())
      {
         pgsTypes::PierSegmentConnectionType pscType = pPier->GetSegmentConnectionType();
         if (pgsTypes::psctContinuousSegment == pscType || pgsTypes::psctIntegralSegment == pscType)
         {
            return pcblContinousSegment;
         }
      }

      return pcblInteriorPier;
   }
}

inline bool IsPierContSeg(const CPierData2* pPier)
{
   if (pPier->IsInteriorPier())
   {
      pgsTypes::PierSegmentConnectionType pscType = pPier->GetSegmentConnectionType();
      if (pgsTypes::psctContinuousSegment == pscType || pgsTypes::psctIntegralSegment == pscType)
      {
         return true;
      }
   }

   return false;
}

// Copying of boundary conditions is complex. We need to inform users with more than a bool
enum CanCopyBoundaryConditionType { ccbOkCopy, ccbPGSplice, ccbIncompatiblePierType };

static CanCopyBoundaryConditionType CanCopyBoundaryConditionData(PierIndexType fromPierIdx, const std::vector<PierIndexType>& toPiers)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // Multiple reasons why we cannot copy
   // First is that we won't even consider spliced girders (too complicated)
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);
   if (bIsSplicedGirder)
   {
      return ccbPGSplice;
   }

   // Last, check if boundary conditions supported by selected piers are compatible
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);
   pgsTypes::BoundaryConditionType fromBC = pFromPier->GetBoundaryConditionType();

   for (auto toPierIdx : toPiers)
   {
      std::vector<pgsTypes::BoundaryConditionType> toConnections(pBridgeDesc->GetBoundaryConditionTypes(toPierIdx));
      if (toConnections.end() == std::find(toConnections.begin(), toConnections.end(), fromBC))
      {
         return ccbIncompatiblePierType;
      }
   }

   return ccbOkCopy;
}

static bool CanCopyConnectionData(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // Multiple reasons why we cannot copy
   // In general, we cannot copy to or from when a segment spans a pier
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);
   if (IsPierContSeg(pFromPier))
   {
      return false;
   }

   for (auto pierIdx : toPiers)
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      if (IsPierContSeg(pPier))
      {
         return false;
      }
   }

   return true;
}


// Declaration of comparison reports
static void PierAllPropertiesComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx);
static void PierConnectionPropertiesComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers);
static void PierDiaphragmPropertiesComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx);
static void PierModelPropertiesComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx);

static void PierMaterialsComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx);
static void write_connection_abbrevation_footnotes(rptParagraph * pPara);

////////////////////////////////////////////////////
//////////////////// Transaction Classes ////////////
////////////////////////////////////////////////////

txnCopyPierAllProperties::txnCopyPierAllProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   m_FromPierIdx = fromPierIdx;
   m_ToPiers  = toPiers;
}

txnCopyPierAllProperties::~txnCopyPierAllProperties()
{
}

std::unique_ptr<CEAFTransaction> txnCopyPierAllProperties::CreateClone() const
{
   return std::make_unique<txnCopyPierAllProperties>(m_FromPierIdx,m_ToPiers);
}

std::_tstring txnCopyPierAllProperties::Name() const
{
   return _T("Copy all pier properties");
}

///////////////////////////////////////////////////////////////////////////////////////
txnCopyPierConnectionProperties::txnCopyPierConnectionProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   m_FromPierIdx = fromPierIdx;
   m_ToPiers  = toPiers;
}

txnCopyPierConnectionProperties::~txnCopyPierConnectionProperties()
{
}

bool txnCopyPierConnectionProperties::Execute()
{
   // Do nothing if data cannot be copied. This saves second guessing when we are part of a composite
   if (!CanCopyConnectionData(m_FromPierIdx, m_ToPiers))
   {
      m_DidDoCopy = false;
   }
   else
   {
      m_DidDoCopy = true;

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker, IEvents, pEvents);
      pEvents->HoldEvents(); // Large bridges can take a long time. Don't fire any changed events until all changes are done

      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      // We don't set boundary conditions for spliced girder. Too indeterminate
      CanCopyBoundaryConditionType canCopyBC = CanCopyBoundaryConditionData(m_FromPierIdx, m_ToPiers);

      m_PierConnectionData.clear();

      PierIndexType nPiers = pBridgeDesc->GetPierCount();

      // Use utility class to get and store from data
      const CPierData2* pFromPier = pBridgeDesc->GetPier(m_FromPierIdx);
      PierCBLocType fromLocType = GetPierLocType(m_FromPierIdx, pFromPier, nPiers);
      PierConnectionData fromData(MakePierConnectionData(fromLocType, pFromPier));

      for (const auto toPierIdx : m_ToPiers)
      {
         // Store old data
         Float64 backBrgOffset, aheadBrgOffset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType backBrgOffsetMeasure, aheadBrgOffsetMeasure;
         Float64 backEndDist, aheadEndDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType backEndDistMeasure, aheadEndDistMeasure;

         pIBridgeDesc->GetConnectionGeometry(toPierIdx, pgsTypes::Back, &backEndDist, &backEndDistMeasure, &backBrgOffset, &backBrgOffsetMeasure);
         pIBridgeDesc->GetConnectionGeometry(toPierIdx, pgsTypes::Ahead, &aheadEndDist, &aheadEndDistMeasure, &aheadBrgOffset, &aheadBrgOffsetMeasure);

         const CPierData2* pToPier = pBridgeDesc->GetPier(toPierIdx);

         pgsTypes::BoundaryConditionType bcType = pToPier->GetBoundaryConditionType();

         PierCBLocType locType = GetPierLocType(toPierIdx, pToPier, nPiers);

         m_PierConnectionData.push_back(PierConnectionData(locType, bcType,
            backEndDist, backEndDistMeasure, backBrgOffset, backBrgOffsetMeasure,
            aheadEndDist, aheadEndDistMeasure, aheadBrgOffset, aheadBrgOffsetMeasure));

         if (toPierIdx != m_FromPierIdx &&
            fromData.m_PierLocType != pcblContinousSegment && m_PierConnectionData.back().m_PierLocType != pcblContinousSegment) // don't set data from continuous segments
         {
            // Set new data
            if (ccbOkCopy == canCopyBC)  
            {
               pIBridgeDesc->SetBoundaryCondition(toPierIdx, fromData.m_BoundaryConditionType);
            }

            pIBridgeDesc->SetConnectionGeometry(toPierIdx, pgsTypes::Back, fromData.m_GirderEndDistance[pgsTypes::Back], fromData.m_EndDistanceMeasurementType[pgsTypes::Back], fromData.m_GirderBearingOffset[pgsTypes::Back], fromData.m_BearingOffsetMeasurementType[pgsTypes::Back]);
            pIBridgeDesc->SetConnectionGeometry(toPierIdx, pgsTypes::Ahead, fromData.m_GirderEndDistance[pgsTypes::Ahead], fromData.m_EndDistanceMeasurementType[pgsTypes::Ahead], fromData.m_GirderBearingOffset[pgsTypes::Ahead], fromData.m_BearingOffsetMeasurementType[pgsTypes::Ahead]);
         }
      }

      pEvents->FirePendingEvents();
   }

   return true;
}

void txnCopyPierConnectionProperties::Undo()
{
   if (m_DidDoCopy)  
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

      CanCopyBoundaryConditionType canCopyBC = CanCopyBoundaryConditionData(m_FromPierIdx, m_ToPiers);

      std::vector<PierConnectionData>::iterator iterPCD = m_PierConnectionData.begin();
      for (const auto toPierIdx : m_ToPiers)
      {
         PierConnectionData& toData = *iterPCD;

         if (toPierIdx != m_FromPierIdx && toData.m_PierLocType != pcblContinousSegment)
         {
            // Set new data
            if (ccbOkCopy == canCopyBC)
            {
               pIBridgeDesc->SetBoundaryCondition(toPierIdx, toData.m_BoundaryConditionType);
            }

            pIBridgeDesc->SetConnectionGeometry(toPierIdx, pgsTypes::Back, toData.m_GirderEndDistance[pgsTypes::Back], toData.m_EndDistanceMeasurementType[pgsTypes::Back], toData.m_GirderBearingOffset[pgsTypes::Back], toData.m_BearingOffsetMeasurementType[pgsTypes::Back]);
            pIBridgeDesc->SetConnectionGeometry(toPierIdx, pgsTypes::Ahead, toData.m_GirderEndDistance[pgsTypes::Ahead], toData.m_EndDistanceMeasurementType[pgsTypes::Ahead], toData.m_GirderBearingOffset[pgsTypes::Ahead], toData.m_BearingOffsetMeasurementType[pgsTypes::Ahead]);
         }

         iterPCD++;
      }
   }
}

std::unique_ptr<CEAFTransaction> txnCopyPierConnectionProperties::CreateClone() const
{
   return std::make_unique<txnCopyPierConnectionProperties>(m_FromPierIdx,m_ToPiers);
}

std::_tstring txnCopyPierConnectionProperties::Name() const
{
   return _T("txnCopyPierConnectionProperties");
}

///////////////////////////////////////////////////////////////////////////////////////
txnCopyPierDiaphragmProperties::txnCopyPierDiaphragmProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   m_FromPierIdx = fromPierIdx;
   m_ToPiers  = toPiers;
}

txnCopyPierDiaphragmProperties::~txnCopyPierDiaphragmProperties()
{
}

bool txnCopyPierDiaphragmProperties::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // Large bridges can take a long time. Don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   m_PierDiaphragmData.clear();

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   // Use utility class to get and store from data
   const CPierData2* pFromPier = pBridgeDesc->GetPier(m_FromPierIdx);
   PierCBLocType fromLocType = GetPierLocType(m_FromPierIdx, pFromPier, nPiers);
   PierDiaphragmData fromData( MakePierDiaphragmData(fromLocType, pFromPier) );

   for (const auto toPierIdx : m_ToPiers)
   {
      // Store old data
      Float64 backWidth, backHeight, backLoadLocation;
      ConnectionLibraryEntry::DiaphragmLoadType backDiaphragmLoadType;
      pIBridgeDesc->GetPierDiaphragmData(toPierIdx, pgsTypes::Back, &backHeight, &backWidth, &backDiaphragmLoadType, &backLoadLocation);

      Float64 aheadWidth, aheadHeight, aheadLoadLocation;
      ConnectionLibraryEntry::DiaphragmLoadType aheadDiaphragmLoadType;
      pIBridgeDesc->GetPierDiaphragmData(toPierIdx, pgsTypes::Ahead, &aheadHeight, &aheadWidth, &aheadDiaphragmLoadType, &aheadLoadLocation);

      const CPierData2* pToPier = pBridgeDesc->GetPier(toPierIdx);
      PierCBLocType locType = GetPierLocType(toPierIdx, pToPier, nPiers);

      m_PierDiaphragmData.push_back(PierDiaphragmData(locType, backHeight, backWidth,  backDiaphragmLoadType, backLoadLocation, aheadHeight, aheadWidth,  aheadDiaphragmLoadType, aheadLoadLocation));

      if (toPierIdx != m_FromPierIdx)
      {
         // Need to play same game that pier dialog uses with continous segments over piers. The load must be centered
         ConnectionLibraryEntry::DiaphragmLoadType backLoadType, aheadLoadType;
         if (locType == pcblContinousSegment)
         {
            backLoadType  = ConnectionLibraryEntry::ApplyAtBearingCenterline;
            aheadLoadType = ConnectionLibraryEntry::ApplyAtBearingCenterline;
         }
         else
         {
            backLoadType = fromData.m_DiaphragmLoadType[pgsTypes::Back];
            aheadLoadType = fromData.m_DiaphragmLoadType[pgsTypes::Ahead];
         }

         // Set new data
         pIBridgeDesc->SetPierDiaphragmData(toPierIdx, pgsTypes::Back, fromData.m_Height[pgsTypes::Back], fromData.m_Width[pgsTypes::Back], backLoadType, fromData.m_LoadLocation[pgsTypes::Back]);
         pIBridgeDesc->SetPierDiaphragmData(toPierIdx, pgsTypes::Ahead, fromData.m_Height[pgsTypes::Ahead], fromData.m_Width[pgsTypes::Ahead], aheadLoadType, fromData.m_LoadLocation[pgsTypes::Ahead]);
      }
   }

   pEvents->FirePendingEvents();

   return true;
}

void txnCopyPierDiaphragmProperties::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::vector<PierDiaphragmData>::iterator iterPCD = m_PierDiaphragmData.begin();
   for (const auto toPierIdx : m_ToPiers)
   {
      PierDiaphragmData& toData = *iterPCD;

      if (toPierIdx != m_FromPierIdx)
      {
         // Set new data
         pIBridgeDesc->SetPierDiaphragmData(toPierIdx, pgsTypes::Back, toData.m_Height[pgsTypes::Back], toData.m_Width[pgsTypes::Back], toData.m_DiaphragmLoadType[pgsTypes::Back], toData.m_LoadLocation[pgsTypes::Back]);
         pIBridgeDesc->SetPierDiaphragmData(toPierIdx, pgsTypes::Ahead, toData.m_Height[pgsTypes::Ahead], toData.m_Width[pgsTypes::Ahead], toData.m_DiaphragmLoadType[pgsTypes::Ahead], toData.m_LoadLocation[pgsTypes::Ahead]);
      }

      iterPCD++;
   }
}

std::unique_ptr<CEAFTransaction> txnCopyPierDiaphragmProperties::CreateClone() const
{
   return std::make_unique<txnCopyPierDiaphragmProperties>(m_FromPierIdx,m_ToPiers);
}

std::_tstring txnCopyPierDiaphragmProperties::Name() const
{
   return _T("txnCopyPierDiaphragmProperties");
}

///////////////////////////////////////////////////////////////////////////////////////
txnCopyPierModelProperties::txnCopyPierModelProperties(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   m_FromPierIdx = fromPierIdx;
   m_ToPiers  = toPiers;
}

txnCopyPierModelProperties::~txnCopyPierModelProperties()
{
}

bool txnCopyPierModelProperties::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // Large bridges can take a long time. Don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   m_PierData.clear();

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   // Use utility class to get From data
   const CPierData2* pFromPier = pBridgeDesc->GetPier(m_FromPierIdx);
   PierModelData fromData( MakePierModelData(pFromPier) );

   for (const auto toPierIdx : m_ToPiers)
   {
      // Store old data
      CPierData2 oldPierData = *(pBridgeDesc->GetPier(toPierIdx));

      m_PierData.push_back(oldPierData);

      if (toPierIdx != m_FromPierIdx)
      {
         // Set new data
         oldPierData.SetPierModelType(fromData.m_PierModelType);

         if (pgsTypes::pmtPhysical == fromData.m_PierModelType)
         {
            oldPierData.SetTransverseOffset(fromData.m_RefColumnIdx, fromData.m_TransverseOffset, fromData.m_TransverseOffsetMeasurement);

            oldPierData.SetXBeamWidth(fromData.m_XBeamWidth);
            oldPierData.SetXBeamDimensions(pgsTypes::stLeft, fromData.m_XBeamHeight[pgsTypes::stLeft], fromData.m_XBeamTaperHeight[pgsTypes::stLeft], fromData.m_XBeamTaperLength[pgsTypes::stLeft], fromData.m_XBeamEndSlopeOffset[pgsTypes::stLeft]);
            oldPierData.SetXBeamOverhang(pgsTypes::stLeft, fromData.m_XBeamOverhang[pgsTypes::stLeft]);

            oldPierData.SetXBeamDimensions(pgsTypes::stRight, fromData.m_XBeamHeight[pgsTypes::stRight], fromData.m_XBeamTaperHeight[pgsTypes::stRight], fromData.m_XBeamTaperLength[pgsTypes::stRight], fromData.m_XBeamEndSlopeOffset[pgsTypes::stRight]);
            oldPierData.SetXBeamOverhang(pgsTypes::stRight, fromData.m_XBeamOverhang[pgsTypes::stRight]);

            oldPierData.SetColumnFixity(fromData.m_ColumnFixity);

            // After removal, CPierData2 will retain one column
            oldPierData.RemoveColumns();
            oldPierData.SetColumnData(0, fromData.m_Columns[0]);

            ColumnIndexType ncols = fromData.m_Columns.size();

            for (ColumnIndexType icol = 1; icol < ncols; icol++)
            {
               oldPierData.AddColumn(fromData.m_ColumnSpacing[icol - 1], fromData.m_Columns[icol]);
            }
         }

         pIBridgeDesc->SetPierByIndex(toPierIdx, oldPierData);

      }
   }

   pEvents->FirePendingEvents();

   return true;
}

void txnCopyPierModelProperties::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   std::vector<CPierData2>::iterator iterPCD = m_PierData.begin();
   for (const auto toPierIdx : m_ToPiers)
   {
      const CPierData2& toData = *iterPCD;

      if (toPierIdx != m_FromPierIdx)
      {
         // Set new data
         pIBridgeDesc->SetPierByIndex(toPierIdx, toData);
      }

      iterPCD++;
   }
}

std::unique_ptr<CEAFTransaction> txnCopyPierModelProperties::CreateClone() const
{
   return std::make_unique<txnCopyPierModelProperties>(m_FromPierIdx,m_ToPiers);
}

std::_tstring txnCopyPierModelProperties::Name() const
{
   return _T("txnCopyPierModelProperties");
}


///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Callback Classes    /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

CCopyPierAllProperties::CCopyPierAllProperties()
{
}

LPCTSTR CCopyPierAllProperties::GetName()
{
   return _T("All Pier Properties");
}

BOOL CCopyPierAllProperties::CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   return CanCopyConnectionData(fromPierIdx, toPiers);
}

std::unique_ptr<CEAFTransaction> CCopyPierAllProperties::CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   return std::make_unique<txnCopyPierAllProperties>(fromPierIdx, toPiers);
}

UINT CCopyPierAllProperties::GetPierEditorTabIndex()
{
   return EPD_GENERAL;
}

rptParagraph* CCopyPierAllProperties::BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   rptParagraph* pPara = new rptParagraph;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   PierAllPropertiesComparison(pPara, pBroker, fromPierIdx);

   return pPara;
}

///////////////////////////////////////////////////////////////////////////////////////

CCopyPierConnectionProperties::CCopyPierConnectionProperties()
{
}

LPCTSTR CCopyPierConnectionProperties::GetName()
{
   return _T("Connection Properties");
}

BOOL CCopyPierConnectionProperties::CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   return CanCopyConnectionData(fromPierIdx, toPiers);
}

std::unique_ptr<CEAFTransaction> CCopyPierConnectionProperties::CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   return std::make_unique<txnCopyPierConnectionProperties>(fromPierIdx, toPiers);
}

UINT CCopyPierConnectionProperties::GetPierEditorTabIndex()
{
   return EPD_CONNECTION;
}

rptParagraph* CCopyPierConnectionProperties::BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   rptParagraph* pPara = new rptParagraph;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   PierConnectionPropertiesComparison(pPara, pBroker, fromPierIdx, toPiers);

   return pPara;
}

///////////////////////////////////////////////////////////////////////////////////////

CCopyPierDiaphragmProperties::CCopyPierDiaphragmProperties()
{
}

LPCTSTR CCopyPierDiaphragmProperties::GetName()
{
   return _T("Diaphragms");
}

BOOL CCopyPierDiaphragmProperties::CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   return TRUE;
}

std::unique_ptr<CEAFTransaction> CCopyPierDiaphragmProperties::CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   return std::make_unique<txnCopyPierDiaphragmProperties>(fromPierIdx, toPiers);
}

UINT CCopyPierDiaphragmProperties::GetPierEditorTabIndex()
{
   return EPD_CONNECTION;
}

rptParagraph* CCopyPierDiaphragmProperties::BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   rptParagraph* pPara = new rptParagraph;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   PierDiaphragmPropertiesComparison(pPara, pBroker, fromPierIdx);

   return pPara;
}

///////////////////////////////////////////////////////////////////////////////////////

CCopyPierModelProperties::CCopyPierModelProperties()
{
}

LPCTSTR CCopyPierModelProperties::GetName()
{
   return _T("Pier Type and Dimensions");
}

BOOL CCopyPierModelProperties::CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   // this one we can always do
   return TRUE;
}

std::unique_ptr<CEAFTransaction> CCopyPierModelProperties::CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   return std::make_unique<txnCopyPierModelProperties>(fromPierIdx, toPiers);
}

UINT CCopyPierModelProperties::GetPierEditorTabIndex()
{
   return EPD_LAYOUT;
}

rptParagraph* CCopyPierModelProperties::BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   rptParagraph* pPara = new rptParagraph;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   PierModelPropertiesComparison(pPara, pBroker, fromPierIdx);

   return pPara;
}


/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Reporting functions /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void PierAllPropertiesComparison(rptParagraph * pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ColumnIndexType nCols = 3;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T("Pier Properties"));
   *pPara << p_table << rptNewLine;

   ColumnIndexType col = 0;
   (*p_table)(0, col++) << _T("Pier");
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Pier?");
   (*p_table)(0, col++) << _T("Pier") << rptNewLine << _T("Model") << rptNewLine << _T("Type");

   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   if (fromPierIdx > nPiers - 1)
   {
      ATLASSERT(0); // this should never happen
   }

   // use utility class to store data and for comparisons
   const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);
   PierCBLocType fromLocType = GetPierLocType(fromPierIdx, pFromPier, nPiers);

   PierAllData fromPierAllData(MakePierAllData(fromLocType, pFromPier));

   RowIndexType irow = p_table->GetNumberOfHeaderRows();

   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   while (pPier != nullptr)
   {
      col = 0;
      PierIndexType pierIdx = pPier->GetIndex();

      bool isFrom = pierIdx == fromPierIdx;
      if (isFrom)
      {
         ColorFromRow(p_table, irow, nCols);
      }

      bool bAbutment = pPier->IsAbutment();
      (*p_table)(irow, col++) << pgsPierLabel::GetPierLabelEx(bAbutment, pierIdx);

      // use utility class to store data and for comparisons
      PierCBLocType locType = GetPierLocType(pierIdx, pPier, nPiers);
      PierAllData pierAllData(MakePierAllData(locType, pPier));

      bool isEqual = fromPierAllData.ArePiersEqual(pierAllData);

      WriteCompareCell(p_table, irow, col++, isFrom, isEqual);

      pgsTypes::PierModelType type = pPier->GetPierModelType();
      if (type == pgsTypes::pmtIdealized)
      {
         (*p_table)(irow, col++) << _T("Idealized");
      }
      else if (type == pgsTypes::pmtPhysical)
      {
         (*p_table)(irow, col++) << _T("Physical Model");
      }
      else
      {
         ATLASSERT(0);
         (*p_table)(irow, col++) << _T("Invalid Type");
      }

      if ( pPier->GetNextSpan() )
      {
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }

      irow++;

   }
}

void PierConnectionPropertiesComparison(rptParagraph * pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false );

   ColumnIndexType nCols = 11;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T("Pier Connections"));
   *pPara << p_table;

   p_table->SetNumberOfHeaderRows(2);

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   p_table->SetRowSpan(0,col++,2);
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Pier?");
   (*p_table)(0,col++) << _T("Boundary") << rptNewLine <<  _T("Condition");

   p_table->SetColumnSpan(0,col,4);
   (*p_table)(0,col) << _T("Back");
   (*p_table)(1,col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*p_table)(1,col++) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");

   p_table->SetColumnSpan(0,col,4);
   (*p_table)(0,col) << _T("Ahead");
   (*p_table)(1,col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*p_table)(1,col++) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");

   RowIndexType irow = p_table->GetNumberOfHeaderRows();

   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   if (fromPierIdx > nPiers - 1)
   {
      ATLASSERT(0); // this should never happen
   }

   const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);
   PierCBLocType fromLocType = GetPierLocType(fromPierIdx, pFromPier, nPiers);

   // use utility class to store data and for comparisons
   PierConnectionData fromPierConnectionData( MakePierConnectionData(fromLocType, pFromPier) );

   for (PierIndexType pierIdx = 0; pierIdx<nPiers; pierIdx++)
   {
      col = 0;

      bool isFrom = pierIdx == fromPierIdx;
      if (isFrom)
      {
         ColorFromRow(p_table, irow, nCols);
      }

      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

      bool bAbutment = pPier->IsAbutment();
      (*p_table)(irow, col++) << pgsPierLabel::GetPierLabelEx(bAbutment, pierIdx);

      // use utility class to store data and for comparisons
      PierCBLocType locType = GetPierLocType(pierIdx, pPier, nPiers);
      PierConnectionData pierConnectionData( MakePierConnectionData(locType, pPier) );

      bool isEqual = fromPierConnectionData.AreConnectionsEqual(pierConnectionData);

      WriteCompareCell(p_table, irow, col++, isFrom, isEqual);


      // Boundary condition
      if (pPier->IsInteriorPier())
      {
         (*p_table)(irow, col++) << CPierData2::AsString(pPier->GetSegmentConnectionType());
      }
      else
      {
         bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());
         (*p_table)(irow, col++) << CPierData2::AsString(pPier->GetBoundaryConditionType(), bNoDeck);
      }

      if (pierConnectionData.m_PierLocType==pcblContinousSegment)
      {
         p_table->SetColumnSpan(irow,col,8);
         (*p_table)(irow, col++) << _T("Segment is continuous over pier");
      }
      // back side
      else
      {
         if (pPier->GetPrevSpan())
         {
            (*p_table)(irow, col++) << cmpdim.SetValue(pierConnectionData.m_GirderBearingOffset[pgsTypes::Back]);
            (*p_table)(irow, col++) << GetBearingOffsetMeasureString(pierConnectionData.m_BearingOffsetMeasurementType[pgsTypes::Back], bAbutment, true);

            (*p_table)(irow, col++) << cmpdim.SetValue(pierConnectionData.m_GirderEndDistance[pgsTypes::Back]);
            (*p_table)(irow, col++) << GetEndDistanceMeasureString(pierConnectionData.m_EndDistanceMeasurementType[pgsTypes::Back], bAbutment, true);
         }
         else
         {
            (*p_table)(irow, col++) << _T("");
            (*p_table)(irow, col++) << _T("");
            (*p_table)(irow, col++) << _T("");
            (*p_table)(irow, col++) << _T("");
         }

         // Ahead side
         if (pPier->GetNextSpan())
         {
            (*p_table)(irow, col++) << cmpdim.SetValue(pierConnectionData.m_GirderBearingOffset[pgsTypes::Ahead]);
            (*p_table)(irow, col++) << GetBearingOffsetMeasureString(pierConnectionData.m_BearingOffsetMeasurementType[pgsTypes::Ahead], bAbutment, true);

            (*p_table)(irow, col++) << cmpdim.SetValue(pierConnectionData.m_GirderEndDistance[pgsTypes::Ahead]);
            (*p_table)(irow, col++) << GetEndDistanceMeasureString(pierConnectionData.m_EndDistanceMeasurementType[pgsTypes::Ahead], bAbutment, true);
         }
         else
         {
            (*p_table)(irow, col++) << _T("");
            (*p_table)(irow, col++) << _T("");
            (*p_table)(irow, col++) << _T("");
            (*p_table)(irow, col++) << _T("");
         }
      }

      if ( pPier->GetNextSpan() )
      {
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }

      irow++;
   }

   // Write note if BC cannot be copied
   CanCopyBoundaryConditionType canCopyBC = CanCopyBoundaryConditionData(fromPierIdx, toPiers);
   if (canCopyBC == ccbPGSplice)
   {
      *pPara << color(Red) << (_T("Note: Boundary conditions will not be copied in spliced girder structures")) << color(Black) << rptNewLine;
   }
   else if (canCopyBC == ccbIncompatiblePierType)
   {
      *pPara << color(Red) << _T("Note: Boundary conditions will not be copied. Pier/BC Types are incompatible between selected piers.") << color(Black) << rptNewLine;
   }

   *pPara << rptNewLine;

   write_connection_abbrevation_footnotes(pPara);

   *pPara << rptNewLine;
}

void PierDiaphragmPropertiesComparison(rptParagraph * pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false );

   ColumnIndexType nCols = 10;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Pier Diaphragms"));
   *pPara << p_table << rptNewLine;

   p_table->SetNumberOfHeaderRows(2);

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   p_table->SetRowSpan(0,col++,2);
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Pier?");

   p_table->SetColumnSpan(0,col,4);
   (*p_table)(0,col) << _T("Diaphragm - Back");
   (*p_table)(1,col++) << COLHDR(_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << COLHDR(_T("Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << _T("Loading");
   (*p_table)(1,col++) << COLHDR(_T("Location(*)"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   p_table->SetColumnSpan(0,col,4);
   (*p_table)(0,col) << _T("Diaphragm - Ahead");
   (*p_table)(1,col++) << COLHDR(_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << COLHDR(_T("Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*p_table)(1,col++) << _T("Loading");
   (*p_table)(1,col++) << COLHDR(_T("Location(*)"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   if (fromPierIdx > nPiers - 1)
   {
      ATLASSERT(0); // this should never happen
   }

   const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);
   PierCBLocType fromLocType = GetPierLocType(fromPierIdx, pFromPier, nPiers);

   // use utility class to store data and for comparisons
   PierDiaphragmData fromPierDiaphragmData( MakePierDiaphragmData(fromLocType, pFromPier) );

   for (PierIndexType pierIdx = 0; pierIdx<nPiers; pierIdx++)
   {
      col = 0;
      bool isFrom = pierIdx == fromPierIdx;
      if (isFrom)
      {
         ColorFromRow(p_table, row, nCols);
      }

      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

      bool bAbutment = pPier->IsAbutment();
      (*p_table)(row, col++) << LABEL_PIER_EX(bAbutment, pierIdx);

      // use utility class to store data and for comparisons
      PierCBLocType locType = GetPierLocType(pierIdx, pPier, nPiers);
      PierDiaphragmData pierDiaphragmData( MakePierDiaphragmData(locType, pPier) );

      bool isEqual = fromPierDiaphragmData.AreDiaphragmsEqual(pierDiaphragmData);

      WriteCompareCell(p_table, row, col++, isFrom, isEqual);

      if (pPier->GetPrevSpan())
      {
         if (pPier->GetDiaphragmHeight(pgsTypes::Back) < 0)
         {
            (*p_table)(row, col++) << _T("Compute");
         }
         else
         {
            (*p_table)(row, col++) << cmpdim.SetValue(pPier->GetDiaphragmHeight(pgsTypes::Back));
         }

         if (pPier->GetDiaphragmWidth(pgsTypes::Back) < 0)
         {
            (*p_table)(row, col++) << _T("Compute");
         }
         else
         {
            (*p_table)(row, col++) << cmpdim.SetValue(pPier->GetDiaphragmWidth(pgsTypes::Back));
         }

         switch (pPier->GetDiaphragmLoadType(pgsTypes::Back))
         {
         case ConnectionLibraryEntry::ApplyAtBearingCenterline:
            (*p_table)(row, col++) << _T("Apply load at centerline bearing");
            (*p_table)(row, col++) << RPT_NA;
            break;
         case ConnectionLibraryEntry::ApplyAtSpecifiedLocation:
            (*p_table)(row, col++) << _T("Apply load to girder");
            (*p_table)(row, col++) << cmpdim.SetValue(pPier->GetDiaphragmLoadLocation(pgsTypes::Back));
            break;
         case ConnectionLibraryEntry::DontApply:
            (*p_table)(row, col++) << _T("Ignore weight");
            (*p_table)(row, col++) << RPT_NA;
            break;
         default:
            ATLASSERT(false); // is there a new type????
         }
      }
      else
      {
         (*p_table)(row, col++) << _T("");
         (*p_table)(row, col++) << _T("");
         (*p_table)(row, col++) << _T("");
         (*p_table)(row, col++) << _T("");
      }

      if (pPier->GetNextSpan())
      {
         if (pPier->GetDiaphragmHeight(pgsTypes::Ahead) < 0)
         {
            (*p_table)(row, col++) << _T("Compute");
         }
         else
         {
            (*p_table)(row, col++) << cmpdim.SetValue(pPier->GetDiaphragmHeight(pgsTypes::Ahead));
         }

         if (pPier->GetDiaphragmWidth(pgsTypes::Ahead) < 0)
         {
            (*p_table)(row, col++) << _T("Compute");
         }
         else
         {
            (*p_table)(row, col++) << cmpdim.SetValue(pPier->GetDiaphragmWidth(pgsTypes::Ahead));
         }

         switch (pPier->GetDiaphragmLoadType(pgsTypes::Ahead))
         {
         case ConnectionLibraryEntry::ApplyAtBearingCenterline:
            (*p_table)(row, col++) << _T("Apply load at centerline bearing");
            (*p_table)(row, col++) << RPT_NA;
            break;
         case ConnectionLibraryEntry::ApplyAtSpecifiedLocation:
            (*p_table)(row, col++) << _T("Apply load to girder");
            (*p_table)(row, col++) << cmpdim.SetValue(pPier->GetDiaphragmLoadLocation(pgsTypes::Ahead));
            break;
         case ConnectionLibraryEntry::DontApply:
            (*p_table)(row, col++) << _T("Ignore weight");
            (*p_table)(row, col++) << RPT_NA;
            break;
         default:
            ATLASSERT(false); // is there a new type????
         }
      }
      else
      {
         (*p_table)(row, col++) << _T("");
         (*p_table)(row, col++) << _T("");
         (*p_table)(row, col++) << _T("");
         (*p_table)(row, col++) << _T("");
      }

      if ( pPier->GetNextSpan() )
      {
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }

      row++;
   }
}

void PierBearingPropertiesComparison(rptParagraph * pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx)
{
}

void PierModelPropertiesComparison(rptParagraph * pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   INIT_UV_PROTOTYPE(rptLengthUnitValue, spandim, pDisplayUnits->GetSpanLengthUnit(), false);

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   bool isAnyPhysical(false);
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      pgsTypes::PierModelType type = pPier->GetPierModelType();
      isAnyPhysical |= type == pgsTypes::pmtPhysical;
   }


   // Cap Dimensions
   ColumnIndexType nCols = isAnyPhysical ? 14 : 3;
   rptRcTable* p_capdim_table = rptStyleManager::CreateDefaultTable(nCols, (isAnyPhysical ? _T("Pier Model Type and Cap Geometry"): _T("Pier Model Type")));
   *pPara << p_capdim_table << rptNewLine;

   p_capdim_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_capdim_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 1;
   (*p_capdim_table)(0, col++) << _T("Same") << rptNewLine << _T("as") << rptNewLine << _T("From") << rptNewLine << _T("Pier?");
   (*p_capdim_table)(0, col++) << _T("Model") << rptNewLine << _T("Type");

   if (isAnyPhysical)
   {
      (*p_capdim_table)(0, col++) << COLHDR(_T("H1"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("H2"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("H3"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("H4"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("X1"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("X2"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("X3"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("X4"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("X5"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("X6"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_capdim_table)(0, col++) << COLHDR(_T("W"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   RowIndexType row = p_capdim_table->GetNumberOfHeaderRows();

   if (fromPierIdx > nPiers - 1)
   {
      ATLASSERT(0); // this should never happen
   }

   const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);

   // use utility class to store data and for comparisons
   PierModelData fromPierModelData ( MakePierModelData(pFromPier) );

   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      col = 0;
      bool isFrom = pierIdx == fromPierIdx;
      if (isFrom)
      {
         ColorFromRow(p_capdim_table, row, nCols);
      }

      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

      bool bAbutment = pPier->IsAbutment();
      (*p_capdim_table)(row, col++) << LABEL_PIER_EX(bAbutment, pierIdx);

      PierModelData pierModelData ( MakePierModelData(pPier) );

      bool isEqual = fromPierModelData.ArePierCapDimensionsEqual(pierModelData);

      WriteCompareCell(p_capdim_table, row, col++, isFrom, isEqual);

      pgsTypes::PierModelType type = pPier->GetPierModelType();

      if (type == pgsTypes::pmtIdealized)
      {
         (*p_capdim_table)(row, col++) << _T("Idealized");
         if (isAnyPhysical)
         {
            p_capdim_table->SetColumnSpan(row, col, nCols - col);
            (*p_capdim_table)(row, col++) << _T(" ");
         }
      }
      else if (type == pgsTypes::pmtPhysical)
      {
         (*p_capdim_table)(row, col++) << _T("Physical Model");

         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamHeight[pgsTypes::stLeft]);      // H1
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamTaperHeight[pgsTypes::stLeft]);
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamHeight[pgsTypes::stRight]);
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamTaperHeight[pgsTypes::stRight]);

         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamTaperLength[pgsTypes::stLeft]);  // X1
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamEndSlopeOffset[pgsTypes::stLeft]);
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamTaperLength[pgsTypes::stRight]); // X3
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamEndSlopeOffset[pgsTypes::stRight]);
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamOverhang[pgsTypes::stLeft]);     // X5
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamOverhang[pgsTypes::stRight]);
         (*p_capdim_table)(row, col++) << spandim.SetValue(pierModelData.m_XBeamWidth); // W
      }

      row++;
   }

   if (isAnyPhysical)
   {
      // Gross column layouts
      ColumnIndexType nCols = 7;
      rptRcTable* p_col_layout_table = rptStyleManager::CreateDefaultTable(nCols, _T("Pier Column Layouts"));
      *pPara << p_col_layout_table << rptNewLine;

      p_col_layout_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_col_layout_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      ColumnIndexType col = 1;
      (*p_col_layout_table)(0, col++) << _T("Same") << rptNewLine << _T("as") << rptNewLine << _T("From") << rptNewLine << _T("Pier?");
      (*p_col_layout_table)(0, col++) << _T("Base of Column") << rptNewLine << _T("Defined by");
      (*p_col_layout_table)(0, col++) << _T("Long. Base") << rptNewLine << _T("Fixity");
      (*p_col_layout_table)(0, col++) << _T("Locator") << rptNewLine << _T("Column") << rptNewLine << _T("#");
      (*p_col_layout_table)(0, col++) << COLHDR(_T("Is") << rptNewLine << _T("Located"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_col_layout_table)(0, col++) << _T("Datum");

      RowIndexType row = p_col_layout_table->GetNumberOfHeaderRows();

      PierIndexType nPiers = pBridgeDesc->GetPierCount();
      if (fromPierIdx > nPiers - 1)
      {
         ATLASSERT(0); // this should never happen
      }

      const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);

      // use utility class to store data and for comparisons
      PierModelData fromPierModelData ( MakePierModelData(pFromPier) );

      for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
      {
         col = 0;
         bool isFrom = pierIdx == fromPierIdx;
         if (isFrom)
         {
            ColorFromRow(p_col_layout_table, row, nCols);
         }

         const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

         bool bAbutment = pPier->IsAbutment();
         (*p_col_layout_table)(row, col++) << LABEL_PIER_EX(bAbutment, pierIdx);

         PierModelData pierModelData ( MakePierModelData(pPier) );

         bool isEqual = fromPierModelData.AreColumnLayoutsEqual(pierModelData);

         WriteCompareCell(p_col_layout_table, row, col++, isFrom, isEqual);

         pgsTypes::PierModelType type = pPier->GetPierModelType();

         if (type == pgsTypes::pmtIdealized)
         {
            p_col_layout_table->SetColumnSpan(row, col, nCols-col);
            (*p_col_layout_table)(row, col++) << RPT_NA;
         }
         else if (type == pgsTypes::pmtPhysical)
         {
            (*p_col_layout_table)(row, col++) << (pierModelData.m_ColumnHeightType == CColumnData::chtBottomElevation ? _T("Bottom Elevation") : _T("Column Height (H)"));
            (*p_col_layout_table)(row, col++) << (pierModelData.m_ColumnFixity == pgsTypes::cftFixed ? _T("Fixed") : _T("Hinged"));
            (*p_col_layout_table)(row, col++) << pierModelData.m_RefColumnIdx + 1;
            (*p_col_layout_table)(row, col++) << spandim.SetValue(pierModelData.m_TransverseOffset);
            (*p_col_layout_table)(row, col++) << (pierModelData.m_TransverseOffsetMeasurement == pgsTypes::omtAlignment ? _T("from the Alignment") : _T("from the Bridge Line"));
         }

         row++;
      }

      // Column details
      nCols = 9;
      rptRcTable* p_col_details_table = rptStyleManager::CreateDefaultTable(nCols, _T("Pier Column Details"));
      *pPara << p_col_details_table << rptNewLine;

      p_col_details_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_col_details_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      col = 1;
      (*p_col_details_table)(0, col++) << _T("Same") << rptNewLine << _T("as") << rptNewLine << _T("From") << rptNewLine << _T("Pier?");
      (*p_col_details_table)(0, col++) << _T("Column") << rptNewLine << _T("#");
      (*p_col_details_table)(0, col++) << COLHDR(_T("Column") << rptNewLine << _T("Height"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_col_details_table)(0, col++) << _T("Transverse") << rptNewLine << _T("Fixity");
      (*p_col_details_table)(0, col++) << _T("Shape");
      (*p_col_details_table)(0, col++) << COLHDR(_T("Diameter") << rptNewLine << _T("or Width"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_col_details_table)(0, col++) << COLHDR(_T("Depth"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*p_col_details_table)(0, col++) << COLHDR(_T("Spacing"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

      row = p_col_details_table->GetNumberOfHeaderRows();

      nPiers = pBridgeDesc->GetPierCount();
      if (fromPierIdx > nPiers - 1)
      {
         ATLASSERT(0); // this should never happen
      }

      pFromPier = pBridgeDesc->GetPier(fromPierIdx);

      // use utility class to store data and for comparisons
      fromPierModelData = MakePierModelData(pFromPier);

      for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
      {
         col = 0;
         bool isFrom = pierIdx == fromPierIdx;
         if (isFrom)
         {
            ColorFromRow(p_col_details_table, row, nCols);
         }

         const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

         bool bAbutment = pPier->IsAbutment();
         (*p_col_details_table)(row, col++) << LABEL_PIER_EX(bAbutment, pierIdx);

         PierModelData pierModelData ( MakePierModelData(pPier) );

         bool isEqual = fromPierModelData.AreColumnLayoutsEqual(pierModelData);

         WriteCompareCell(p_col_details_table, row, col++, isFrom, isEqual);

         pgsTypes::PierModelType type = pPier->GetPierModelType();

         if (type == pgsTypes::pmtIdealized)
         {
            p_col_details_table->SetColumnSpan(row, col, nCols-col);
            (*p_col_details_table)(row, col++) << RPT_NA;
         }
         else if (type == pgsTypes::pmtPhysical)
         {
            IndexType colno = 1;
            ColumnIndexType lastcol;
            for (auto& colData : pierModelData.m_Columns)
            {
               ColumnIndexType loccol = col;
               (*p_col_details_table)(row, loccol++) << colno++ << rptNewLine;
               (*p_col_details_table)(row, loccol++) << spandim.SetValue(colData.GetColumnHeight()) << rptNewLine;
               pgsTypes::ColumnTransverseFixityType fixity = colData.GetTransverseFixity();
               CString strfix;
               if (pgsTypes::ctftTopFixedBottomFixed == fixity)
               {
                  strfix = _T("Top and Bottom Fixed");
               }
               else if (pgsTypes::ctftTopFixedBottomPinned == fixity)
               {
                  strfix = _T("Top Fixed, Bottom Pinned");
               }
               else if (pgsTypes::ctftTopPinnedBottomFixed == fixity)
               {
                  strfix = _T("Top Pinned, Bottom Fixed");
               }

               (*p_col_details_table)(row, loccol++) << strfix << rptNewLine;
               (*p_col_details_table)(row, loccol++) << (colData.GetColumnShape() == CColumnData::cstCircle ? _T("Circle") : _T("Rectangle")) << rptNewLine;
               Float64 width, depth;
               colData.GetColumnDimensions(&width, &depth);
               (*p_col_details_table)(row, loccol++) << spandim.SetValue(width) << rptNewLine;
               if (colData.GetColumnShape() != CColumnData::cstCircle)
               {
                  (*p_col_details_table)(row, loccol++) << spandim.SetValue(depth) << rptNewLine;
               }
               else
               {
                  (*p_col_details_table)(row, loccol++) << _T(" ") << rptNewLine;
               }

               lastcol = loccol;
            }

            for (auto& spacing : pierModelData.m_ColumnSpacing)
            {
               (*p_col_details_table)(row, lastcol) << spandim.SetValue(spacing) << rptNewLine;
            }
         }

         row++;
      }

      PierMaterialsComparison(pPara, pBroker, fromPierIdx);
   }
}

void PierMaterialsComparison(rptParagraph* pPara, CComPtr<IBroker> pBroker, PierIndexType fromPierIdx)
{
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );

   ColumnIndexType nCols = 8;
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T("Pier Material Comparison"));
   *pPara << p_table<<rptNewLine;

   ColumnIndexType iCol = 0;
   (*p_table)(0,iCol++) << _T("Pier");
   (*p_table)(0,iCol++) << _T("Same") << rptNewLine <<  _T("as") << rptNewLine <<_T("From") << rptNewLine <<_T("Pier?");
   (*p_table)(0,iCol++) << _T("Model") << rptNewLine << _T("Type");
   (*p_table)(0,iCol++) << _T("Concrete") << rptNewLine << _T("Type");
   (*p_table)(0,iCol++) << COLHDR(RPT_FC,rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,iCol++) << COLHDR(_T("Density") << rptNewLine << _T("for") << rptNewLine << _T("Strength"),rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,iCol++) << COLHDR(_T("Density") << rptNewLine << _T("for") << rptNewLine << _T("Weight"),rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,iCol++) << COLHDR(_T("Max") << rptNewLine << _T("Aggregate") << rptNewLine << _T("Size"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   GET_IFACE2(pBroker,ILibrary, pLib );
   GET_IFACE2(pBroker,ISpecification, pSpec );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();

   // special considerations for f'ci if time step
   auto loss_method = prestress_loss_criteria.LossMethod;
   bool isTimeStep = loss_method == PrestressLossCriteria::LossMethodType::TIME_STEP;

   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   if ((nPiers - 1) < fromPierIdx)
   {
      ATLASSERT(false); // this should never happen
   }

   RowIndexType row = 1;
   const CPierData2* pFromPier = pBridgeDesc->GetPier(fromPierIdx);

   // use utility class to store data and for comparisons
   PierModelData fromPierModelData ( MakePierModelData(pFromPier) );

   bool isAnyPhysical(false);
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      iCol = 0;

      bool isFrom = pierIdx == fromPierIdx;
      if (isFrom)
      {
         ColorFromRow(p_table, row, nCols);
      }

      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

      bool bAbutment = pPier->IsAbutment();
      (*p_table)(row, iCol++) << LABEL_PIER_EX(bAbutment, pierIdx);

      PierModelData pierModelData ( MakePierModelData(pPier) );

      bool isEqual = fromPierModelData.ArePierMaterialsEqual(pierModelData);

      WriteCompareCell(p_table, row, iCol++, isFrom, isEqual);

      pgsTypes::PierModelType type = pPier->GetPierModelType();

      if (type == pgsTypes::pmtIdealized)
      {
         (*p_table)(row, iCol++) << _T("Idealized");
         if (isAnyPhysical)
         {
            p_table->SetColumnSpan(row, iCol, nCols-iCol);
            (*p_table)(row, iCol++) << _T(" ");
         }
      }
      else if (type == pgsTypes::pmtPhysical)
      {
         (*p_table)(row, iCol++) << _T("Physical Model");

         const CConcreteMaterial* pConcrete = &pierModelData.m_Concrete;

         pgsTypes::ConcreteType type = pConcrete->Type;
         std::_tstring  name = WBFL::LRFD::ConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)type, false);
         (*p_table)(row, iCol++) << name;

         (*p_table)(row, iCol++) << stress.SetValue(pConcrete->Fc);
         (*p_table)(row, iCol++) << density.SetValue(pConcrete->StrengthDensity);
         (*p_table)(row, iCol++) << density.SetValue(pConcrete->WeightDensity);
         (*p_table)(row, iCol++) << dim.SetValue(pConcrete->MaxAggregateSize);
      }

      row++;
   }
}

void write_connection_abbrevation_footnotes(rptParagraph * pPara)
{
   *pPara << Underline(Bold(_T("Legend:"))) << rptNewLine;
   *pPara << Bold(_T("Bearing Offset Measure")) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, true, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, true, false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, false, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, false, false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true, false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false, false) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << Bold(_T("End Distance Measure")) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder, true, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, true, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, false, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, true, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, false, false) << rptNewLine;
}

////////////////////////////////////////////////////////////////////////
/////// Classes for comparing pier data and transactions ///////////////
////////////////////////////////////////////////////////////////////////

PierConnectionData::PierConnectionData(PierCBLocType PierType, pgsTypes::BoundaryConditionType boundaryConditionType,
                                       Float64 backEndDist, ConnectionLibraryEntry::EndDistanceMeasurementType backEndDistMeasure,
                                       Float64 backGirderBearingOffset,  ConnectionLibraryEntry::BearingOffsetMeasurementType backBearingOffsetMeasurementType,
                                       Float64 aheadEndDist, ConnectionLibraryEntry::EndDistanceMeasurementType aheadEndDistMeasure,
                                       Float64 aheadGirderBearingOffset,  ConnectionLibraryEntry::BearingOffsetMeasurementType aheadBearingOffsetMeasurementType)
{
   m_PierLocType = PierType;
   m_BoundaryConditionType = boundaryConditionType;
   // use pgsTypes::PierFace to access arrays
   m_GirderEndDistance[pgsTypes::Back]            = backEndDist;
   m_EndDistanceMeasurementType[pgsTypes::Back]   = backEndDistMeasure;
   m_GirderBearingOffset[pgsTypes::Back]          = backGirderBearingOffset;
   m_BearingOffsetMeasurementType[pgsTypes::Back] = backBearingOffsetMeasurementType;

   m_GirderEndDistance[pgsTypes::Ahead] = aheadEndDist;
   m_EndDistanceMeasurementType[pgsTypes::Ahead] = aheadEndDistMeasure;
   m_GirderBearingOffset[pgsTypes::Ahead]          = aheadGirderBearingOffset;
   m_BearingOffsetMeasurementType[pgsTypes::Ahead] = aheadBearingOffsetMeasurementType;
}

bool PierConnectionData::AreConnectionsEqual(const PierConnectionData& rOther) const
{
   // we are boundary pier
   if (m_PierLocType == pcblLeftBoundaryPier && rOther.m_PierLocType == pcblLeftBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther);
   }
   else if (m_PierLocType == pcblLeftBoundaryPier && rOther.m_PierLocType == pcblInteriorPier)
   {
      return IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Back, rOther);
   }
   else if (m_PierLocType == pcblLeftBoundaryPier && rOther.m_PierLocType == pcblRightBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Ahead, pgsTypes::Back, rOther); // mirror
   }
   else if (m_PierLocType == pcblRightBoundaryPier && rOther.m_PierLocType == pcblRightBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther);
   }
   else if (m_PierLocType == pcblRightBoundaryPier && rOther.m_PierLocType == pcblInteriorPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther) && IsSideEqual(pgsTypes::Back, pgsTypes::Ahead, rOther);
   }
   else if (m_PierLocType == pcblRightBoundaryPier && rOther.m_PierLocType == pcblLeftBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Ahead, rOther); // mirror
   }
   // we are interior pier
   else if (m_PierLocType == pcblInteriorPier && rOther.m_PierLocType == pcblRightBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Back, rOther);
   }
   else if (m_PierLocType == pcblInteriorPier && rOther.m_PierLocType == pcblLeftBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Ahead, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther);
   }
   else if (m_PierLocType == pcblContinousSegment && rOther.m_PierLocType == pcblContinousSegment)
   {
      return true;
   }
   else if (m_PierLocType == pcblContinousSegment && rOther.m_PierLocType != pcblContinousSegment)
   {
      return false;
   }
   else if (rOther.m_PierLocType == pcblContinousSegment && m_PierLocType != pcblContinousSegment)
   {
      return false;
   }
   else
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther);
   }
}

bool PierConnectionData::IsSideEqual(pgsTypes::PierFaceType myFace,pgsTypes::PierFaceType otherFace, const PierConnectionData& rOther) const
{
   return ::IsEqual(m_GirderEndDistance[myFace],    rOther.m_GirderEndDistance[otherFace]) &&
            m_EndDistanceMeasurementType[myFace] ==   rOther.m_EndDistanceMeasurementType[otherFace] &&
            ::IsEqual(m_GirderBearingOffset[myFace],  rOther.m_GirderBearingOffset[otherFace]) &&
            m_BearingOffsetMeasurementType[myFace] == rOther.m_BearingOffsetMeasurementType[otherFace];
}

void PierConnectionData::CopyFace(pgsTypes::PierFaceType myFace, pgsTypes::PierFaceType otherFace, const PierConnectionData& rFromOther)
{
   m_GirderEndDistance[myFace]            = rFromOther.m_GirderEndDistance[otherFace];
   m_EndDistanceMeasurementType[myFace]   = rFromOther.m_EndDistanceMeasurementType[otherFace];
   m_GirderBearingOffset[myFace]          = rFromOther.m_GirderBearingOffset[otherFace];
   m_BearingOffsetMeasurementType[myFace] = rFromOther.m_BearingOffsetMeasurementType[otherFace];
}

/////////////////////

PierDiaphragmData::PierDiaphragmData(PierCBLocType PierType,
   Float64 backHeight, Float64 backWidth, ConnectionLibraryEntry::DiaphragmLoadType backLoadType, Float64 backLoadLocation,
   Float64 aheadHeight, Float64 aheadWidth, ConnectionLibraryEntry::DiaphragmLoadType aheadLoadType, Float64 aheadLoadLocation)
{
   m_PierLocType = PierType;
   // use pgsTypes::PierFace to access arrays
   m_Height[pgsTypes::Back]            = backHeight;
   m_Width[pgsTypes::Back]             = backWidth;
   m_LoadLocation[pgsTypes::Back]      = backLoadLocation;
   m_DiaphragmLoadType[pgsTypes::Back] = backLoadType;

   m_Height[pgsTypes::Ahead]            = aheadHeight;
   m_Width[pgsTypes::Ahead]             = aheadWidth;
   m_LoadLocation[pgsTypes::Ahead]      = aheadLoadLocation;
   m_DiaphragmLoadType[pgsTypes::Ahead] = aheadLoadType;
}

bool PierDiaphragmData::AreDiaphragmsEqual(const PierDiaphragmData& rOther) const
{
   // we are boundary pier
   if (m_PierLocType == pcblLeftBoundaryPier && rOther.m_PierLocType == pcblLeftBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther);
   }
   else if (m_PierLocType == pcblLeftBoundaryPier && rOther.m_PierLocType == pcblInteriorPier)
   {
      return IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Back, rOther);
   }
   else if (m_PierLocType == pcblLeftBoundaryPier && rOther.m_PierLocType == pcblRightBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Ahead, pgsTypes::Back, rOther); // mirror
   }
   else if (m_PierLocType == pcblRightBoundaryPier && rOther.m_PierLocType == pcblRightBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther);
   }
   else if (m_PierLocType == pcblRightBoundaryPier && rOther.m_PierLocType == pcblInteriorPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther) && IsSideEqual(pgsTypes::Back, pgsTypes::Ahead, rOther);
   }
   else if (m_PierLocType == pcblRightBoundaryPier && rOther.m_PierLocType == pcblLeftBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Ahead, rOther); // mirror
   }
   // we are interior pier
   else if (m_PierLocType == pcblInteriorPier && rOther.m_PierLocType == pcblRightBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Back, rOther);
   }
   else if (m_PierLocType == pcblInteriorPier && rOther.m_PierLocType == pcblLeftBoundaryPier)
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Ahead, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther);
   }
   else
   {
      return IsSideEqual(pgsTypes::Back, pgsTypes::Back, rOther) && IsSideEqual(pgsTypes::Ahead, pgsTypes::Ahead, rOther);
   }
}

bool PierDiaphragmData::IsSideEqual(pgsTypes::PierFaceType myFace,pgsTypes::PierFaceType otherFace, const PierDiaphragmData& rOther) const
{
   // Comparing is a bit complex
   bool isDimEqual = ::IsEqual(m_Height[myFace], rOther.m_Height[otherFace]) && ::IsEqual(m_Width[myFace], rOther.m_Width[otherFace]);

   if (!isDimEqual)
   {
      return false;
   }
   else
   {
      if (m_DiaphragmLoadType[myFace] != rOther.m_DiaphragmLoadType[otherFace])
      {
         return false;
      }
      else if (m_DiaphragmLoadType[myFace] == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
      {
         return ::IsEqual(m_LoadLocation[myFace], rOther.m_LoadLocation[otherFace]);
      }
      else
      {
         return true;
      }
   }
}

void PierDiaphragmData::CopyFace(pgsTypes::PierFaceType myFace, pgsTypes::PierFaceType otherFace, const PierDiaphragmData& rFromOther)
{
   m_Height[myFace]            = rFromOther.m_Height[otherFace];
   m_Width[myFace]              = rFromOther.m_Width[otherFace];
   m_DiaphragmLoadType[myFace]  = rFromOther.m_DiaphragmLoadType[otherFace];
   m_LoadLocation[myFace]       = rFromOther.m_LoadLocation[otherFace];
}

/////////////////////

PierModelData::PierModelData(const CConcreteMaterial & rConcrete, ColumnIndexType refColumnIdx, Float64 transverseOffset, pgsTypes::OffsetMeasurementType transverseOffsetMeasurement, 
   Float64 leftXBeamHeight, Float64 leftXBeamTaperHeight, Float64 leftXBeamTaperLength, Float64 leftXBeamEndSlopeOffset, Float64 leftXBeamOverhang, 
   Float64 rightXBeamHeight, Float64 rightXBeamTaperHeight, Float64 rightXBeamTaperLength, Float64 rightXBeamEndSlopeOffset, Float64 rightXBeamOverhang,
   Float64 xBeamWidth, CColumnData::ColumnHeightMeasurementType columnHeightType, pgsTypes::ColumnLongitudinalBaseFixityType columnFixity, const CColumnData & rColumnData):
         m_PierModelType(pgsTypes::pmtPhysical),
         m_Concrete(rConcrete),
         m_RefColumnIdx(refColumnIdx),
         m_TransverseOffset(transverseOffset),
         m_TransverseOffsetMeasurement(transverseOffsetMeasurement),
         m_XBeamHeight{ {leftXBeamHeight,rightXBeamHeight} },
         m_XBeamTaperHeight{ {leftXBeamTaperHeight, rightXBeamTaperHeight} },
         m_XBeamTaperLength{ {leftXBeamTaperLength, rightXBeamTaperLength} },
         m_XBeamEndSlopeOffset{ {leftXBeamEndSlopeOffset, rightXBeamEndSlopeOffset} },
         m_XBeamOverhang{ {leftXBeamOverhang, rightXBeamOverhang} },
         m_XBeamWidth(xBeamWidth),
         m_ColumnHeightType(columnHeightType),
         m_ColumnFixity(columnFixity)
{
   m_Columns.push_back(rColumnData);
}

void PierModelData::AddColumn(Float64 spacing, const CColumnData & rColumnData)
{
   m_ColumnSpacing.push_back(spacing);
   m_Columns.push_back(rColumnData);
}

template<class T>
bool IsEqual2(const T& t1, const T& tother)
{
   return ::IsEqual(t1[0], tother[0]) && ::IsEqual(t1[1], tother[1]);
}

bool PierModelData::AreModelsEqual(const PierModelData& rOther) const
{
   return ArePierMaterialsEqual(rOther) && ArePierCapDimensionsEqual(rOther) && AreColumnLayoutsEqual(rOther);
}

bool PierModelData::ArePierMaterialsEqual(const PierModelData& rOther) const
{
   if (m_PierModelType == pgsTypes::pmtIdealized && rOther.m_PierModelType == pgsTypes::pmtIdealized)
   {
      return true;
   }
   else if (m_PierModelType != rOther.m_PierModelType)
   {
      return false; // ideal and physical don't match
   }
   else
   {
      return
      m_Concrete == rOther.m_Concrete;
   }
}

bool PierModelData::ArePierCapDimensionsEqual(const PierModelData& rOther) const
{

   if (m_PierModelType == pgsTypes::pmtIdealized && rOther.m_PierModelType == pgsTypes::pmtIdealized)
   {
      return true;
   }
   else if (m_PierModelType != rOther.m_PierModelType)
   {
      return false; // ideal and physical don't match
   }
   else
   { 
      return
         IsEqual2(m_XBeamHeight, rOther.m_XBeamHeight) &&
         IsEqual2(m_XBeamTaperHeight, rOther.m_XBeamTaperHeight) &&
         IsEqual2(m_XBeamTaperLength, rOther.m_XBeamTaperLength) &&
         IsEqual2(m_XBeamEndSlopeOffset, rOther.m_XBeamEndSlopeOffset) &&
         IsEqual2(m_XBeamOverhang, rOther.m_XBeamOverhang) &&
         IsEqual2(m_XBeamHeight, rOther.m_XBeamHeight);
   }
}

bool PierModelData::AreColumnLayoutsEqual(const PierModelData& rOther) const
{

   if (m_PierModelType == pgsTypes::pmtIdealized && rOther.m_PierModelType == pgsTypes::pmtIdealized)
   {
      return true;
   }
   else if (m_PierModelType != rOther.m_PierModelType)
   {
      return false; // ideal and physical don't match
   }
   else
   { 
      return
         IsEqual(m_TransverseOffset, rOther.m_TransverseOffset) &&
         m_TransverseOffsetMeasurement == rOther.m_TransverseOffsetMeasurement &&
         m_ColumnHeightType == rOther.m_ColumnHeightType &&
         m_ColumnFixity == rOther.m_ColumnFixity &&
         m_Columns == rOther.m_Columns;
   }
}

bool PierAllData::ArePiersEqual(const PierAllData & rOther) const
{
   // We are equal if all component parts are equal
   return m_PierModelData.AreModelsEqual(rOther.m_PierModelData) &&
          m_PierDiaphragmData.AreDiaphragmsEqual(rOther.m_PierDiaphragmData) &&
          m_PierConnectionData.AreConnectionsEqual(rOther.m_PierConnectionData);
}
