///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include "TxDOTCadWriter.h"
#include "TxDOTOptionalDesignData.h"
#include "TxDataExporter.h"

#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\MomentCapacity.h>
#include <IFace\DistributionFactors.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\DistFactorEngineer.h>
#include <IFace\GirderHandling.h>
#include <IFace\Intervals.h>

#include <IFace\RatingSpecification.h>
#include <PgsExt\RatingArtifact.h>
#include <PgsExt\ISummaryRatingArtifact.h>

#if defined _DEBUG
#include <IFace\DocumentType.h>
#endif

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderArtifactTool.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Utility functions
//
static std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi, bool isIBeam, const pgsPointOfInterest& pmid);
static std::_tstring FractionalStrandSize(Float64 realSize); // Return fractional string for strand size

//
// Utility Workhorse for writing debond information
//

class TxDOTDebondWriter : public TxDOTDebondTool
{
public:

   TxDOTDebondWriter(const CSegmentKey& segmentKey, Float64 girderLength, IStrandGeometry* pStrandGeometry):
   TxDOTDebondTool(segmentKey, girderLength, pStrandGeometry)
   {;}

   Uint32 WriteDebondDataData(CTxDataExporter& rDataExporter, Uint32 currRow);

   StrandIndexType GetTotalNumDebonds() const
   {
      return this->m_NumDebonded;
   }

private:
   void WriteZeroDebondInfo(CTxDataExporter& rDataExporter, Uint32 currRow);
   void WriteRowData(CTxDataExporter& rDataExporter, const RowData& row,Float64 Hg, Uint32 currRowNum);
};

