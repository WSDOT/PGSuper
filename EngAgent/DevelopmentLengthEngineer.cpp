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

#include "stdafx.h"
#include "DevelopmentLengthEngineer.h"

#include <IFace\Intervals.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\PrecastSegmentData.h>

#include <Reporting\ReportNotes.h>


pgsDevelopmentLengthEngineer::pgsDevelopmentLengthEngineer()
{
   m_pBroker = nullptr;
}

pgsDevelopmentLengthEngineer::~pgsDevelopmentLengthEngineer()
{
}

void pgsDevelopmentLengthEngineer::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsDevelopmentLengthEngineer::Invalidate()
{
   for (auto& cache : m_BondedCache)
      cache.clear();

   for (auto& cache : m_DebondCache)
      cache.clear();
}

const std::shared_ptr<pgsDevelopmentLength> pgsDevelopmentLengthEngineer::GetDevelopmentLengthDetails(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig) const
{
   if (pConfig == nullptr)
   {
      const Cache* pCache = (bDebonded ? &m_DebondCache : &m_BondedCache);
      auto found = (*pCache)[strandType].find(poi);
      if (found != (*pCache)[strandType].end()) return found->second;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType intervalIdx = nIntervals - 1;

   // NOTE: The fpe we want must account for transfer length effects
   // The fpe returned from the IPretensionForce interface is the basic value so we compute it here as P/A
   // since P is adjusted for transfer effects
   GET_IFACE(IPretensionForce, pPrestressForce);
   Float64 Ppe = pPrestressForce->GetPrestressForce(poi, strandType, intervalIdx, pgsTypes::End, pgsTypes::TransferLengthType::Maximum, pConfig);
   // NOTE: development length is related to strength limit states so we want to use the maximum transfer length.

   GET_IFACE(IStrandGeometry, pStrandGeom);
   Float64 Aps = pStrandGeom->GetStrandArea(poi, intervalIdx, strandType, pConfig);

   Float64 fpe = IsZero(Aps) ? 0 : Ppe / Aps;

   GET_IFACE(IMomentCapacity, pMomCap);
   const MOMENTCAPACITYDETAILS* pmcd = pMomCap->GetMomentCapacityDetails(intervalIdx, poi, true/*positive moment*/, pConfig);
   Float64 fps = IsZero(Aps) ? 0 : pmcd->fps_avg; // fps_avg is for permanent strands... if either Straight or Harped is zero, then this should be zero for that strand type

   const std::shared_ptr<pgsDevelopmentLength> details = GetDevelopmentLengthDetails(poi, strandType, bDebonded, fps, fpe, pConfig);

   if (pConfig == nullptr)
   {
      Cache* pCache = (bDebonded ? &m_DebondCache : &m_BondedCache);
      (*pCache)[strandType].insert(std::make_pair(poi, details));
   }

   return details;
}

const std::shared_ptr<pgsDevelopmentLength> pgsDevelopmentLengthEngineer::GetDevelopmentLengthDetails(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, Float64 fps, Float64 fpe, const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IMaterials, pMaterials);
   const auto* pStrand = pMaterials->GetStrandMaterial(segmentKey, strandType);

   std::shared_ptr<pgsDevelopmentLength> details;
   Float64 db = pStrand->GetNominalDiameter();

   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      details = std::make_shared<pgsPCIUHPCDevelopmentLength>(db, fpe, fps);
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      GET_IFACE(IPretensionForce, pPrestress);
      Float64 lt = pPrestress->GetTransferLength(segmentKey, strandType, pgsTypes::TransferLengthType::Maximum, pConfig);
      details = std::make_shared<pgsUHPCDevelopmentLength>(lt, db, fpe, fps);
   }
   else
   {
      GET_IFACE(IGirder, pGirder);
      Float64 mbrDepth = pGirder->GetHeight(poi);
      details = std::make_shared<pgsLRFDDevelopmentLength>(db, fpe, fps, mbrDepth, bDebonded);
   }

   return details;
}

Float64 pgsDevelopmentLengthEngineer::GetDevelopmentLength(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig) const
{
   auto details = GetDevelopmentLengthDetails(poi, strandType, bDebonded, pConfig);
   return details->GetDevelopmentLength();
}

