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

#include "StdAfx.h"

#include "TestFileWriter.h"

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


static void write_spec_check_results(FILE *fp, IBroker* pBroker, const CGirderKey& girderKey, bool designSucceeded);
static std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi);

// Return fractional string for strand size
static int txdString_ftofrac	/* <=  Completion value                   */
(
LPTSTR      stringP,		      /* <=  Output text string                 */
size_t      size,             /* <= size of output string               */
Float64		value 			   /*  => Value to convert                   */
)
{
    if(value < 0.0 || 1.0 < value)
    {
        ATLASSERT(0); // we don't deal with more than an inch
        _stprintf_s(stringP, 4, _T("Err "));
        return CAD_FAIL;
    }

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

	return CAD_SUCCESS;
}

// Function to return number of raised straight strands as defined by top kern point
inline StrandIndexType GetNumRaisedStraightStrands(IStrandGeometry * pStrandGeometry, const CSegmentKey& segmentKey,  const pgsPointOfInterest& pois, Float64 kt )
{
   StrandIndexType numRaisedStraightStrands = 0;
   if (pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey) && 0 < pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped))
   {

      CComPtr<IPoint2dCollection> pPoints;
      pStrandGeometry->GetStrandPositions(pois, pgsTypes::Harped, &pPoints);
      StrandIndexType Ns;
      pPoints->get_Count(&Ns);
      StrandIndexType strandIdx;
      for (strandIdx = 0; strandIdx < Ns; strandIdx++)
      {
         CComPtr<IPoint2d> strand_point;
         pPoints->get_Item(strandIdx, &strand_point);

         Float64 y;
         strand_point->get_Y(&y); // measured in Girder Section Coordinates
         y *= -1.0;
         if (y < kt)
         {
            numRaisedStraightStrands++; // above the kern
         }
      }
   }

   return numRaisedStraightStrands;
}

// Legacy TxDOT algo for determining strand fill type. Not really useful for our purposes, but this is how it's always been done...
static bool IsTxStandardStrands(bool isHarpedDesign, pgsTypes::StrandDefinitionType sdtType, const CSegmentKey& segmentKey, IBroker* pBroker)
{
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

   StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
   StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
   StrandIndexType nperm = ns + nh;

   if (nperm == 0)
   {
      return false; // no strands, not standard
   }
   else if (sdtType == pgsTypes::sdtTotal)
   {
      // strands filled using library order. always standard
      return true;
   }
   else if (sdtType == pgsTypes::sdtStraightHarped)
   {
      // check if strands entered straight/harped match library order. then standard
      StrandIndexType tns, tnh;
      if (pStrandGeometry->ComputeNumPermanentStrands(nperm, segmentKey, &tns, &tnh))
      {
         if (tns == ns && tnh == nh)
         {
            return true;
         }
      }
   }

   if (isHarpedDesign)
   {
      // standard harped designs must be filled using library order
      return false;
   }
   else if (sdtType == pgsTypes::sdtDirectSelection)
   {
      // This is the hard one. We have a straight design, possibly with raised strands. In order to be standard;
      // the bottom half of the girder must be filled filling each row completely from the bottom up.
   	GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
      PoiList vPoi;
      pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
	   ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& pmid(vPoi.back());

      // Get list of strand rows filled by current project
      StrandRowUtil::StrandRowSet strandrows = StrandRowUtil::GetStrandRowSet(pBroker, pmid);

      // Get list of strand rows for case with all possible permanent strands filled
      StrandRowUtil::StrandRowSet popstrandrows = StrandRowUtil::GetFullyPopulatedStrandRowSet(pBroker, pmid);

      // Only consider strands in the bottom half of the girder (raised strands are standard)
      GET_IFACE2(pBroker,IGirder,pGirder);
      Float64 hg2 = pGirder->GetHeight(pmid) / 2.0;

      bool isStandard(true);
      bool oneBelow(false); // at least one row must be below mid-height
      RowIndexType numPartiallyFilledRows = 0; // we can have one partially filled row only
      StrandRowUtil::StrandRowIter itstr = strandrows.begin();
      StrandRowUtil::StrandRowIter itpopstr = popstrandrows.begin();
      while (itstr != strandrows.end())
      {
         if (itstr->Elevation >= hg2)
         {
            // row is above half height. we are done and stardard if at least one row is below mid-height
            isStandard = oneBelow;
            break;
         }
         else if (!IsEqual(itstr->Elevation, itpopstr->Elevation))
         {
            // can't skip possible rows when we are below half height - rows must be continuously filled from bottom
            isStandard = false;
            break;
         }
         else if (itstr->Count != itpopstr->Count)
         {
            // a partially filled row
            if (++numPartiallyFilledRows > 1)
            {
               // can only have one partially filled row below half height
               isStandard = false;
               break;
            }
         }
         else // (itstr->Count == itpopstr->Count) // by default
         {
            // a fully filled row
            if (numPartiallyFilledRows > 0)
            {
               // cannot have a fully filled row above a partially filled row
               isStandard = false;
               break;
            }
         }

         oneBelow = true;
         itstr++;
         itpopstr++;
      }

      return isStandard;
   }
   else
   {
      return false; // myriad of other cases fall through the cracks
   }
}


int Test_WriteCADDataToFile (FILE *fp, IBroker* pBroker, const CGirderKey& girderKey, bool designSucceeded)
{
#if defined _DEBUG
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument());
#endif
   CSegmentKey segmentKey(girderKey,0);
   SpanIndexType spanIdx = girderKey.groupIndex;
   GirderIndexType gdrIdx = girderKey.girderIndex;
   CSpanKey spanKey(spanIdx,gdrIdx);