int TxDOTCadWriter::WriteCADDataToFile (CTxDataExporter& rDataExporter, IBroker* pBroker, const CGirderKey& girderKey, txcwStrandLayoutType strandLayout, txcwNsTableLayout tableLayout, bool isIBeam)
{
#if defined _DEBUG
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument());
#endif

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, ISegmentData,pSegmentData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
	GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2(pBroker, IMomentCapacity, pMomentCapacity);
   GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pDistFact);
   GET_IFACE2(pBroker, IMaterials, pMaterial);
   GET_IFACE2(pBroker, IIntervals, pIntervals);
	GET_IFACE2(pBroker, IArtifact, pIArtifact);

   CSegmentKey segmentKey(girderKey,0);
   SpanIndexType spanIdx = girderKey.groupIndex;
   GirderIndexType gdrIdx = girderKey.girderIndex;
   CSpanKey spanKey(spanIdx,gdrIdx);

   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType lastIntervalIdx          = pIntervals->GetIntervalCount()-1;

 	const pgsSegmentArtifact* pGdrArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   if(!(pGdrArtifact->Passed()))
	{
//		AfxMessageBox(_T("The Specification Check was NOT Successful"),MB_OK);
	}

	// Create pois at the start of girder and mid-span
   PoiList vPoi;
   pPointOfInterest->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& pois(vPoi.front());
	
   vPoi.clear();
   pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
	ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& pmid(vPoi.back());

   StrandIndexType harpedCount   = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
   StrandIndexType straightCount = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);

   // Determine if harped strands are straight by comparing harped eccentricity at end/mid
   bool isHarpedDesign = strandLayout == tslHarped || isIBeam; // all I beams treated as harped
   bool are_harped_bent = strandLayout == tslHarped;

   // Small fakeout here to allow pure straight designs to coexist with harped output
   if (isIBeam && (strandLayout==tslStraight || strandLayout==tslRaisedStraight))
   {
      strandLayout = tslHarped;
   }

   // STRUCTURE NAME
   if (m_RowNum==0)
   {
      GET_IFACE2(pBroker,IProjectProperties,pProjectProperties);
      std::_tstring bridgeName = pProjectProperties->GetBridgeName();
      rDataExporter.WriteStringToCell(1, _T("StructureName"), m_RowNum, bridgeName.c_str());
   }

	// SPAN NUMBER
   rDataExporter.WriteStringToCell(1, _T("SpanNum"), m_RowNum, LABEL_SPAN(segmentKey.groupIndex));

	// GIRDER NUMBER
   rDataExporter.WriteStringToCell(1, _T("GdrNum"), m_RowNum, LABEL_GIRDER(segmentKey.girderIndex));

	// BEAM TYPE
   std::_tstring strb = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetGirderName();

   size_t start = strb.rfind(_T(" "));    // assume that last contiguous string is type
   strb.erase(0,start+1);

   rDataExporter.WriteStringToCell(1, _T("GdrType"), m_RowNum, strb.c_str());

	// STRAND PATTERN
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   std::_tstring ns_strand_pattern;
   bool do_write_ns_data = !IsTxDOTStandardStrands(strandLayout, pStrands->GetStrandDefinitionType(), segmentKey, pBroker );
   if (do_write_ns_data && TxDOTCadWriter::ttlnTwoTables == tableLayout)
   {
      ns_strand_pattern = std::_tstring((std::size_t)++m_NonStandardCnt, _T('*')); // write *, **, ***,... depending on number of non-standard beams
      rDataExporter.WriteStringToCell(1, _T("NonStd"), m_RowNum, ns_strand_pattern.c_str());
   }

	// STRAND COUNT
	StrandIndexType strandNum = harpedCount + straightCount;
   rDataExporter.WriteIntToCell(1, _T("NStot"), m_RowNum, strandNum);

	// STRAND SIZE
   const matPsStrand* strandMatP = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Straight);
   Float64 value = strandMatP->GetNominalDiameter();
   value = ::ConvertFromSysUnits( value, unitMeasure::Inch );
   std::_tstring strandSize = FractionalStrandSize (value); // Convert value to fraction representation

   rDataExporter.WriteStringToCell(1, _T("Size"), m_RowNum, strandSize.c_str());

   // STRAND STRENGTH
	int strandStrength = (strandMatP->GetGrade() == matPsStrand::Gr1725 ?  250 :  270);
   rDataExporter.WriteIntToCell(1, _T("Strength"), m_RowNum, strandStrength);

	// STRAND ECCENTRICITY AT CENTER LINE
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pmid, pgsTypes::Permanent).Y();
	Float64 strandEccCL = ::ConvertFromSysUnits( value, unitMeasure::Inch );

   rDataExporter.WriteFloatToCell(1, _T("eCL"), m_RowNum, strandEccCL);

	// STRAND ECCENTRICITY AT ENDS
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pois, pgsTypes::Permanent).Y();
	Float64 strandEccEnd = ::ConvertFromSysUnits( value, unitMeasure::Inch );

   rDataExporter.WriteFloatToCell(1, _T("eEnd"), m_RowNum, strandEccEnd);

	// CONCRETE RELEASE STRENGTH
   value = pMaterial->GetSegmentDesignFc(segmentKey,releaseIntervalIdx);
	Float64 concreteRelStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

   rDataExporter.WriteFloatToCell(1, _T("FCI"), m_RowNum, concreteRelStrength);

	// MINIMUM 28 DAY COMP. STRENGTH
	value = pMaterial->GetSegmentDesignFc(segmentKey,lastIntervalIdx);
	Float64 min28dayCompStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

   rDataExporter.WriteFloatToCell(1, _T("FC"), m_RowNum, min28dayCompStrength);

	// DESIGN LOAD COMPRESSIVE STRESS (TOP CL)
   const pgsFlexuralStressArtifact* pArtifact;
   Float64 fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;

   StressCheckTask task;
   task.intervalIdx = lastIntervalIdx;
   task.limitState = pgsTypes::ServiceI;
   task.stressType = pgsTypes::Compression;
   task.bIncludeLiveLoad = true;

   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( task,pmid.GetID() );
   fcTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
	value = -fcTop;

	Float64 designLoadCompStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

   rDataExporter.WriteFloatToCell(1, _T("fComp"), m_RowNum, designLoadCompStress);

	// DESIGN LOAD TENSILE STRESS (BOT CL)
   task.intervalIdx = lastIntervalIdx;
   task.limitState = pgsTypes::ServiceIII;
   task.stressType = pgsTypes::Tension;
   task.bIncludeLiveLoad = true;
   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( task,pmid.GetID() );
   ftBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);
	value = -ftBot;

	Float64 designLoadTensileStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

   rDataExporter.WriteFloatToCell(1, _T("fTens"), m_RowNum, designLoadTensileStress);

   // REQUIRED MINIMUM ULTIMATE MOMENT CAPACITY
   const MINMOMENTCAPDETAILS* pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(lastIntervalIdx,pmid,true);
   value = Max(pmmcd->Mu,pmmcd->MrMin);

	int reqMinUltimateMomentCapacity = (int)Round(::ConvertFromSysUnits( value, unitMeasure::KipFeet ));

   rDataExporter.WriteFloatToCell(1, _T("UltMom"), m_RowNum, reqMinUltimateMomentCapacity);

	// LIVE LOAD DISTRIBUTION FACTORS
   Float64 momentDistFactor = pDistFact->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);

   rDataExporter.WriteFloatToCell(1, _T("gMoment"), m_RowNum, momentDistFactor);

   Float64 shearDistFactor = pDistFact->GetShearDistFactor(spanKey,pgsTypes::StrengthI);

   rDataExporter.WriteFloatToCell(1, _T("gShear"), m_RowNum, shearDistFactor);

   // Design Load rating
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   std::vector<CGirderKey> girderKeys{ girderKey };
   std::shared_ptr<const pgsISummaryRatingArtifact> pInventoryRatingArtifact = pIArtifact->GetSummaryRatingArtifact(girderKeys,pgsTypes::lrDesign_Inventory,INVALID_INDEX);
   std::shared_ptr<const pgsISummaryRatingArtifact> pOperatingRatingArtifact = pIArtifact->GetSummaryRatingArtifact(girderKeys,pgsTypes::lrDesign_Operating,INVALID_INDEX);

   // Strength I
   Float64 invMomRF = pInventoryRatingArtifact->GetMomentRatingFactor(true);
   Float64 invShearRF(0.0);
   if (pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory))
   {
      invShearRF = pInventoryRatingArtifact->GetShearRatingFactor();
   }
   
   Float64 oprMomRF = pOperatingRatingArtifact->GetMomentRatingFactor(true);
   Float64 oprShearRF(0.0);
   if (pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating))
   {
      oprShearRF = pOperatingRatingArtifact->GetShearRatingFactor();
   }

   // Inventory and operating Str I RF's are envelope of moment and shear
   Float64 invRF = min(invMomRF, invShearRF);
   Float64 oprRF = min(oprMomRF, oprShearRF);

   // Service III
   Float64 invStressRF = pInventoryRatingArtifact->GetStressRatingFactor();

   rDataExporter.WriteFloatToCell(1, _T("RFStr1Inventory"), m_RowNum, invRF);
   rDataExporter.WriteFloatToCell(1, _T("RFStr1Operating"), m_RowNum, oprRF);
   rDataExporter.WriteFloatToCell(1, _T("RFSvc3Inventory"), m_RowNum, invStressRF);

   // Done with values that are common to both strand layouts. Now write to specific layouts
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE2(pBroker, ISectionProperties,pSectProp);
   Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, pois);

   // use utility class for writing debond information
   TxDOTDebondWriter writer(segmentKey, girder_length, pStrandGeometry);

   if (isHarpedDesign)
   {
	   // COUNT OF DEPRESSED (HARPED) STRANDS 
	   StrandIndexType dstrandNum;

	   // DEPRESSED (HARPED) STRAND
      Float64 dstrandToEnd;
      Float64 dstrandToCL;

      if(!are_harped_bent)
      {
         // Report harped strands as straight
         dstrandNum = 0;
         dstrandToEnd = 0.0;
         dstrandToCL = 0.0;
      }
      else
      {
         dstrandNum = harpedCount;

         pStrandGeometry->GetHighestHarpedStrandLocationEnds(segmentKey, &value);

         // value is measured down from top of girder... we want it measured up from the bottom
         value += Hg;

         dstrandToEnd = ::ConvertFromSysUnits( value, unitMeasure::Inch );

         pStrandGeometry->GetHighestHarpedStrandLocationHPs(segmentKey, &value);
         value += Hg;

         dstrandToCL = ::ConvertFromSysUnits( value, unitMeasure::Inch );
      }

      if (dstrandNum > 0) // for harped sheet leave blanks when no strands
      {
         rDataExporter.WriteIntToCell(1, _T("NH"), m_RowNum, dstrandNum);
         rDataExporter.WriteFloatToCell(1, _T("ToEnd"), m_RowNum, dstrandToEnd);
      }
   }
   else
   {

      m_RowNum = writer.WriteDebondDataData(rDataExporter, m_RowNum);
   }

   if (do_write_ns_data)
   {
      std::_tstring ns_strand_str;
      ns_strand_str = MakeNonStandardStrandString(pBroker,pmid, isIBeam, pmid);

      if (tableLayout == TxDOTCadWriter::ttlnTwoTables)
      {
         // Non-Standard Design Data on worksheet #2
         rDataExporter.WriteStringToCell(2, _T("NSPattern"), m_NonStandardCnt - 1, ns_strand_pattern.c_str());
         rDataExporter.WriteStringToCell(2, _T("NSArrangement"), m_NonStandardCnt - 1, ns_strand_str.c_str());
      }
      else
      {
         rDataExporter.WriteStringToCell(1, _T("NSArrangement"), m_RowNum, ns_strand_str.c_str());
      }
   }

   if (writer.GetTotalNumDebonds() > 0 && are_harped_bent)
   {
      CString msg;
      msg.Format(_T("Warning: Beam %s in span %s has mixed harped and debonded strands. This is an invalid strand configuration for TxDOT, and is not supported by the TxDOT CAD Data Export feature. Strand data may not be reported correctly.\n"), LABEL_GIRDER(gdrIdx), LABEL_SPAN(spanIdx));
      rDataExporter.WriteWarningToCell(1, _T("StructureName"), ++m_RowNum, msg);
   }


   m_RowNum++;

   return CAD_SUCCESS;
}