Float64 pgsDevelopmentLengthEngineer::GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi, StrandIndexType strandIdx, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig) const
{
   const std::shared_ptr<pgsDevelopmentLength> pDevLength = GetDevelopmentLengthDetails(poi, strandType, bDebonded, pConfig);
   Float64 fps = pDevLength->GetFps();
   Float64 fpe = pDevLength->GetFpe();

   return GetDevelopmentLengthAdjustment(poi, strandIdx, strandType, fps, fpe, pConfig);
}

Float64 pgsDevelopmentLengthEngineer::GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi, StrandIndexType strandIdx, pgsTypes::StrandType strandType, Float64 fps, Float64 fpe, const GDRCONFIG* pConfig) const
{
   // Compute a scaling factor to apply to the basic prestress force to
   // adjust for prestress force in the development
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Xpoi = poi.GetDistFromStart();

   GET_IFACE(IStrandGeometry, pStrandGeom);
   Float64 bond_start, bond_end;
   bool bDebonded = pStrandGeom->IsStrandDebonded(segmentKey, strandIdx, strandType, pConfig, &bond_start, &bond_end);
   bool bExtendedStrand = pStrandGeom->IsExtendedStrand(poi, strandIdx, strandType, pConfig);

   // determine minimum bonded length from poi
   Float64 left_bonded_length, right_bonded_length;
   if (bDebonded)
   {
      // measure bonded length
      left_bonded_length = Xpoi - bond_start;
      right_bonded_length = bond_end - Xpoi;
   }
   else if (bExtendedStrand)
   {
      // strand is extended into end diaphragm... the development length adjustment is 1.0
      return 1.0;
   }
   else
   {
      // no debonding, bond length is to ends of girder
      GET_IFACE(IBridge, pBridge);
      Float64 gdr_length = pBridge->GetSegmentLength(segmentKey);

      left_bonded_length = Xpoi;
      right_bonded_length = gdr_length - Xpoi;
   }

   Float64 lpx = Min(left_bonded_length, right_bonded_length);

   if (lpx <= 0.0)
   {
      // strand is unbonded at location, no more to do
      return 0.0;
   }
   else
   {
      GET_IFACE(IPretensionForce, pPrestressForce);
      const std::shared_ptr<pgsTransferLength> pXferLength = pPrestressForce->GetTransferLengthDetails(poi.GetSegmentKey(), strandType, pgsTypes::TransferLengthType::Maximum,pConfig);
      const std::shared_ptr<pgsDevelopmentLength> pDevLength = GetDevelopmentLengthDetails(poi, strandType, bDebonded, fps, fpe, pConfig);
      Float64 xfer_length = pXferLength->GetTransferLength();
      Float64 dev_length = pDevLength->GetDevelopmentLength();

      Float64 adjust = -999; // dummy value, helps with debugging

      if (IsLE(lpx, xfer_length))
      {
         adjust = (IsLE(fpe, fps) ? (lpx * fpe) / (xfer_length * fps) : 1.0);
      }
      else if (IsLE(lpx, dev_length))
      {
         adjust = (fpe + (lpx - xfer_length) * (fps - fpe) / (dev_length - xfer_length)) / fps;
      }
      else
      {
         adjust = 1.0;
      }

      adjust = IsZero(adjust) ? 0 : adjust;
      adjust = ::ForceIntoRange(0.0, adjust, 1.0);
      return adjust;
   }
}

void pgsDevelopmentLengthEngineer::ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const
{
   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      pgsPCIUHPCDevelopmentLengthReporter reporter(m_pBroker, this);
      reporter.ReportDevelopmentLengthDetails(segmentKey, pChapter);
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      pgsUHPCDevelopmentLengthReporter reporter(m_pBroker, this);
      reporter.ReportDevelopmentLengthDetails(segmentKey, pChapter);
   }
   else
   {
      pgsLRFDDevelopmentLengthReporter reporter(m_pBroker, this);
      reporter.ReportDevelopmentLengthDetails(segmentKey, pChapter);
   }
}



////////////////////////////////