// Get data first and convert to correct units. then write it all at end of function
// Note that Units are hard-coded into this routine. TxDOT has no use for SI units
	TCHAR	charBuffer[32];
   Float64 value;


	/* Regenerate bridge data */
	GET_IFACE2(pBroker, IArtifact, pIArtifact);
 	const pgsSegmentArtifact* pGdrArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   if(!(pGdrArtifact->Passed()))
	{
//		AfxMessageBox(_T("The Specification Check was NOT Successful"),MB_OK);
	}

	/* Interfaces to all relevant agents */
   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, ISegmentData,pSegmentData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
	GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2(pBroker, IMomentCapacity, pMomentCapacity);
   GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pDistFact);
   GET_IFACE2(pBroker, IMaterials, pMaterial);
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   GET_IFACE2(pBroker,IProjectProperties,pProjectProperties);

   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType lastIntervalIdx          = pIntervals->GetIntervalCount()-1;

   // Use workerbee class to do actual writing of data
   bool is_test_output = true; // (format == tcxTest) ? true : false;
   CadWriterWorkerBee workerB(is_test_output);//

	// Create pois at the start of girder and mid-span
   PoiList vPoi;
   pPointOfInterest->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& pois(vPoi.front());

   // Attempt to get poi at mid-span. If segment does not have a poi there, then use center of segment
   vPoi.clear();
   pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
   if (vPoi.empty())
   {
      pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   }
	ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& pmid(vPoi.back());

   // IGirders are treated differently than others
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring girderFamily = pBridgeDesc->GetGirderFamilyName();

   bool isIBeam = girderFamily == _T("I-Beam");
   bool isUBeam = girderFamily == _T("U-Beam");

   // Determine type of output and number of strands
   bool isHarpedDesign = !pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey) &&
                        0 < pStrandGeometry->GetMaxStrands(segmentKey, pgsTypes::Harped) &&
                        isIBeam;

   StrandIndexType harpedCount   = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
   StrandIndexType straightCount = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);

   // Determine if harped strands are straight by comparing harped eccentricity at end/mid
   bool are_harped_bent(false);
   if (0 < harpedCount)
   {
      Float64 hs_ecc_end = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pois,pgsTypes::Harped).Y();
      Float64 hs_ecc_mid = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pmid,pgsTypes::Harped).Y();
      are_harped_bent = !IsEqual(hs_ecc_end, hs_ecc_mid);
   }

   bool isExtendedVersion = true; // (format == tcxExtended || format == tcxTest);

   // Determine if a straight-raised design
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   Float64 kt = pSectProp->GetKt(releaseIntervalIdx, pois);

   StrandIndexType numRaisedStraightStrands = GetNumRaisedStraightStrands(pStrandGeometry, segmentKey, pois, kt);

   std::_tstring bridgeName = pProjectProperties->GetBridgeName();
   // Max length of name is 16 chars
   if (bridgeName.size()>16)
   {
      bridgeName.resize(16);
   }

   // extended version writes data at front and back of line
   if (isExtendedVersion)
   {
	   /* 0a. ROADWAY WIDTH */
      value = pBridge->GetCurbToCurbWidth(0.00);

      Float64 roadwayWidth = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Feet );

	   /* 0b. NUMBER OF BEAMS */
      GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);

	   /* 0a. BEAM SPACING */
      ATLASSERT( pBridgeDesc->GetGirderSpacingType() != pgsTypes::sbsGeneral );
      GirderIndexType spaceIdx = (segmentKey.girderIndex == nGirders-1 ? nGirders-2 : segmentKey.girderIndex);
      value = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing(spaceIdx);

      Float64 girderSpacing = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Feet );

	   //----- COL 0a ---- 
	   workerB.WriteFloat64(roadwayWidth,_T("RoadW"),7,5,_T("%5.2f"));
	   //----- COL 0b ----- 
	   workerB.WriteInt16((Int16)nGirders,_T("Ng "),5,3,_T("%3d"));
	   //----- COL 0c ----- 
	   workerB.WriteFloat64(girderSpacing,_T("Spcng"),7,5,_T("%5.2f"));
   }


	/* 1. SPAN NUMBER */
	TCHAR	spanNumber[5+1];
	_stprintf_s(spanNumber, sizeof(spanNumber)/sizeof(TCHAR), _T("%s"), LABEL_SPAN(segmentKey.groupIndex));

	/* 1. GIRDER NUMBER */
	TCHAR  beamNumber[5+1];
	_stprintf_s(beamNumber, sizeof(beamNumber)/sizeof(TCHAR), _T("%s"), LABEL_GIRDER(segmentKey.girderIndex));

	/* 3. BEAM TYPE */
	TCHAR beamType[5+1];
   beamType[5] = _T('\0');
#if defined _UNICODE
   wmemset( beamType,_T(' '),5);
#else
   memset( beamType,' ',5);