Uint32 TxDOTDebondWriter::WriteDebondDataData(CTxDataExporter& rDataExporter, Uint32 currRow)
{
   rDataExporter.WriteIntToCell(1, _T("NDBtot"), currRow, m_NumDebonded);

   if (m_NumDebonded > 0)
   {
      // write out debonding data for bottom row

      if (m_OutCome == SectionMismatch || m_OutCome == TooManySections || m_OutCome == SectionsNotSymmetrical || m_OutCome == NonStandardSection)
      {
         // warning condition - don't write any data. Will write warning later in this function
      }
      else if (m_Rows.empty())
      {
         // row height, srands in row, and debonds in row are zero. Just write zeros to all fields
         WriteZeroDebondInfo(rDataExporter, currRow);
      }
      else
      {
         // A little checking
         pgsPointOfInterest poi(m_SegmentKey, m_GirderLength/2.0);
         RowIndexType nrs = m_pStrandGeometry->GetNumRowsWithStrand(poi,pgsTypes::Straight);
         ATLASSERT((RowIndexType)m_Rows.size() == nrs); // could have more rows than rows with debonded strands

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
      
         GET_IFACE2(pBroker, IIntervals, pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);

         GET_IFACE2(pBroker,ISectionProperties,pSectProp);
         Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poi);

         // Where the rubber hits the road - Write rows
         bool did_write(false);
         RowListIter riter = m_Rows.begin();
         while(riter != m_Rows.end())
         {
            const RowData& row = *riter;
            // Only write rows that contain debonding
            if (!row.m_Sections.empty())
            {
               WriteRowData(rDataExporter, row, Hg, currRow++);
               did_write = true;
            }

            riter++;
         }

         if (did_write)
         {
            currRow--; // need to back up to last row written
         }
      }
   }
   else
   {
      // No debonding. Just write zeros
      WriteZeroDebondInfo(rDataExporter, currRow);
   }

   // Lastly, write any warnings or errors
   SpanIndexType spanIdx = m_SegmentKey.groupIndex;
   GirderIndexType gdrIdx = m_SegmentKey.girderIndex;
   ATLASSERT(m_SegmentKey.segmentIndex == 0);

   if (m_OutCome==SectionMismatch)
   {
      CString msg;
      msg.Format(_T("Warning: Irregular, Non-standard debonding increments used for girder %s in span %s. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
      rDataExporter.WriteWarningToCell(1, _T("StructureName"), ++currRow, msg);
   }
   if (m_OutCome==SectionsNotSymmetrical)
   {
      CString msg;
      msg.Format(_T("Warning: Debonding is not symmetrical for girder %s in span %s. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
      rDataExporter.WriteWarningToCell(1, _T("StructureName"), ++currRow, msg);
   }
   else if (m_OutCome==TooManySections)
   {
      CString msg;
      msg.Format(_T("Warning: The number of debonded sections exceeds ten for girder %s in span %s. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
      rDataExporter.WriteWarningToCell(1, _T("StructureName"), ++currRow, msg);
   }
   else if (m_OutCome==NonStandardSection)
   {
      Float64 spac = ::ConvertFromSysUnits(m_SectionSpacing , unitMeasure::Feet );
      CString msg;
      msg.Format( _T("Warning: Non-standard debonding increment of %6.3f ft used  for girder %s in span %s. \n"),spac,LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
      rDataExporter.WriteWarningToCell(1, _T("StructureName"), ++currRow, msg);
   }

   return currRow;
}

void TxDOTDebondWriter::WriteRowData(CTxDataExporter& rDataExporter, const RowData& row, Float64 Hg, Uint32 currRowNum)
{
   // Blast all debond columns with zeros (easier than figuring out what is and isn't)
   WriteZeroDebondInfo(rDataExporter, currRowNum);

   // elevation of row
   Float64 row_elev = ::ConvertFromSysUnits( Hg + row.m_Elevation, unitMeasure::Inch );
   rDataExporter.WriteFloatToCell(1, _T("DBBotDist"), currRowNum, row_elev);

   // total strands in row
   rDataExporter.WriteIntToCell(1, _T("NSRow"), currRowNum, row.m_NumTotalStrands);

   // num debonded strands in row
   Int16 nsr = CountDebondsInRow(row);
   rDataExporter.WriteIntToCell(1, _T("NDBRow"), currRowNum, nsr);

   // cycle true each section. *** Note that sections are in 3' intervals - this is written in stone ***
   for (const auto& section : row.m_Sections)
   {
      Float64 xloc = ::ConvertFromSysUnits(section.m_XLoc, unitMeasure::Feet);

      // only write rows with data
      if (IsEqual(xloc, 3.0))
      {
         rDataExporter.WriteIntToCell(1, _T("DB_3"), currRowNum, section.m_NumDebonds);
      }
      else if (IsEqual(xloc, 6.0))
      {
         rDataExporter.WriteIntToCell(1, _T("DB_6"), currRowNum, section.m_NumDebonds);
      }
      else if (IsEqual(xloc, 9.0))
      {
         rDataExporter.WriteIntToCell(1, _T("DB_9"), currRowNum, section.m_NumDebonds);
      }
      else if (IsEqual(xloc, 12.0))
      {
         rDataExporter.WriteIntToCell(1, _T("DB_12"), currRowNum, section.m_NumDebonds);
      }
      else if (IsEqual(xloc, 15.0))
      {
         rDataExporter.WriteIntToCell(1, _T("DB_15"), currRowNum, section.m_NumDebonds);
      }
   }
}

void  TxDOTDebondWriter::WriteZeroDebondInfo(CTxDataExporter& rDataExporter, Uint32 currRow)
{
   rDataExporter.WriteFloatToCell(1, _T("DBBotDist"), currRow, 0.0);
   rDataExporter.WriteIntToCell(1, _T("NSRow"), currRow, 0);
   rDataExporter.WriteIntToCell(1, _T("NDBRow"), currRow, 0);
   rDataExporter.WriteIntToCell(1, _T("DB_3"), currRow, 0);
   rDataExporter.WriteIntToCell(1, _T("DB_6"), currRow, 0);
   rDataExporter.WriteIntToCell(1, _T("DB_9"), currRow, 0);
   rDataExporter.WriteIntToCell(1, _T("DB_12"), currRow, 0);
   rDataExporter.WriteIntToCell(1, _T("DB_15"), currRow, 0);
}

std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi, bool isIBeam, const pgsPointOfInterest& pmid)
{
   std::_tostringstream os;

   bool no_strands = true;
   if (isIBeam)
   {
      OptionalDesignHarpedFillUtil::StrandRowSet strandrows = OptionalDesignHarpedFillUtil::GetStrandRowSet(pBroker, pmid);

      bool first = true;
      for (auto srow : strandrows)
      {
         if (!first)
            os << _T(", "); //  This does not work well with CSV formatting
         else
            first = false;

         Float64 elev_in = RoundOff(::ConvertFromSysUnits(srow.Elevation, unitMeasure::Inch), 0.001);
         os << elev_in << _T("(") << srow.fillListString  << _T(")");
      }

      no_strands = strandrows.empty();
   }
   else
   {
      StrandRowUtil::StrandRowSet strandrows = StrandRowUtil::GetStrandRowSet(pBroker, midPoi);
      bool first = true;
      for (auto srow : strandrows)
      {
         if (!first)
            os << _T(", ");
         else
            first = false;

         Float64 elev_in = RoundOff(::ConvertFromSysUnits(srow.Elevation, unitMeasure::Inch), 0.001);
         os << elev_in << _T("(") << srow.Count  << _T(")");
      }

      no_strands = strandrows.empty();
   }

   if (no_strands)
   {
      os << _T("No strands are defined for this girder");
   }

   return os.str();
}

std::_tstring FractionalStrandSize(Float64 value)
{
    if(value < 0.0 || 1.0 < value)
    {
        ATLASSERT(0); // we don't deal with more than an inch
        return std::_tstring(_T("Err "));
    }

    const size_t size = 32;
    TCHAR stringP[size];

    // See if we can resolve to 1/16th's
    const Float64 stinkth = 1.0/16;
    Float64 mod16 = fmod(value, stinkth);
    if (1.0e-05 < mod16)
    {
        // Not a 16th - Print decimal value
        _stprintf_s(stringP, size, _T(" %3.1f"),value);
    }
    else
    {
        Float64 num_16ths = Round(value/stinkth);
        Float64 numerator(num_16ths), denominator(16.0);
        // loop until we get an odd numerator
        while(IsZero(fmod(numerator, 2.0)))
        {
            numerator /= 2.0;
            denominator /= 2.0;
        }

        Int32 num = (Int32)Round(numerator);
        Int32 den = (Int32)Round(denominator);

        // Want to right justify in four characters
        ATLASSERT(num < 9);  // we can't handle something like 11/16 (five chars)
        int nd = den < 9 ? 1 : 2; // number of decimals in denom 

        int nc;
        if (nd>1)
        {
            nc = _stprintf_s(stringP, size, _T("%d/%-d"), num, den);
        }
        else
        {
            nc = _stprintf_s(stringP, size, _T(" %d/%-d"), num, den); // leading space
        }
    }

    return std::_tstring(stringP);
}