pgsDevelopmentLengthBase::pgsDevelopmentLengthBase(Float64 db, Float64 fpe, Float64 fps) :
   pgsDevelopmentLength(fpe,fps), m_db(db)
{
}

void pgsDevelopmentLengthBase::SetStrandDiameter(Float64 db)
{
   m_db = db;
}

Float64 pgsDevelopmentLengthBase::GetStrandDiameter() const
{
   return m_db;
}

/////////////////////////////
/////////////////////////////
/////////////////////////////

pgsLRFDDevelopmentLength::pgsLRFDDevelopmentLength(Float64 db, Float64 fpe, Float64 fps, Float64 mbrDepth, bool bDebonded) :
   pgsDevelopmentLengthBase(db, fpe, fps), m_MbrDepth(mbrDepth), m_bDebonded(bDebonded)
{
}

Float64 pgsLRFDDevelopmentLength::GetDevelopmentLengthFactor() const
{
   return WBFL::LRFD::PsStrand::GetDevLengthFactor(m_MbrDepth, m_bDebonded);
}

Float64 pgsLRFDDevelopmentLength::GetDevelopmentLength() const
{
   return WBFL::LRFD::PsStrand::GetDevLength(m_db, m_fps, m_fpe, m_MbrDepth, m_bDebonded);
}

////////////////////////
////////////////////////
////////////////////////

pgsPCIUHPCDevelopmentLength::pgsPCIUHPCDevelopmentLength(Float64 db, Float64 fpe, Float64 fps) :
   pgsDevelopmentLengthBase(db, fpe, fps)
{
}

Float64 pgsPCIUHPCDevelopmentLength::GetDevelopmentLength() const
{
   Float64 fps_ksi = WBFL::Units::ConvertFromSysUnits(m_fps, WBFL::Units::Measure::KSI);
   Float64 fpe_ksi = WBFL::Units::ConvertFromSysUnits(m_fpe, WBFL::Units::Measure::KSI);
   Float64 db_inch = WBFL::Units::ConvertFromSysUnits(m_db, WBFL::Units::Measure::Inch);
   Float64 ld_inch = 20.0 * db_inch + 0.2 * (fps_ksi - fpe_ksi) * db_inch;
   Float64 ld = WBFL::Units::ConvertToSysUnits(ld_inch, WBFL::Units::Measure::Inch);
   return ld;
}

////////////////////////
////////////////////////
////////////////////////

pgsUHPCDevelopmentLength::pgsUHPCDevelopmentLength(Float64 lt, Float64 db, Float64 fpe, Float64 fps) :
   pgsDevelopmentLengthBase(db, fpe, fps), m_lt(lt)
{
}

Float64 pgsUHPCDevelopmentLength::GetDevelopmentLength() const
{
   Float64 fps_ksi = WBFL::Units::ConvertFromSysUnits(m_fps, WBFL::Units::Measure::KSI);
   Float64 fpe_ksi = WBFL::Units::ConvertFromSysUnits(m_fpe, WBFL::Units::Measure::KSI);
   Float64 db_inch = WBFL::Units::ConvertFromSysUnits(m_db, WBFL::Units::Measure::Inch);
   Float64 lt_inch = WBFL::Units::ConvertFromSysUnits(m_lt, WBFL::Units::Measure::Inch);

   Float64 ld_inch = lt_inch + 0.30 * (fps_ksi - fpe_ksi) * db_inch;
   Float64 ld = WBFL::Units::ConvertToSysUnits(ld_inch, WBFL::Units::Measure::Inch);
   return ld;
}

////////////////////////
////////////////////////
////////////////////////

pgsDevelopmentLengthReporterBase::pgsDevelopmentLengthReporterBase(IBroker* pBroker,const pgsDevelopmentLengthEngineer* pEngineer) :
   m_pBroker(pBroker),m_pEngineer(pEngineer)
{
}

void pgsDevelopmentLengthReporterBase::ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   (*pPara) << _T("Development Length") << rptNewLine;

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   pgsTypes::AdjustableStrandType adj_type = pSegment->Strands.GetAdjustableStrandType();
   m_AdjustableStrandName = pgsTypes::asHarped == adj_type ? _T("Harped") : _T("Adj. Straight");

   GET_IFACE(IDocumentType, pDocType);
   bool bIsPGSplice = pDocType->IsPGSpliceDocument();
   m_PoiType = (bIsPGSplice ? POI_ERECTED_SEGMENT : POI_SPAN);

   GET_IFACE(IPointOfInterest, pPoi);
   pPoi->GetPointsOfInterest(segmentKey, &m_vPoi);
}