#endif
   std::_tstring str = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetGirderName();

   // assume that last contiguous string is type
   size_t start = str.rfind(_T(" "));
   str.erase(0,start+1);
   size_t cnt = Min((size_t)5, str.length());

   // if string doesn't fill, leave first char blank
   TCHAR* cnxt = beamType;
   size_t count = sizeof(beamType)/sizeof(TCHAR);
   if (cnt<5)
   {
      cnxt++;
      count--;
   }

   _tcsncpy_s(cnxt, count, str.c_str(), cnt);

	/* 4. STRAND PATTERN */
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   TCHAR  strandPat[5+1]; 
   bool do_write_ns_data = !IsTxStandardStrands( isHarpedDesign, pStrands->GetStrandDefinitionType(), segmentKey, pBroker );
   if (do_write_ns_data)
   {
	   _tcscpy_s(strandPat, sizeof(strandPat)/sizeof(TCHAR), _T("*"));
   }
   else
   {
	   _tcscpy_s(strandPat, sizeof(strandPat)/sizeof(TCHAR), _T(" "));
   }

	/* 5. STRAND COUNT */

	StrandIndexType strandNum = harpedCount + straightCount;


	/* 6. STRAND SIZE */
	TCHAR    strandSize[4+1];
   const auto* strandMatP = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Straight);
   value = strandMatP->GetNominalDiameter();
   value = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

	/* Convert value to fraction representation */
	txdString_ftofrac (charBuffer, sizeof(charBuffer)/sizeof(TCHAR), value); 
	_tcscpy_s(strandSize, sizeof(strandSize)/sizeof(TCHAR), charBuffer);

   /* 7. STRAND STRENGTH */
	int strandStrength = (strandMatP->GetGrade() == WBFL::Materials::PsStrand::Grade::Gr1725 ?  250 :  270);

	/* 8. STRAND ECCENTRICITY AT CENTER LINE */
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pmid, pgsTypes::Permanent).Y();

	Float64 strandEccCL = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

	/* 9. STRAND ECCENTRICITY AT END */
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pois, pgsTypes::Permanent).Y();

	Float64 strandEccEnd = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

	/* 12. CONCRETE RELEASE STRENGTH */
   value = pMaterial->GetSegmentDesignFc(segmentKey,releaseIntervalIdx);

	Float64 concreteRelStrength = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::KSI );

	/* 13. MINIMUM 28 DAY COMP. STRENGTH */
	value = pMaterial->GetSegmentDesignFc(segmentKey,lastIntervalIdx);

	Float64 min28dayCompStrength = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::KSI );

	/* 14. DESIGN LOAD COMPRESSIVE STRESS (TOP CL) */ 
   const pgsFlexuralStressArtifact* pArtifact;
   Float64 fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;

   StressCheckTask task;
   task.intervalIdx = lastIntervalIdx;
   task.limitState = pgsTypes::ServiceI;
   task.stressType = pgsTypes::Compression;
   task.bIncludeLiveLoad = true;
   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( task,pmid.GetID() );
   if (pArtifact)
   {
      fcTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
      value = -fcTop;
   }

	Float64 designLoadCompStress = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::KSI );

	/* 15. DESIGN LOAD TENSILE STRESS (BOT CL) */
   task.intervalIdx = lastIntervalIdx;
   task.limitState = pgsTypes::ServiceIII;
   task.stressType = pgsTypes::Tension;
   task.bIncludeLiveLoad = true;
   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( task,pmid.GetID() );
   if (pArtifact)
   {
      ftBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);
      value = -ftBot;
   }

	Float64 designLoadTensileStress = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::KSI );

   /* 16. REQUIRED MINIMUM ULTIMATE MOMENT CAPACITY */
   const MINMOMENTCAPDETAILS* pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(lastIntervalIdx,pmid,true);
   value = Max(pmmcd->Mu,pmmcd->MrMin);

	int reqMinUltimateMomentCapacity = (int)Round(WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::KipFeet ));

	/* 17. LIVE LOAD DISTRIBUTION FACTOR */
   Float64 momentDistFactor = pDistFact->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);

	/* 17aa. LIVE LOAD DISTRIBUTION FACTOR */
   Float64 shearDistFactor = pDistFact->GetShearDistFactor(spanKey,pgsTypes::StrengthI);

   /* 17a - Non-Standard Design Data */
   std::_tstring ns_strand_str;
   if (do_write_ns_data && !isExtendedVersion)
   {
      ns_strand_str = MakeNonStandardStrandString(pBroker,pmid);
   }

   // WRITE DATA TO OUTPUT FILE
   if (isExtendedVersion)
   {
      workerB.WriteString(bridgeName.c_str(),_T("STRUCTURE"), 16, (Int16)bridgeName.size(),_T("%s"));
   }

   //----- COL 1 ----- 
   int ls = lstrlen(spanNumber);
   int lp = 7-ls-2;
   ATLASSERT(lp>0);
   workerB.WriteStringEx(spanNumber,_T("Span"),lp,ls,2,_T("%s"));
	//----- COL 2 ----- 
   ls = lstrlen(beamNumber);
   lp = 7-ls-3;
   ATLASSERT(lp>0);
   workerB.WriteStringEx(beamNumber,_T(" Gdr"),lp,ls,3,_T("%s"));
	//----- COL 3 ----- 
   workerB.WriteString(beamType,_T("Type "),7,5,_T("%-5s"));
	//----- COL 4 ----- 
   workerB.WriteString(strandPat,_T("N"),6,1,_T("%1s"));
	//----- COL 5 ----- 
   workerB.WriteInt16((Int16)strandNum,_T("Ns"),6,3,_T("%3d"));
	//----- COL 6 ----- 
   workerB.WriteStringEx(strandSize,_T("Size"),0,4,1,_T("%4s"));
	//----- COL 7 ----- 
   workerB.WriteInt16(strandStrength,_T("Strn"),5,3,_T("%3d"));
	//----- COL 8 ----- 
   workerB.WriteFloat64(strandEccCL,_T("EccCL"),7,5,_T("%5.2f"));
	//----- COL 9 ----- 
   workerB.WriteFloat64(strandEccEnd,_T("EccEn"),7,5,_T("%5.2f"));

   Int16 extraSpacesForSlabOffset = 0; // Pad in debond additional lines for output of A
   if (isExtendedVersion && pBridge->GetDeckType()!=pgsTypes::sdtNone && pBridge->GetHaunchInputDepthType()==pgsTypes::hidACamber)
   {
      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(segmentKey.groupIndex, &startPierIdx, &endPierIdx);

      Float64 astart = pBridge->GetSlabOffset(segmentKey, pgsTypes::metStart);
      Float64 aend = pBridge->GetSlabOffset(segmentKey, pgsTypes::metEnd);


      astart = WBFL::Units::ConvertFromSysUnits( astart, WBFL::Units::Measure::Inch );
      aend = WBFL::Units::ConvertFromSysUnits( aend, WBFL::Units::Measure::Inch );

      workerB.WriteFloat64(astart,_T("Astart"),7,5,_T("%5.2f"));
      workerB.WriteFloat64(aend,_T("Aend"),7,5,_T("%5.2f"));

      extraSpacesForSlabOffset = 14; // width of two data fields above = 7+7

      GET_IFACE2(pBroker,ISpecification,pSpec);
      if (pSpec->IsAssumedExcessCamberInputEnabled())
      {
         value = pIBridgeDesc->GetAssumedExcessCamber(segmentKey.groupIndex, segmentKey.girderIndex);
      	Float64 aecamber = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );
         workerB.WriteFloat64(aecamber,_T("AECmbr"),7,5,_T("%5.2f"));
         extraSpacesForSlabOffset += 7;
      }
    }

   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

   Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, pois);

   // create debond writer in case we need it
   TestFileWriter writer(segmentKey, girder_length, isUBeam, pStrandGeometry);

   if (isHarpedDesign)
   {
      ATLASSERT(isIBeam);

      // Empty space in IGND
      workerB.WriteBlankSpaces(44);

	   /* 10. COUNT OF DEPRESSED (HARPED) STRANDS */
	   StrandIndexType dstrandNum;

	   /* 11. DEPRESSED (HARPED) STRAND */
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

         dstrandToEnd = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

         pStrandGeometry->GetHighestHarpedStrandLocationHPs(segmentKey, &value);
         value += Hg;

         dstrandToCL = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );
      }

      // output
      //----- COL 10 ---- 
      workerB.WriteInt16((Int16)dstrandNum,_T("Nh"),(isExtendedVersion? 4:5),2,_T("%2d"));
	   //----- COL 11 ---- 
      workerB.WriteFloat64(dstrandToEnd,_T("ToEnd"),5,4,_T("%4.1f"));
      workerB.WriteFloat64(dstrandToCL,_T("ToCL"),5,4,_T("%4.1f"));
   }
   else
   {
      if (isIBeam)
      {
         // Empty space in IGND column
         workerB.WriteBlankSpaces(1);
      }
      // debond or straight design
      writer.WriteInitialData(workerB);

      if (isIBeam && numRaisedStraightStrands==0)
      {
         // Empty spaces in IGND
         workerB.WriteBlankSpaces(isExtendedVersion ? 14:15);
      }
      else if (numRaisedStraightStrands>0)
      {
         // Raised strand design data. 
         Float64 dstrandToEnd(0);
         Float64 dstrandToCL(0);

         pStrandGeometry->GetHighestHarpedStrandLocationEnds(segmentKey, &value);
         dstrandToEnd = WBFL::Units::ConvertFromSysUnits( value+Hg, WBFL::Units::Measure::Inch );

         pStrandGeometry->GetHighestHarpedStrandLocationHPs(segmentKey, &value);
         dstrandToCL = WBFL::Units::ConvertFromSysUnits( value+Hg, WBFL::Units::Measure::Inch );

         // output
         workerB.WriteInt16((Int16)numRaisedStraightStrands,_T("Nh"),(isExtendedVersion? 4:5),2,_T("%2d"));
         workerB.WriteFloat64(dstrandToEnd,_T("ToEnd"),5,4,_T("%4.1f"));
         workerB.WriteFloat64(dstrandToCL,_T("ToCL"),5,4,_T("%4.1f"));
      }
   }

   // onward with common data for harped or debond designs
	//----- COL 12 ---- 
   workerB.WriteFloat64(concreteRelStrength,_T(" Fci  "),7,6,_T("%6.3f"));
	//----- COL 13 ---- 
   workerB.WriteFloat64(min28dayCompStrength,_T(" Fc   "),7,6,_T("%6.3f"));

   if (isIBeam && !isExtendedVersion) // Tweak from txdot - do not affect test files
   {
      workerB.WriteBlankSpaces(1);
   }

	//----- COL 14 ---- 
   workerB.WriteFloat64(designLoadCompStress,_T(" fcomp"),10,6,_T("%6.3f"));
	//----- COL 15 ---- 
   workerB.WriteFloat64(designLoadTensileStress,_T(" ftens "),10,6,_T("%6.3f"));

	//----- COL 16 ---- 
   workerB.WriteInt32(reqMinUltimateMomentCapacity,_T("ultMo"),9,5,_T("%5d"));
	//----- COL 17 ---- 
   workerB.WriteFloat64(momentDistFactor,_T("LLDFm"),7,5,_T("%5.3f"));
	//----- COL 17aa ---- 
   workerB.WriteFloat64(shearDistFactor,_T("LLDFs"),7,5,_T("%5.3f"));

   if (do_write_ns_data && !isExtendedVersion)
   {
      std::_tstring::size_type cnt = max(ns_strand_str.size(), 7);
      workerB.WriteString(ns_strand_str.c_str(),_T("NS Data"),32,(Int16)cnt,_T("%s"));
   }


   // EXTENDED INFORMATION, IF REQUESTED // 
   if (isExtendedVersion)
   {
      GET_IFACE2(pBroker,ICamber,pCamber);
      GET_IFACE2(pBroker,IProductForces, pProductForces);
      GET_IFACE2(pBroker,ILosses,pLosses);

      pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   	/* 18. ESTIMATED CAMBER IMMEDIATELY BEFORE SLAB CASTING (MAX) */
      value = pCamber->GetDCamberForGirderSchedule( pmid,CREEP_MAXTIME);
      value = IsZero(value) ? 0 : value;

      Float64 initialCamber = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

   	/* 19. DEFLECTION (SLAB AND DIAPHRAGMS)  */
      value = pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftSlab,      pmid, bat, rtCumulative, false )
            + pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftSlabPad,   pmid, bat, rtCumulative, false )
            + pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftDiaphragm, pmid, bat, rtCumulative, false )
            + pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftShearKey,  pmid, bat, rtCumulative, false );
      value = IsZero(value) ? 0 : value;

      Float64 slabDiaphDeflection = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

   	/* 20. DEFLECTION (OVERLAY)  */
      if ( pBridge->HasOverlay() )
      {
         value = pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftOverlay, pmid, bat, rtCumulative, false );
         value = IsZero(value) ? 0 : value;
      }
      else
      {
      value = 0;
      }

      Float64 overlayDeflection = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

   	/* 21. DEFLECTION (OTHER)  */
      value =  pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftTrafficBarrier, pmid, bat, rtCumulative, false );
      value += pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftSidewalk,       pmid, bat, rtCumulative, false );
      value = IsZero(value) ? 0 : value;

      Float64 otherDeflection = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Inch );

   	/* 22. DEFLECTION (TOTAL)  */
      Float64 totalDeflection = slabDiaphDeflection + overlayDeflection + otherDeflection;

   	/* 23. LOSSES (INITIAL)  */
      Float64 aps = pStrandGeometry->GetStrandArea(pmid,releaseIntervalIdx,pgsTypes::Permanent);
      value = pLosses->GetEffectivePrestressLoss(pmid,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::End) * aps;

      Float64 initialLoss = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Kip );

   	/* 24. LOSSES (FINAL)  */
      value = pLosses->GetEffectivePrestressLoss(pmid,pgsTypes::Permanent,lastIntervalIdx,pgsTypes::End) * aps;

      Float64 finalLoss = WBFL::Units::ConvertFromSysUnits( value, WBFL::Units::Measure::Kip );

   	/* 25. Lifting location  */
      GET_IFACE2(pBroker,ISegmentLifting,pLifting);
      Float64 liftLoc = WBFL::Units::ConvertFromSysUnits( pLifting->GetLeftLiftingLoopLocation(segmentKey), WBFL::Units::Measure::Feet );

   	/* 26. Forward handling location  */
      GET_IFACE2(pBroker,ISegmentHauling,pHauling);
      Float64 fwdLoc = WBFL::Units::ConvertFromSysUnits( pHauling->GetLeadingOverhang(segmentKey), WBFL::Units::Measure::Feet );

   	/* 27. Trailing handling location  */
      Float64 trlLoc = WBFL::Units::ConvertFromSysUnits( pHauling->GetTrailingOverhang(segmentKey), WBFL::Units::Measure::Feet );

      /* WRITE TO FILE */
      //==================
	   //----- COL 18 ---- 
      workerB.WriteFloat64(initialCamber,_T("Dinit"),7,6,_T("%6.3f"));
	   //----- COL 19 ---- 
      workerB.WriteFloat64(slabDiaphDeflection,_T("Dslab"),7,6,_T("%6.3f"));
	   //----- COL 20 ---- 
      workerB.WriteFloat64(overlayDeflection,_T("Dolay"),7,6,_T("%6.3f"));
	   //----- COL 21 ---- 
      workerB.WriteFloat64(otherDeflection,_T("Dothr"),7,6,_T("%6.3f"));
	   //----- COL 22 ---- 
      workerB.WriteFloat64(totalDeflection,_T("Dtot "),7,6,_T("%6.3f"));
	   //----- COL 23 ---- 
      workerB.WriteFloat64(initialLoss,_T("LossIn"),8,6,_T("%6.2f"));
	   //----- COL 24 ---- 
      workerB.WriteFloat64(finalLoss,_T("LossFn"),8,6,_T("%6.2f"));
	   //----- COL 25 ---- 
      workerB.WriteFloat64(liftLoc,_T("LiftLc"),8,6,_T("%6.2f"));
	   //----- COL 26 ---- 
      workerB.WriteFloat64(fwdLoc,_T("fwHaul"),8,6,_T("%6.2f"));
	   //----- COL 27 ---- 
      workerB.WriteFloat64(trlLoc,_T("trHaul"),8,6,_T("%6.2f"));

      // rating factors, if enabled
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating))
      {
         std::vector<CGirderKey> girderKeys{ girderKey };
         std::shared_ptr<const pgsISummaryRatingArtifact> pInventoryRatingArtifact = pIArtifact->GetSummaryRatingArtifact(girderKeys, pgsTypes::lrDesign_Inventory, INVALID_INDEX);
         std::shared_ptr<const pgsISummaryRatingArtifact> pOperatingRatingArtifact = pIArtifact->GetSummaryRatingArtifact(girderKeys, pgsTypes::lrDesign_Operating, INVALID_INDEX);

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

         // Service III
         Float64 invStressRF = pInventoryRatingArtifact->GetStressRatingFactor();

         // WriteFloat64 can't handle large values, like Float64_Max which
         // is used for infinite rating factors. In this case, use 10. because
         // we report 10+ when rating factors exceed 10
         invMomRF = (invMomRF == Float64_Max ? 10. : invMomRF);
         invShearRF = (invShearRF == Float64_Max ? 10. : invShearRF);
         oprMomRF = (oprMomRF == Float64_Max ? 10. : oprMomRF);
         oprShearRF = (oprShearRF == Float64_Max ? 10. : oprShearRF);
         invStressRF = (invStressRF == Float64_Max ? 10. : invStressRF);

         workerB.WriteFloat64(invMomRF, _T("RfInMom"), 8, 6, _T("%6.2f"));
         workerB.WriteFloat64(invShearRF, _T("RfInShr"), 8, 6, _T("%6.2f"));
         workerB.WriteFloat64(oprMomRF, _T("RfOpMom"), 8, 6, _T("%6.2f"));
         workerB.WriteFloat64(oprShearRF, _T("RfOpShr"), 8, 6, _T("%6.2f"));
         workerB.WriteFloat64(invStressRF, _T("RfInSts"), 8, 6, _T("%6.2f"));
      }
   }

	// ------ END OF RECORD ----- 
	workerB.WriteToFile(fp);

   // final debond data
   if (!isHarpedDesign)
   {
      writer.WriteFinalData(fp,isExtendedVersion,isIBeam, extraSpacesForSlabOffset);
   }

   if (writer.GetTotalNumDebonds() > 0 && are_harped_bent)
   {
      _ftprintf(fp, _T("Warning: Beam %s in span %s has mixed harped and debonded strands. This is an invalid strand configuration for TxDOT, and is not supported by the TxDOT CAD Data Export feature. Strand data may not be reported correctly.\n"), LABEL_GIRDER(gdrIdx), LABEL_SPAN(spanIdx));
   }

   // Write spec check results data for Test version
   if (is_test_output)
   {
      write_spec_check_results(fp, pBroker, segmentKey, designSucceeded);
      _ftprintf(fp, _T("\n"));
      _ftprintf(fp, _T("\n"));
   }

   return CAD_SUCCESS;
}


