///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "TransferLengthEngineer.h"

#include <PgsExt\GirderMaterial.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\PrecastSegmentData.h>

#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsTransferLengthEngineer::pgsTransferLengthEngineer() 
{
   m_pBroker = nullptr;
}

pgsTransferLengthEngineer::~pgsTransferLengthEngineer()
{
}

void pgsTransferLengthEngineer::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsTransferLengthEngineer::Invalidate()
{
   for (auto& cache : m_MinCache)
      cache.clear();

   for (auto& cache : m_MaxCache)
      cache.clear();
}

std::shared_ptr<pgsTransferLength> pgsTransferLengthEngineer::GetTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType,const GDRCONFIG* pConfig) const
{
   if (pConfig == nullptr)
   {
      auto found = (xferType == pgsTypes::tltMinimum ? m_MinCache[strandType].find(segmentKey) : m_MaxCache[strandType].find(segmentKey));
      if (found != (xferType == pgsTypes::tltMinimum ? m_MinCache[strandType].end() : m_MaxCache[strandType].end())) return found->second;
   }

   GET_IFACE(ISpecification, pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());

   std::shared_ptr<pgsTransferLength> pTransferLength;

   auto prestressTransferComputationType = pSpecEntry->GetPrestressTransferComputationType();

   if (prestressTransferComputationType == pgsTypes::ptMinuteValue)
   {
      // Model zero prestress transfer length. 0.1 inches seems to give
      // good designs and spec checks. This does not happen if the value is reduced to 0.0;
      // because pgsuper is not set up to deal with moment point loads.
      //
      pTransferLength = std::make_shared<pgsMinuteTransferLength>();
   }
   else
   {
      ATLASSERT(prestressTransferComputationType == pgsTypes::ptUsingSpecification);

      GET_IFACE(ISegmentData, pSegmentData);
      const auto* pStrand = pSegmentData->GetStrandMaterial(segmentKey, strandType);
      ATLASSERT(pStrand != nullptr);

      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      if (pMaterial->Concrete.Type == pgsTypes::PCI_UHPC)
      {
         pTransferLength = std::make_shared<pgsPCIUHPCTransferLength>(pStrand->GetNominalDiameter());
      }
      else if (pMaterial->Concrete.Type == pgsTypes::FHWA_UHPC)
      {
         pTransferLength = std::make_shared<pgsFHWAUHPCTransferLength>(pStrand->GetNominalDiameter(),xferType);
      }
      else
      {
         pTransferLength = std::make_shared<pgsLRFDTransferLength>(pStrand->GetNominalDiameter(), pStrand->GetCoating());
      }
   }

   if (pConfig == nullptr)
   {
      xferType == pgsTypes::tltMinimum ? m_MinCache[strandType].insert(std::make_pair(segmentKey, pTransferLength)) : m_MaxCache[strandType].insert(std::make_pair(segmentKey, pTransferLength));
   }
   return pTransferLength;
}

Float64 pgsTransferLengthEngineer::GetTransferLength(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig) const
{
   std::shared_ptr<pgsTransferLength> pTransferLength = GetTransferLengthDetails(segmentKey, strandType, xferType, pConfig);
   return pTransferLength->GetTransferLength();
}