const std::_tstring& pgsDevelopmentLengthReporterBase::GetAdjustableStrandName() const
{
   return m_AdjustableStrandName;
}

PoiAttributeType pgsDevelopmentLengthReporterBase::GetLocationPoiAttribute() const
{
   return m_PoiType;
}

const PoiList& pgsDevelopmentLengthReporterBase::GetPoiList() const
{
   return m_vPoi;
}

////////////////////////
////////////////////////
////////////////////////

pgsLRFDDevelopmentLengthReporter::pgsLRFDDevelopmentLengthReporter(IBroker* pBroker,const pgsDevelopmentLengthEngineer* pEngineer) :
   pgsDevelopmentLengthReporterBase(pBroker,pEngineer)
{
}

void pgsLRFDDevelopmentLengthReporter::ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const
{
   __super::ReportDevelopmentLengthDetails(segmentKey, pChapter);

   GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);


   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   (*pPara) << _T("AASHTO LRFD BDS ") << WBFL::LRFD::LrfdCw8th(_T("5.11.4.1, 5.11.4.2"), _T("5.9.4.3.1, 5.9.4.3.2")) << rptNewLine;
   if (IS_US_UNITS(pDisplayUnits))
   {
      (*pPara) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("DevLength_US.png")) << rptNewLine;
   }
   else
   {
      (*pPara) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("DevLength_SI.png")) << rptNewLine;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(16);
   (*pPara) << pTable << rptNewLine;

   ColumnIndexType col = 0;
   pTable->SetNumberOfHeaderRows(3);
   pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   pTable->SetColumnSpan(0, col, 10);
   (*pTable)(0, col) << _T("Straight Strands");
   pTable->SetColumnSpan(1, col, 5);
   (*pTable)(1, col) << _T("Bonded");
   (*pTable)(2, col++) << symbol(kappa);
   (*pTable)(2, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(2, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(2, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(2, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   pTable->SetColumnSpan(1, col, 5);
   (*pTable)(1, col) << _T("Debonded");
   (*pTable)(2, col++) << symbol(kappa);
   (*pTable)(2, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(2, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(2, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(2, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   pTable->SetColumnSpan(0, col, 5);
   (*pTable)(0, col) << GetAdjustableStrandName();
   pTable->SetColumnSpan(1, col, 5);
   (*pTable)(1, col) << _T("Bonded");
   (*pTable)(2, col++) << symbol(kappa);
   (*pTable)(2, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(2, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(2, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(2, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   const PoiList& vPoi = GetPoiList();
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      const auto& segmentKey(poi.GetSegmentKey());

      (*pTable)(row, col++) << location.SetValue(GetLocationPoiAttribute(), poi);

      for (int i = 0; i < 2; i++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

         const std::shared_ptr<pgsDevelopmentLength>& pDevLength = m_pEngineer->GetDevelopmentLengthDetails(poi, strandType, false); // not debonded
         const pgsDevelopmentLengthBase* pDevLengthBase = dynamic_cast<const pgsDevelopmentLengthBase*>(pDevLength.get());

         (*pTable)(row, col++) << dynamic_cast<const pgsLRFDDevelopmentLength*>(pDevLengthBase)->GetDevelopmentLengthFactor();
         (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFps());
         (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFpe());
         (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetStrandDiameter());
         (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetDevelopmentLength());

         if (strandType == pgsTypes::Straight)
         {
            const std::shared_ptr<pgsDevelopmentLength>& pDevLength = m_pEngineer->GetDevelopmentLengthDetails(poi, strandType, true); // debonded
            const pgsDevelopmentLengthBase* pDevLengthBase = dynamic_cast<const pgsDevelopmentLengthBase*>(pDevLength.get());
            (*pTable)(row, col++) << dynamic_cast<const pgsLRFDDevelopmentLength*>(pDevLengthBase)->GetDevelopmentLengthFactor();
            (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFps());
            (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFpe());
            (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetStrandDiameter());
            (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetDevelopmentLength());
         }
      } // next stand type

      row++;
   } // next poi
}

////////////////////////
////////////////////////
////////////////////////

pgsPCIUHPCDevelopmentLengthReporter::pgsPCIUHPCDevelopmentLengthReporter(IBroker* pBroker,const pgsDevelopmentLengthEngineer* pEngineer) :
   pgsDevelopmentLengthReporterBase(pBroker,pEngineer)
{
}

void pgsPCIUHPCDevelopmentLengthReporter::ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const
{
   __super::ReportDevelopmentLengthDetails(segmentKey, pChapter);

   GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);


   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   (*pPara) << _T("PCI UHPC SDG E.9.3.2.2-2") << rptNewLine;

   (*pPara) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("PCI_UHPC_DevLength.png")) << rptNewLine;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9);
   (*pPara) << pTable << rptNewLine;

   ColumnIndexType col = 0;
   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   pTable->SetColumnSpan(0, col, 4);
   (*pTable)(0, col) << _T("Straight Strands");
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   pTable->SetColumnSpan(0, col, 4);
   (*pTable)(0, col) << GetAdjustableStrandName() << _T(" Strands");
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   const PoiList& vPoi = GetPoiList();

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      const auto& segmentKey(poi.GetSegmentKey());

      (*pTable)(row, col++) << location.SetValue(GetLocationPoiAttribute(), poi);

      for (int i = 0; i < 2; i++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

         const std::shared_ptr<pgsDevelopmentLength>& pDevLength = m_pEngineer->GetDevelopmentLengthDetails(poi, strandType, false); // not debonded
         const pgsDevelopmentLengthBase* pDevLengthBase = dynamic_cast<const pgsDevelopmentLengthBase*>(pDevLength.get());

         (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFps());
         (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFpe());
         (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetStrandDiameter());
         (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetDevelopmentLength());
      } // next stand type

      row++;
   } // next poi
}


////////////////////////
////////////////////////
////////////////////////

pgsUHPCDevelopmentLengthReporter::pgsUHPCDevelopmentLengthReporter(IBroker* pBroker,const pgsDevelopmentLengthEngineer* pEngineer) :
   pgsDevelopmentLengthReporterBase(pBroker,pEngineer)
{
}

void pgsUHPCDevelopmentLengthReporter::ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const
{
   __super::ReportDevelopmentLengthDetails(segmentKey, pChapter);

   GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);


   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << _T("GS 1.9.4.3.2") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   (*pPara) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_DevLength.png")) << rptNewLine;
   (*pPara) << Sub2(_T("l"), _T("t")) << _T(" is the longer transfer length.") << rptNewLine;

   // NOTE: Transfer length is a part of the development length, but we don't list transfer length here
   // because it is reported with the transfer length details

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9);
   (*pPara) << pTable << rptNewLine;

   ColumnIndexType col = 0;
   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   pTable->SetColumnSpan(0, col, 4);
   (*pTable)(0, col) << _T("Straight Strands");
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   pTable->SetColumnSpan(0, col, 4);
   (*pTable)(0, col) << GetAdjustableStrandName() << _T(" Strands");
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   const PoiList& vPoi = GetPoiList();

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      const auto& segmentKey(poi.GetSegmentKey());

      (*pTable)(row, col++) << location.SetValue(GetLocationPoiAttribute(), poi);

      for (int i = 0; i < 2; i++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

         const std::shared_ptr<pgsDevelopmentLength>& pDevLength = m_pEngineer->GetDevelopmentLengthDetails(poi, strandType, false); // not debonded
         const pgsDevelopmentLengthBase* pDevLengthBase = dynamic_cast<const pgsDevelopmentLengthBase*>(pDevLength.get());

         (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFps());
         (*pTable)(row, col++) << stress.SetValue(pDevLengthBase->GetFpe());
         (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetStrandDiameter());
         (*pTable)(row, col++) << length.SetValue(pDevLengthBase->GetDevelopmentLength());
      } // next stand type

      row++;
   } // next poi
}