void CadWriterWorkerBee::WriteFloat64(Float64 val, LPCTSTR title, Int16 colWidth, Int16 nChars, LPCTSTR format)
{
   // write string to local buffer
   TCHAR buf[32];
   int nr = _stprintf_s(buf, 32, format, val);

   ATLASSERT(nr==nChars);

   this->WriteString(buf, title, colWidth, nChars,_T("%s"));
}

void CadWriterWorkerBee::WriteInt16(Int16 val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format)
{
   // write string to local buffer
   TCHAR buf[32];
   int nr = _stprintf_s(buf, 32, format, val);

   ATLASSERT(nr==nchars);

   this->WriteString(buf, title, colWidth, nchars,_T("%s"));
}

void CadWriterWorkerBee::WriteInt32(Int32 val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format)
{
   // write string to local buffer
   TCHAR buf[32];
   int nr = _stprintf_s(buf, 32, format, val);

   ATLASSERT(nr == nchars);

   this->WriteString(buf, title, colWidth, nchars, _T("%s"));
}



void CadWriterWorkerBee::WriteString(LPCTSTR val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format)
{
   ATLASSERT(nchars<=colWidth);
   ATLASSERT(std::_tstring(title).size()<=colWidth);

   // determine where to write string.
   // Center string in column, biased to right
   Int16 slack = colWidth - nchars;
   Int16 right = slack/2;
   Int16 left = (slack % 2 == 0) ? right : right + 1;
   ATLASSERT(colWidth == left+nchars+right);

   WriteBlankSpacesNoTitle(left);

   int nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   ATLASSERT(nr==nchars);

   m_DataLineCursor += nchars;

   WriteBlankSpacesNoTitle(right);

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, colWidth);
   }
}