Float64 pgsTransferLengthEngineer::GetTransferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig) const
{
   ATLASSERT(strandType != pgsTypes::Permanent); // there isn't a composite adjustment

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   // Quick check to make sure there is even an adjustment to be made. If there are no strands, just leave
   GET_IFACE(IStrandGeometry, pStrandGeom);
   auto strand_count = pStrandGeom->GetStrandCount(poi, releaseIntervalIdx, strandType, pConfig);
   if (strand_count.first == 0)
   {
      // no strands
      return 1.0;
   }

   // Compute a scaling factor to apply to the basic prestress force to adjust for transfer length/ and debonded strands
   Float64 xfer_length = GetTransferLength(segmentKey, strandType, xferType);

   GET_IFACE(IBridge, pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 Xpoi_from_left_end = poi.GetDistFromStart();
   Float64 Xpoi_from_right_end = segment_length - Xpoi_from_left_end;

   // Determine effectiveness of partially bonded strands
   Float64 nDebondedEffective = 0; // number of strands that are bonding over the transfer length times the transfer factor

   if (pConfig)
   {
      for (const auto& debond_info : pConfig->PrestressConfig.Debond[strandType])
      {
         Float64 Xleft_bond = debond_info.DebondLength[pgsTypes::metStart];
         Float64 Xright_bond = segment_length - debond_info.DebondLength[pgsTypes::metEnd];

         if (Xleft_bond < Xpoi_from_left_end && Xpoi_from_left_end < Xright_bond)
         {
            // poi is at a section where the strands are bonded (not in a debond region)

            // see if the poi is in a transfer length region
            Float64 left_distance = Xpoi_from_left_end - Xleft_bond; // distance from start of bonding on left end to the poi
            Float64 right_distance = Xright_bond - Xpoi_from_left_end; // distance from start of bonding on right end to the poi

            if (left_distance < xfer_length)
            {
               // poi is in the transfer length region on the left end
               nDebondedEffective += left_distance / xfer_length;
            }
            else if (right_distance < xfer_length)
            {
               // poi is in the transfer length region on the right end
               nDebondedEffective += right_distance / xfer_length;
            }
            else
            {
               // poi is in a region where the strand is fully bonded
               nDebondedEffective += 1.0;
            }
         }
      }
   }
   else
   {
      GET_IFACE(ISegmentData, pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

      if (pStrands->GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
      {
         const auto& strandRows = pStrands->GetStrandRows();
         for (const auto& strandRow : strandRows)
         {
            if (strandRow.m_StrandType != strandType)
            {
               continue;
            }

            if (strandRow.m_bIsDebonded[pgsTypes::metStart] || strandRow.m_bIsDebonded[pgsTypes::metEnd])
            {
               Float64 Xleft_bond = strandRow.m_bIsDebonded[pgsTypes::metStart] ? strandRow.m_DebondLength[pgsTypes::metStart] : 0;
               Float64 Xright_bond = segment_length - (strandRow.m_bIsDebonded[pgsTypes::metEnd] ? strandRow.m_DebondLength[pgsTypes::metEnd] : 0.0);

               if (Xleft_bond < Xpoi_from_left_end && Xpoi_from_left_end < Xright_bond)
               {
                  // poi is at a section where the strands are bonded (not in a debond region)

                  // see if the poi is in a transfer length region
                  Float64 left_distance = Xpoi_from_left_end - Xleft_bond; // distance from start of bonding on left end to the poi
                  Float64 right_distance = Xright_bond - Xpoi_from_left_end; // distance from start of bonding on right end to the poi

                  if (left_distance < xfer_length)
                  {
                     // poi is in the transfer length region on the left end
                     nDebondedEffective += (left_distance / xfer_length) * strandRow.m_nStrands;
                  }
                  else if (right_distance < xfer_length)
                  {
                     // poi is in the transfer length region on the right end
                     nDebondedEffective += (right_distance / xfer_length) * strandRow.m_nStrands;
                  }
                  else
                  {
                     // poi is in a region where the strand is fully bonded
                     nDebondedEffective += strandRow.m_nStrands;
                  }
               }
            }
         }
      }
      else
      {
         const auto& debonding = pStrands->GetDebonding(strandType);
         for (const auto& debond_data : debonding)
         {
            Float64 Xleft_bond = debond_data.Length[pgsTypes::metStart];
            Float64 Xright_bond = segment_length - debond_data.Length[pgsTypes::metEnd];

            if (Xleft_bond < Xpoi_from_left_end && Xpoi_from_left_end < Xright_bond)
            {
               // poi is at a section where the strands are bonded (not in a debond region)

               // see if the poi is in a transfer length region
               Float64 left_distance = Xpoi_from_left_end - Xleft_bond; // distance from start of bonding on left end to the poi
               Float64 right_distance = Xright_bond - Xpoi_from_left_end; // distance from start of bonding on right end to the poi

               StrandIndexType strandIdx1, strandIdx2;
               pStrandGeom->GridPositionToStrandPosition(segmentKey, strandType, debond_data.strandTypeGridIdx, &strandIdx1, &strandIdx2);
               ATLASSERT(strandIdx1 != INVALID_INDEX);
               StrandIndexType nStrands = 1;
               if (strandIdx2 != INVALID_INDEX)
               {
                  nStrands++;
               }

               if (left_distance < xfer_length)
               {
                  // poi is in the transfer length region on the left end
                  nDebondedEffective += (left_distance / xfer_length) * nStrands;
               }
               else if (right_distance < xfer_length)
               {
                  // poi is in the transfer length region on the right end
                  nDebondedEffective += (right_distance / xfer_length) * nStrands;
               }
               else
               {
                  // poi is in a region where the strand is fully bonded
                  nDebondedEffective += nStrands;
               }
            }
         }
      }
   }

   // Determine effectiveness of bonded strands
   Float64 nBondedEffective = 0; // number of effective bonded strands
   if (InRange(0.0, Xpoi_from_left_end, xfer_length))
   {
      // from the left end of the girder, POI is in transfer zone
      nBondedEffective = Xpoi_from_left_end / xfer_length;
   }
   else if (InRange(0.0, Xpoi_from_right_end, xfer_length))
   {
      // from the right end of the girder, POI is in transfer zone
      nBondedEffective = Xpoi_from_right_end / xfer_length;
   }
   else
   {
      // strand is fully bonded at the location of the POI
      nBondedEffective = 1.0;
   }

   // nBondedEffective is for 1 strand... make it for all the bonded strands
   // number of bonded strands is total number at section - number debonded at section
   nBondedEffective *= (strand_count.first - strand_count.second);

   Float64 adjust = (nBondedEffective + nDebondedEffective) / strand_count.first;

   return adjust;
}

Float64 pgsTransferLengthEngineer::GetTransferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, StrandIndexType strandIdx, const GDRCONFIG* pConfig) const
{
   ATLASSERT(strandType != pgsTypes::Permanent); // there isn't a composite adjustment
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 xfer_length = GetTransferLength(segmentKey, strandType, xferType);

   GET_IFACE(IBridge, pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 Xpoi_from_left_end = Max(poi.GetDistFromStart(), 0.0); // can be negative if POI is before start of segment (like in a closure joint or pier diaphragm)
   Float64 Xpoi_from_right_end = Max(segment_length - Xpoi_from_left_end, 0.0); // can be negative if POI is beyond end of segment (like in a closure joint or pier diaphragm)

   Float64 adjust = 1.0;

   GET_IFACE(ISegmentData, pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   if (pStrands->GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
   {
      StrandIndexType startStrandIdx = 0;
      const auto& strandRows = pStrands->GetStrandRows();
      for (const CStrandRow& strandRow : strandRows)
      {
         if (strandRow.m_StrandType != strandType)
         {
            continue;
         }

         StrandIndexType endStrandIdx = startStrandIdx + strandRow.m_nStrands;
         if (startStrandIdx <= strandIdx && strandIdx <= endStrandIdx)
         {
            if (strandRow.m_bIsDebonded[pgsTypes::metStart] || strandRow.m_bIsDebonded[pgsTypes::metEnd])
            {
               Float64 Xleft_bond = strandRow.m_bIsDebonded[pgsTypes::metStart] ? strandRow.m_DebondLength[pgsTypes::metStart] : 0;
               Float64 Xright_bond = segment_length - strandRow.m_bIsDebonded[pgsTypes::metEnd] ? strandRow.m_DebondLength[pgsTypes::metEnd] : 0;

               if (Xleft_bond < Xpoi_from_left_end && Xpoi_from_left_end < Xright_bond)
               {
                  // poi is at a section where the strands are bonded (not in a debond region)

                  // see if the poi is in a transfer length region
                  Float64 left_distance = Xpoi_from_left_end - Xleft_bond; // distance from start of bonding on left end to the poi
                  Float64 right_distance = Xright_bond - Xpoi_from_left_end; // distance from start of bonding on right end to the poi

                  if (left_distance < xfer_length)
                  {
                     // poi is in the transfer length region on the left end
                     adjust = left_distance / xfer_length;
                  }
                  else if (right_distance < xfer_length)
                  {
                     // poi is in the transfer length region on the right end
                     adjust = right_distance / xfer_length;
                  }
                  else
                  {
                     // poi is in a region where the strand is fully bonded
                     adjust = 1.0;
                  }
               }
               else
               {
                  adjust = 0.0; // the POI isn't in a location where bond has started yet
               }
            }
            else
            {
               // this strand doesn't have any debonding
               if (Xpoi_from_left_end < xfer_length)
               {
                  // poi is in the transfer length at the left end
                  adjust = Xpoi_from_left_end / xfer_length;
               }
               else if (Xpoi_from_right_end < xfer_length)
               {
                  // poi is in the transfer length at the right end
                  adjust = Xpoi_from_right_end / xfer_length;
               }
               else
               {
                  // poi is in the fully transfered region
                  adjust = 1.0;
               }
            }
            break; // we found our strand, no need to continue looping
         }
         startStrandIdx = endStrandIdx;
      }
   }
   else
   {
      GET_IFACE(IStrandGeometry, pStrandGeom);
      Float64 debond_left, debond_right;
      if (pStrandGeom->IsStrandDebonded(segmentKey, strandIdx, strandType, pConfig, &debond_left, &debond_right))
      {
         // this is the debonding information for the strand we are interested in
         if (debond_left < Xpoi_from_left_end && Xpoi_from_left_end < debond_right)
         {
            // poi is at a section where the strands are bonded (not in a debond region)

            Float64 left_distance = Xpoi_from_left_end - debond_left; // distance from start of bonding on left end to the poi
            Float64 right_distance = debond_right - Xpoi_from_left_end; // distance from start of bonding on right end to the poi

            if (left_distance < xfer_length)
            {
               adjust = left_distance / xfer_length;
            }
            else if (right_distance < xfer_length)
            {
               adjust = right_distance / xfer_length;
            }
            else
            {
               adjust = 1.0;
            }
         }
         else
         {
            // bonding for the subject strand has not started at this section (eg, the strand is debonded here so there is no force transfer)
            adjust = 0.0;
         }
      }
      else
      {
         // this strand doesn't have any debonding
         if (Xpoi_from_left_end < xfer_length)
         {
            // poi is in the transfer length at the left end
            adjust = Xpoi_from_left_end / xfer_length;
         }
         else if (Xpoi_from_right_end < xfer_length)
         {
            // poi is in the transfer length at the right end
            adjust = Xpoi_from_right_end / xfer_length;
         }
         else
         {
            // poi is in the fully transfered region
            adjust = 1.0;
         }
      }
   }

   return adjust;
}

void pgsTransferLengthEngineer::ReportTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::TransferLengthType xferType, rptChapter* pChapter) const
{
   GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   pgsTypes::AdjustableStrandType adj_type = pSegment->Strands.GetAdjustableStrandType();
   std::_tstring strAdj(pgsTypes::asHarped == adj_type ? _T("Harped") : _T("Adj. Straight"));

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   auto pTransferLength = GetTransferLengthDetails(segmentKey, pgsTypes::Straight, xferType);
   auto pTransferLengthBase = std::dynamic_pointer_cast<pgsTransferLengthBase>(pTransferLength);

   *pPara << pTransferLengthBase->GetTransferLengthType(xferType) << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   
   pTransferLengthBase->ReportTransferLengthSpecReference(pPara); 

   (*pPara) << Bold(_T("Straight Strands")) << rptNewLine;
   pTransferLengthBase->ReportDetails(pChapter, pDisplayUnits);

   pPara = new rptParagraph;
   *pChapter << pPara;

   (*pPara) << bold(ON) << strAdj << _T(" Strands") << bold(OFF) << rptNewLine;

   pTransferLength = GetTransferLengthDetails(segmentKey, pgsTypes::Harped, xferType);
   pTransferLengthBase = std::dynamic_pointer_cast<pgsTransferLengthBase>(pTransferLength);
   pTransferLengthBase->ReportDetails(pChapter, pDisplayUnits);
}

////////////////
Float64 pgsMinuteTransferLength::GetTransferLength() const
{
   return WBFL::Units::ConvertToSysUnits(0.1, WBFL::Units::Measure::Inch);
}

void pgsMinuteTransferLength::ReportDetails(rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true);
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;
   (*pPara) << _T("Zero Transfer Length Selected in Project Criteria") << rptNewLine;
   (*pPara) << _T("Actual length used ") << Sub2(_T("l"), _T("t")) << _T(" = ") << length.SetValue(GetTransferLength()) << rptNewLine;
}

////////////////////////
pgsLRFDTransferLength::pgsLRFDTransferLength() :
   m_Coating(WBFL::Materials::PsStrand::Coating::None),
   m_db(0.0)
{
}
pgsLRFDTransferLength::pgsLRFDTransferLength(Float64 db,WBFL::Materials::PsStrand::Coating coating) :
   m_db(db), m_Coating(coating)
{
}

void pgsLRFDTransferLength::SetStrandDiameter(Float64 db)
{
   m_db = db;
}

Float64 pgsLRFDTransferLength::GetStrandDiameter() const
{
   return m_db;
}

void pgsLRFDTransferLength::SetCoating(WBFL::Materials::PsStrand::Coating coating)
{
   m_Coating = coating;
}

WBFL::Materials::PsStrand::Coating pgsLRFDTransferLength::GetCoating() const
{
   return m_Coating;
}

Float64 pgsLRFDTransferLength::GetTransferLength() const
{
   return (m_Coating == WBFL::Materials::PsStrand::Coating::None ? 60 : 50) * m_db;
}

void pgsLRFDTransferLength::ReportDetails(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true);

   (*pPara) << Sub2(_T("l"), _T("t")) << _T(" = ") << (m_Coating == WBFL::Materials::PsStrand::Coating::None ? _T("60") : _T("50")) << Sub2(_T("d"), _T("b")) << _T(" = ") << (m_Coating == WBFL::Materials::PsStrand::Coating::None ? _T("60") : _T("50"));
   (*pPara) << _T("(") << length.SetValue(m_db) << _T(") = ");
   (*pPara) << length.SetValue(GetTransferLength()) << rptNewLine;
}

void pgsLRFDTransferLength::ReportTransferLengthSpecReference(rptParagraph* pPara) const
{
   (*pPara) << _T("AASHTO LRFD BDS ") << LrfdCw8th(_T("5.11.4.1"), _T("5.9.4.3.1")) << rptNewLine;
   (*pPara) << _T("See also \"Guidelines for the use of Epoxy-Coated Strand\", Section 5.5.2, PCI Journal, July-August 1993") << rptNewLine;
}

////////////////////////
////////////////////////
////////////////////////

pgsPCIUHPCTransferLength::pgsPCIUHPCTransferLength()
{
}

pgsPCIUHPCTransferLength::pgsPCIUHPCTransferLength(Float64 db) :
   m_db(db)
{
}

void pgsPCIUHPCTransferLength::SetStrandDiameter(Float64 db)
{
   m_db = db;
}

Float64 pgsPCIUHPCTransferLength::GetStrandDiameter() const
{
   return m_db;
}

Float64 pgsPCIUHPCTransferLength::GetTransferLength() const
{
   return 20.0 * m_db;
}

void pgsPCIUHPCTransferLength::ReportDetails(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true);

   (*pPara) << Sub2(_T("l"), _T("t")) << _T(" = ") << _T("20") << Sub2(_T("d"), _T("b")) << _T(" = 20(") << length.SetValue(m_db) << _T(") = ");
   (*pPara) << length.SetValue(GetTransferLength()) << rptNewLine;
}

void pgsPCIUHPCTransferLength::ReportTransferLengthSpecReference(rptParagraph* pPara) const
{
   *pPara << _T("PCI UHPC SDG E.9.3.2.1") << rptNewLine;
}

////////////////////////
////////////////////////
////////////////////////

pgsFHWAUHPCTransferLength::pgsFHWAUHPCTransferLength()
{
}

pgsFHWAUHPCTransferLength::pgsFHWAUHPCTransferLength(Float64 db, pgsTypes::TransferLengthType xferType) :
   m_db(db), m_XferType(xferType)
{
}

void pgsFHWAUHPCTransferLength::SetStrandDiameter(Float64 db)
{
   m_db = db;
}

Float64 pgsFHWAUHPCTransferLength::GetStrandDiameter() const
{
   return m_db;
}

void pgsFHWAUHPCTransferLength::SetTransferLengthType(pgsTypes::TransferLengthType xferType)
{
   m_XferType = xferType;
}

pgsTypes::TransferLengthType pgsFHWAUHPCTransferLength::GetTransferLengthType() const
{
   return m_XferType;
}

Float64 pgsFHWAUHPCTransferLength::GetTransferLength() const
{
   Float64 xi = GetTransferLengthFactor();
   Float64 lt = xi * 24.0 * m_db;
   return lt;
}

void pgsFHWAUHPCTransferLength::ReportDetails(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true);

   Float64 xi = GetTransferLengthFactor();

   (*pPara) << Sub2(_T("l"), _T("t")) << _T(" = ") << symbol(xi) << _T("24") << Sub2(_T("d"), _T("b")) << _T(" = (") << xi << _T(")(24.0)(") << length.SetValue(m_db) << _T(") = ");
   (*pPara) << length.SetValue(GetTransferLength()) << rptNewLine;
}

void pgsFHWAUHPCTransferLength::ReportTransferLengthSpecReference(rptParagraph* pPara) const
{
   *pPara << _T("GS 1.9.4.3.1") << rptNewLine;
}

Float64 pgsFHWAUHPCTransferLength::GetTransferLengthFactor() const
{
   Float64 xi = m_XferType == pgsTypes::tltMinimum ? 0.75 : 1.0;
   return xi;
}