void CadWriterWorkerBee::WriteStringEx(LPCTSTR val, LPCTSTR title, Int16 lftPad, Int16 nchars, Int16 rgtPad, LPCTSTR format)
{
   int colWidth = lftPad + nchars + rgtPad;
   ATLASSERT(std::_tstring(title).size()<=colWidth);

   WriteBlankSpacesNoTitle(lftPad);

   int nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   ATLASSERT(nr==nchars);

   m_DataLineCursor += nchars;

   WriteBlankSpacesNoTitle(rgtPad);

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, colWidth);
   }
}

void CadWriterWorkerBee::WriteBlankSpaces(Int16 ns)
{
   for (Int16 is=0; is<ns; is++)
   {
      *(m_DataLineCursor++) = _T(' ');

      if (m_DoWriteTitles)
      {
         *(m_TitleLineCursor++) = _T(' ');
         *(m_DashLineCursor++) = _T('-');
      }
   }

   *m_DataLineCursor = _T('\0');

   if (m_DoWriteTitles)
   {
      *m_TitleLineCursor = _T('\0');
      *m_DashLineCursor = _T('\0');
   }
}

void CadWriterWorkerBee::WriteBlankSpacesNoTitle(Int16 ns)
{
   for (Int16 is=0; is<ns; is++)
   {
      *(m_DataLineCursor++) = _T(' ');
   }

   *m_DataLineCursor = _T('\0');
}

void CadWriterWorkerBee::WriteToFile(FILE* fp)
{
   // Now that we've filled up our buffers, write them to the file
   if (m_DoWriteTitles)
   {
      _ftprintf(fp, _T("%s"), m_TitleLine);
      _ftprintf(fp, _T("\n"));

      _ftprintf(fp, _T("%s"), m_DashLine);
      _ftprintf(fp, _T("\n"));
   }

   _ftprintf(fp, _T("%s"), m_DataLine);
   _ftprintf(fp, _T("\n"));
}

CadWriterWorkerBee::CadWriterWorkerBee(bool doWriteTitles):
m_DoWriteTitles(doWriteTitles)
{
    m_DataLineCursor  = m_DataLine;
    m_TitleLineCursor = m_TitleLine;
    m_DashLineCursor  = m_DashLine;
}

void CadWriterWorkerBee::WriteTitle(LPCTSTR title, Int16 colWidth)
{
   // Write title line and dash line 
   size_t nchars = _tcslen(title);

   // Center string in column, biased to right
   size_t slack = colWidth - nchars;
   size_t right = slack/2;
   size_t left = (slack % 2 == 0) ? right : right + 1;
   ATLASSERT(colWidth == left+nchars+right);

   // left 
   for (size_t is=0; is<left; is++)
   {
      *(m_TitleLineCursor++) = _T(' ');
   }

   // center
   for (size_t is=0; is<nchars; is++)
   {
      *(m_TitleLineCursor++) = *(title++);
   }

   // right
   for (size_t is=0; is<right; is++)
   {
      *(m_TitleLineCursor++) = _T(' ');
   }

   for (size_t is=0; is<(size_t)colWidth; is++)
   {
      *(m_DashLineCursor++) = _T('-');
   }

   *m_TitleLineCursor = _T('\0');
   *m_DashLineCursor = _T('\0');
}

void TestFileWriter::WriteInitialData(CadWriterWorkerBee& workerB)
{
   const Int16 NDBSPCS=43; // width of this debond pattern region

   if (m_NumDebonded > 0)
   {
      // write out debonding data for bottom row
      workerB.WriteInt16((Int16)m_NumDebonded,_T("Ndb"),4,2,_T("%2d"));

      if (m_Rows.empty() || m_OutCome==SectionMismatch || m_OutCome==TooManySections || m_OutCome==SectionsNotSymmetrical)
      {
         // row height, srands in row, and debonds in row are zero
	      workerB.WriteFloat64(0.0,_T("Debnd"),7,5,_T("%5.2f"));
         workerB.WriteInt16(0,_T("   "),6,2,_T("%2d"));
         workerB.WriteInt16(0,_T("   "),6,2,_T("%2d"));

         if (m_Rows.empty())
         {
            // no use searching for nothing
            for (int i=0; i<5; i++)
            {
               workerB.WriteInt16(0,_T("  "),4,2,_T("%2d"));
            }
         }
         else
         {
            // this is an error condition, just right out blanks to fill space
	         //----- COL 11-23 ---- 
               workerB.WriteBlankSpaces(NDBSPCS);
         }
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

         // Where the rubber hits the road - Write first row
         const RowData& row = *(m_Rows.begin());
         WriteRowData(workerB, row, Hg);
      }
   }
   else
   {
      // No debonding. Just write blanks
      workerB.WriteBlankSpaces(NDBSPCS);
   }
}

void TestFileWriter::WriteFinalData(FILE *fp, bool isExtended, bool isIBeam, Int16 extraSpacesForSlabOffset)
{
   // fist write out remaining rows 
   if(!m_Rows.empty())
   {
      pgsPointOfInterest poi(m_SegmentKey, m_GirderLength/2.0);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
   
      GET_IFACE2(pBroker, IIntervals, pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);

      GET_IFACE2(pBroker,ISectionProperties,pSectProp);
      Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poi);

      Int16 nLeadingSpaces;
      if (isIBeam)
      {
         nLeadingSpaces= isExtended ? 97 : 62; // more leading spaces for extended output
      }
      else
      {
         nLeadingSpaces= isExtended ? 96 : 61; // more leading spaces for extended output
      }

      nLeadingSpaces += extraSpacesForSlabOffset; // more spaces if slab offset was printed in first row

      Int16 nrow = 1;
      RowListIter riter = m_Rows.begin();
      riter++;
      while(riter != m_Rows.end())
      {
         const RowData& row = *riter;
         // Only write rows that contain debonding
         if (!row.m_Sections.empty())
         {
            CadWriterWorkerBee workerB(false); // no title lines for last lines

            // leading blank spaces
            workerB.WriteBlankSpaces(nLeadingSpaces);

            WriteRowData(workerB, row, Hg);

	         // ------ END OF RECORD ----- 
            workerB.WriteToFile(fp);
            nrow++;
         }

         riter++;
      }
   }

   SpanIndexType spanIdx = m_SegmentKey.groupIndex;
   GirderIndexType gdrIdx = m_SegmentKey.girderIndex;
   ATLASSERT(m_SegmentKey.segmentIndex == 0);

   // lastly write any information
   if (m_OutCome==SectionMismatch || m_OutCome==SectionsNotSymmetrical)
   {
	   _ftprintf(fp, _T("Warning: Irregular, Non-standard debonding increments used for beam %s in span %s. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
   }
   else if (m_OutCome==TooManySections)
   {
	   _ftprintf(fp, _T("Warning: The number of debonded sections exceeds ten for beam %s in span %s. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
   }
   else if (m_OutCome==NonStandardSection)
   {
      Float64 spac = WBFL::Units::ConvertFromSysUnits(m_SectionSpacing , WBFL::Units::Measure::Feet );
	   _ftprintf(fp, _T("Warning: Non-standard debonding increment of %6.3f ft used  for beam %s in span %s. \n"),spac,LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
   }
}

void TestFileWriter::WriteRowData(CadWriterWorkerBee& workerB, const RowData& row, Float64 Hg) const
{
	//----- COL 11 ----- 
   // elevation of row
   Float64 row_elev = WBFL::Units::ConvertFromSysUnits( Hg + row.m_Elevation, WBFL::Units::Measure::Inch );

   if (m_isUBeam)
   {
      workerB.WriteFloat64(row_elev,_T("Elev "),7,5,_T("%5.2f")); // ubeam needs two digits
   }
   else
   {
      workerB.WriteFloat64(row_elev,_T("Elev "),7,5,_T("%5.1f"));
   }

   // total strands in row
   workerB.WriteInt16((Int16)row.m_NumTotalStrands,_T("Nsr"),6,2,_T("%2d"));

   // num debonded strands in row
   Int16 nsr = CountDebondsInRow(row);
   workerB.WriteInt16(nsr,_T("Ndb"),6,2,_T("%2d"));

	//----- COL 14-23 ---- 
   // we have 5 columns to write no matter what
   SectionListConstIter scit = row.m_Sections.begin();

   TCHAR buff[4];
   for (Int16 icol=0; icol<5; icol++)
   {
      Int16 db_cnt = 0;

      if (scit!= row.m_Sections.end())
      {
         const SectionData rdata = *scit;
         Float64 row_loc = (icol+1)*m_SectionSpacing;

         if (IsEqual(row_loc, rdata.m_XLoc))
         {
            db_cnt = rdata.m_NumDebonds;
            scit++;
         }
      }

      _stprintf_s(buff,sizeof(buff)/sizeof(TCHAR),_T("%2d"),icol+1);

      workerB.WriteInt16(db_cnt,buff,4,2,_T("%2d"));
   }

   ATLASSERT(scit==row.m_Sections.end()); // we didn't find all of our sections - bug
}


void write_spec_check_results(FILE *fp, IBroker* pBroker, const CGirderKey& girderKey, bool designSucceeded)
{
#if defined _DEBUG
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument());
#endif

   _ftprintf(fp, _T("\n\n"));

   if (!designSucceeded)
   {
      _ftprintf(fp, _T("%s\n"), _T("Girder design was Not Successful"));
   }

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   if( pGirderArtifact->Passed() )
   {
      _ftprintf(fp, _T("%s\n"), _T("The Specification Check was Successful"));
   }
   else
   {
      _ftprintf(fp, _T("%s\n"), _T("The Specification Check was Not Successful"));
     
      GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
      bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

      // Build a list of our failures
      FailureList failures;

      // Allowable stress checks
      ListStressFailures(pBroker,failures,pGirderArtifact,false);

      // Moment Capacity Checks
      ListMomentCapacityFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListMomentCapacityFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      //Stirrup Checks
      ListVerticalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListVerticalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      ListHorizontalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListHorizontalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      ListStirrupDetailingFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListStirrupDetailingFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      ListDebondingFailures(pBroker,failures,pGirderArtifact);
      ListSplittingZoneFailures(pBroker,failures,pGirderArtifact);
      ListConfinementZoneFailures(pBroker,failures,pGirderArtifact);
      ListVariousFailures(pBroker,failures,pGirderArtifact,false);

      // Put failures into report
      for (FailureListIterator it=failures.begin(); it!=failures.end(); it++)
      {
         _ftprintf(fp, _T("%s\n"), it->c_str());
      }
   }
}


int Test_WriteDistributionFactorsToFile (FILE *fp, IBroker* pBroker, const CGirderKey& girderKey)
{
#if defined _DEBUG
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument());
#endif

   SpanIndexType spanIdx = girderKey.groupIndex;
   GirderIndexType gdrIdx = girderKey.girderIndex;
   CSpanKey spanKey(spanIdx,gdrIdx);

   GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pDfEng);

   Float64 gpM, gpM1, gpM2;  // pos moment
   Float64 gnM, gnM1, gnM2;  // neg moment, ahead face
   Float64 gV,  gV1,  gV2;   // shear

   pDfEng->GetDFResultsEx(spanKey,pgsTypes::StrengthI,
                          &gpM, &gpM1, &gpM2,
                          &gnM, &gnM1, &gnM2,
                          &gV,  &gV1,  &gV2);

	TCHAR	spanNumber[5+1];
	_stprintf_s(spanNumber, sizeof(spanNumber)/sizeof(TCHAR), _T("%s"), LABEL_SPAN(spanIdx));

	TCHAR  beamNumber[5+1];
	_stprintf_s(beamNumber, sizeof(beamNumber)/sizeof(TCHAR), _T("%s"), LABEL_GIRDER(gdrIdx));

   // have our data, now need to write it
   CadWriterWorkerBee workerB(true);

   workerB.WriteString(spanNumber,_T("Span "),7,5,_T("%5s"));
   workerB.WriteString(beamNumber,_T(" Gdr "),7,5,_T("%5s"));

   workerB.WriteFloat64(gpM, _T(" gpM  "),8,6,_T("%6.3f"));
   workerB.WriteFloat64(gpM1,_T(" gpM1 "),8,6,_T("%6.3f"));
   workerB.WriteFloat64(gpM2,_T(" gpM2 "),8,6,_T("%6.3f"));

   workerB.WriteFloat64(gnM, _T(" gnM  "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(gnM1,_T(" gnM1 "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(gnM2,_T(" gnM2 "),8,6,_T("%6.2f"));

   workerB.WriteFloat64(gV, _T("  gV  "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(gV1,_T(" gV1  "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(gV2,_T(" gV2  "),8,6,_T("%6.2f"));

   workerB.WriteToFile(fp);

   _ftprintf(fp, _T("\n"));

   return CAD_SUCCESS;
}

std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi)
{
   StrandRowUtil::StrandRowSet strandrows = StrandRowUtil::GetStrandRowSet(pBroker, midPoi);

   // At this point, we have counted the number of strands per row. Now create string
   bool first = true;
   std::_tostringstream os;
   for (StrandRowUtil::StrandRowIter srit=strandrows.begin(); srit!=strandrows.end(); srit++)
   {
      if (!first)
         os<<_T(",");
      else
         first=false;

      const StrandRowUtil::StrandRow& srow = *srit;
      Float64 elev_in = RoundOff(WBFL::Units::ConvertFromSysUnits( srow.Elevation, WBFL::Units::Measure::Inch ),0.001);
      os<<elev_in<<_T("(")<<srow.Count<<_T(")");
   }

   return os.str();
}

