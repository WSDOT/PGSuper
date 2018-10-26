///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "TxDOTOptionalDesignUtilities.h"

#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\MomentCapacity.h>
#include <IFace\DistributionFactors.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\DistFactorEngineer.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\GirderArtifactTool.h>
#include <PgsExt\GirderLabel.h>

static void write_spec_check_results(FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, bool designSucceeded);
static std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi);

// Return fractional string for strand size
static int txdString_ftofrac	/* <=  Completion value                   */
(
LPTSTR      stringP,		      /* <=  Output text string                 */
size_t      size,             /* <= size of output string               */
Float64		value 			   /*  => Value to convert                   */
)
{
    if(value>1.0 || value<0.0)
    {
        ATLASSERT(0); // we don't deal with more than an inch
        _stprintf_s(stringP, 4, _T("Err "));
        return CAD_FAIL;
    }

    // See if we can resolve to 1/16th's
    const Float64 stinkth = 1.0/16;
    Float64 mod16 = fmod(value, stinkth);
    if (mod16 > 1.0e-05)
    {
        // Not a 16th - Print decimal value
        _stprintf_s(stringP, size, _T("%4.2f"),value);
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

        _stprintf_s(stringP, size, _T("%d/%-d"), num, den);

        int a = 0;
    }

	return CAD_SUCCESS;
}



int TxDOT_WriteCADDataToFile (FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, TxDOTCadExportFormatType format, bool designSucceeded)
{
// Get data first and convert to correct units. then write it all at end of function
// Note that Units are hard-coded into this routine. TxDOT has no use for SI units
	TCHAR	charBuffer[32];
   Float64 value;


	/* Regenerate bridge data */
	GET_IFACE2(pBroker, IArtifact, pIArtifact);
 	const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span, gdr);
   if(!(pGdrArtifact->Passed()))
	{
//		AfxMessageBox(_T("The Specification Check was NOT Successful"),MB_OK);
	}

	/* Interfaces to all relevant agents */
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker, IGirderData, pGirderData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, ISectProp2, pSectProp2);
	GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2(pBroker, ILibrary,pLibrary);
   GET_IFACE2(pBroker, IMomentCapacity, pMomentCapacity);
   GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pDistFact);
   GET_IFACE2(pBroker, IBridgeMaterial, pMaterial);
   GET_IFACE2(pBroker,IProjectProperties,pProjectProperties);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Use workerbee class to do actual writing of data
   bool is_test_output = (format== tcxTest) ? true : false;
   CadWriterWorkerBee workerB(is_test_output);//

	/* Create pois at the start of girder and mid-span */
	pgsPointOfInterest pois(span, gdr, 0.0);
	std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(span, gdr, pgsTypes::BridgeSite1, POI_MIDSPAN);
	CHECK(pmid.size() == 1);

   // IGirders are treated differently than others
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);
   std::_tstring girderFamily = pGirderEntry->GetGirderFamilyName();

   bool isIBeam = girderFamily == _T("I-Beam");

   // Determine type of output and number of strands
   bool isHarpedDesign = !pStrandGeometry->GetAreHarpedStrandsForcedStraight(span,gdr) &&
                        0 < pStrandGeometry->GetMaxStrands(span, gdr, pgsTypes::Harped) &&
                        isIBeam;

   StrandIndexType harpedCount   = pStrandGeometry->GetNumStrands(span, gdr,pgsTypes::Harped);
   StrandIndexType straightCount = pStrandGeometry->GetNumStrands(span, gdr,pgsTypes::Straight);

   bool isExtendedVersion = (format==tcxExtended || format==tcxTest);

   const CGirderData* pgirderData = pGirderData->GetGirderData(span, gdr);

   // Determine if non-standard design
   bool do_write_ns_data = isHarpedDesign && 
                           (pgirderData->PrestressData.GetNumPermStrandsType() != NPS_TOTAL_NUMBER) && 
                           (harpedCount+straightCount > 0);

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

      Float64 roadwayWidth = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   /* 0b. NUMBER OF BEAMS */
      GirderIndexType nGirders = pBridge->GetGirderCount(span);

	   /* 0a. BEAM SPACING */
      ATLASSERT( pBridgeDesc->GetGirderSpacingType() != pgsTypes::sbsGeneral );
      GirderIndexType spaceIdx = (gdr == nGirders-1 ? nGirders-2 : gdr);
      value = pBridgeDesc->GetSpan(span)->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(spaceIdx);

      Float64 girderSpacing = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   //----- COL 0a ---- 
	   workerB.WriteFloat64(roadwayWidth,_T("RoadW"),7,5,_T("%5.2f"));
	   //----- COL 0b ----- 
	   workerB.WriteInt16((Int16)nGirders,_T("Ng "),5,3,_T("%3d"));
	   //----- COL 0c ----- 
	   workerB.WriteFloat64(girderSpacing,_T("Spcng"),7,5,_T("%5.2f"));
   }


	/* 1. SPAN NUMBER */
	TCHAR	spanNumber[4+1];
	_stprintf_s(spanNumber, sizeof(spanNumber)/sizeof(TCHAR), _T("%d"), LABEL_SPAN(span));

	/* 1. GIRDER NUMBER */
	TCHAR  beamNumber[4+1];
	_stprintf_s(beamNumber, sizeof(beamNumber)/sizeof(TCHAR), _T("%s"), LABEL_GIRDER(gdr));

	/* 3. BEAM TYPE */
	TCHAR beamType[5+1];
   beamType[5] = _T('\0');
#if defined _UNICODE
   wmemset( beamType,_T(' '),5);
#else
   memset( beamType,' ',5);
#endif
   std::_tstring str = pIBridgeDesc->GetBridgeDescription()->GetSpan(span)->GetGirderTypes()->GetGirderName(gdr);

   // assume that last contiguous string is type
   size_t start = str.rfind(_T(" "));
   str.erase(0,start+1);
   size_t cnt = min(5, str.length());

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
	TCHAR  strandPat[5+1]; 
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
   const matPsStrand* strandMatP = pGirderData->GetStrandMaterial(span, gdr,pgsTypes::Permanent);
   value = strandMatP->GetNominalDiameter();
   value = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* Convert value to fraction representation */
	txdString_ftofrac (charBuffer, sizeof(charBuffer)/sizeof(TCHAR), value); 
	_tcscpy_s(strandSize, sizeof(strandSize)/sizeof(TCHAR), charBuffer);

   /* 7. STRAND STRENGTH */
	int strandStrength = (strandMatP->GetGrade() == matPsStrand::Gr1725 ?  250 :  270);

	/* 8. STRAND ECCENTRICITY AT CENTER LINE */
   Float64 nEff;
	value = pStrandGeometry->GetEccentricity( pmid[0], false, &nEff );

	Float64 strandEccCL = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* 9. STRAND ECCENTRICITY AT END */
   value = pStrandGeometry->GetEccentricity( pois, false, &nEff );

	Float64 strandEccEnd = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* 12. CONCRETE RELEASE STRENGTH */
   value = pMaterial->GetFciGdr(span, gdr);

	Float64 concreteRelStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 13. MINIMUM 28 DAY COMP. STRENGTH */
	value = pMaterial->GetFcGdr(span, gdr);

	Float64 min28dayCompStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 14. DESIGN LOAD COMPRESSIVE STRESS (TOP CL) */ 
   const pgsFlexuralStressArtifact* pArtifact;
   Float64 fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;

   pArtifact = pGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,
	                                                     pgsTypes::ServiceI,pgsTypes::Compression,pmid[0].GetDistFromStart()) );
   pArtifact->GetExternalEffects( &fcTop, &fcBot );
	value = -fcTop;

	Float64 designLoadCompStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 15. DESIGN LOAD TENSILE STRESS (BOT CL) */
   pArtifact = pGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pmid[0].GetDistFromStart()) );
   pArtifact->GetExternalEffects( &ftTop, &ftBot );
	value = -ftBot;

	Float64 designLoadTensileStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

   /* 16. REQUIRED MINIMUM ULTIMATE MOMENT CAPACITY */
   MINMOMENTCAPDETAILS mmcd;
   pMomentCapacity->GetMinMomentCapacityDetails(pgsTypes::BridgeSite3,pmid[0],true,&mmcd);
   value = max(mmcd.Mu,mmcd.MrMin);

	int reqMinUltimateMomentCapacity = (int)Round(::ConvertFromSysUnits( value, unitMeasure::KipFeet ));

	/* 17. LIVE LOAD DISTRIBUTION FACTOR */
   Float64 momentDistFactor = pDistFact->GetMomentDistFactor(span, gdr, pgsTypes::StrengthI);

	/* 17aa. LIVE LOAD DISTRIBUTION FACTOR */
   Float64 shearDistFactor = pDistFact->GetShearDistFactor(span, gdr, pgsTypes::StrengthI);

   /* 17a - Non-Standard Design Data */
   std::_tstring ns_strand_str;
   if (do_write_ns_data && !isExtendedVersion)
   {
      ns_strand_str = MakeNonStandardStrandString(pBroker,pmid[0]);
   }

   // WRITE DATA TO OUTPUT FILE
   workerB.WriteString(bridgeName.c_str(),_T("STRUCTURE"), 16, (Int16)bridgeName.size(),_T("%s"));
	//----- COL 1 ----- 
   workerB.WriteString(spanNumber,_T("Span"),7,3,_T("%-3s"));
	//----- COL 2 ----- 
   workerB.WriteString(beamNumber,_T(" Gdr"),7,4,_T("%-4s"));
	//----- COL 3 ----- 
   workerB.WriteString(beamType,_T("Type "),7,5,_T("%-5s"));
	//----- COL 4 ----- 
   workerB.WriteString(strandPat,_T("N"),6,1,_T("%1s"));
	//----- COL 5 ----- 
   workerB.WriteInt16((Int16)strandNum,_T("Ns"),6,2,_T("%2d"));
	//----- COL 6 ----- 
   workerB.WriteString(strandSize,_T("Size"),5,3,_T("%3s"));
	//----- COL 7 ----- 
   workerB.WriteInt16(strandStrength,_T("Strn"),5,3,_T("%3d"));
	//----- COL 8 ----- 
   workerB.WriteFloat64(strandEccCL,_T("EccCL"),7,5,_T("%5.2f"));
	//----- COL 9 ----- 
   workerB.WriteFloat64(strandEccEnd,_T("EccEn"),7,5,_T("%5.2f"));

   Float64 girder_length = pBridge->GetGirderLength(span, gdr);

   // create debond writer in case we need it
   TxDOTCadWriter writer(span, gdr, girder_length, pStrandGeometry);

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

      // Determine if harped strands are straight by comparing harped eccentricity at end/mid
      bool are_harped_straight(true);
      if (harpedCount>0)
      {
         Float64 nEff;
         Float64 hs_ecc_end = pStrandGeometry->GetHsEccentricity(pois, &nEff);
         Float64 hs_ecc_mid = pStrandGeometry->GetHsEccentricity(pmid[0], &nEff);
         are_harped_straight = IsEqual(hs_ecc_end, hs_ecc_mid);
      }

      if(are_harped_straight)
      {
         // Report harped strands as straight
         dstrandNum = 0;
         dstrandToEnd = 0.0;
         dstrandToCL = 0.0;
      }
      else
      {
         dstrandNum = harpedCount;

         pStrandGeometry->GetHighestHarpedStrandLocationEnds(span, gdr, &value);
         dstrandToEnd = ::ConvertFromSysUnits( value, unitMeasure::Inch );

         pStrandGeometry->GetHighestHarpedStrandLocationCL(span, gdr, &value);
         dstrandToCL = ::ConvertFromSysUnits( value, unitMeasure::Inch );
      }

      // output
	   //----- COL 10 ---- 
      workerB.WriteInt16((Int16)dstrandNum,_T("Nh"),4,2,_T("%2d"));
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

      if (isIBeam)
      {
         // Empty spaces in IGND
         workerB.WriteBlankSpaces(14);
      }
   }

   // onward with common data for harped or debond designs
	//----- COL 12 ---- 
   workerB.WriteFloat64(concreteRelStrength,_T(" Fci  "),7,6,_T("%6.3f"));
	//----- COL 13 ---- 
   workerB.WriteFloat64(min28dayCompStrength,_T(" Fc   "),7,6,_T("%6.3f"));
	//----- COL 14 ---- 
   workerB.WriteFloat64(designLoadCompStress,_T(" fcomp"),10,6,_T("%6.3f"));
	//----- COL 15 ---- 
   workerB.WriteFloat64(designLoadTensileStress,_T(" ftens "),10,6,_T("%6.3f"));

	//----- COL 16 ---- 
   workerB.WriteInt16(reqMinUltimateMomentCapacity,_T("ultMo"),9,5,_T("%5d"));
	//----- COL 17 ---- 
   workerB.WriteFloat64(momentDistFactor,_T("LLDFm"),7,5,_T("%5.3f"));
	//----- COL 17aa ---- 
   workerB.WriteFloat64(shearDistFactor,_T("LLDFs"),7,5,_T("%5.3f"));

   if (do_write_ns_data && !isExtendedVersion)
   {
      std::_tstring::size_type cnt = max(ns_strand_str.size(), 7);
      workerB.WriteString(ns_strand_str.c_str(),_T("NS Data"),10,(Int16)cnt,_T("%s"));
   }

   // EXTENDED INFORMATION, IF REQUESTED // 
   if (isExtendedVersion)
   {
      GET_IFACE2(pBroker,ICamber,pCamber);
      GET_IFACE2(pBroker,IProductForces, pProductForces);
      GET_IFACE2( pBroker, ISpecification, pSpec );
      GET_IFACE2(pBroker,ILosses,pLosses);

      pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
      BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   	/* 18. ESTIMATED CAMBER IMMEDIATELY BEFORE SLAB CASTING (MAX) */
      value = pCamber->GetDCamberForGirderSchedule( pmid[0],CREEP_MAXTIME);

      Float64 initialCamber = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 19. DEFLECTION (SLAB AND DIAPHRAGMS)  */
      value = pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftSlab,      pmid[0], SimpleSpan )
            + pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftSlabPad, pmid[0], SimpleSpan )
            + pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftDiaphragm, pmid[0], SimpleSpan )
            + pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftShearKey,  pmid[0], SimpleSpan );

      Float64 slabDiaphDeflection = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 20. DEFLECTION (OVERLAY)  */
      pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

      value = pProductForces->GetDisplacement(overlay_stage, pftOverlay, pmid[0], bat );

      Float64 overlayDeflection = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 21. DEFLECTION (OTHER)  */
      value =  pProductForces->GetDisplacement(pgsTypes::BridgeSite2, pftTrafficBarrier, pmid[0], bat );
      value += pProductForces->GetDisplacement(pgsTypes::BridgeSite2, pftSidewalk,       pmid[0], bat );

      Float64 otherDeflection = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 22. DEFLECTION (TOTAL)  */
      Float64 totalDeflection = slabDiaphDeflection + overlayDeflection + otherDeflection;

   	/* 23. LOSSES (INITIAL)  */
      Float64 aps = pStrandGeometry->GetAreaPrestressStrands(span,gdr,false);
      value = pLosses->GetAfterXferLosses(pmid[0],pgsTypes::Permanent) * aps;

      Float64 initialLoss = ::ConvertFromSysUnits( value, unitMeasure::Kip );

   	/* 24. LOSSES (FINAL)  */
      value = pLosses->GetFinal(pmid[0],pgsTypes::Permanent) * aps;

      Float64 finalLoss = ::ConvertFromSysUnits( value, unitMeasure::Kip );

   	/* 25. Lifting location  */
      const CHandlingData& handlingData = pgirderData->HandlingData;
      Float64 liftLoc = ::ConvertFromSysUnits( handlingData.LeftLiftPoint, unitMeasure::Feet );

   	/* 26. Forward handling location  */
      Float64 fwdLoc = ::ConvertFromSysUnits( handlingData.LeadingSupportPoint, unitMeasure::Feet );

   	/* 27. Trailing handling location  */
      Float64 trlLoc = ::ConvertFromSysUnits( handlingData.TrailingSupportPoint, unitMeasure::Feet );

      /* WRITE TO FILE */
      //==================
	   //----- COL 18 ---- 
      workerB.WriteFloat64(initialCamber,_T("Dinit"),7,5,_T("%5.2f"));
	   //----- COL 19 ---- 
      workerB.WriteFloat64(slabDiaphDeflection,_T("Dslab"),7,5,_T("%5.2f"));
	   //----- COL 20 ---- 
      workerB.WriteFloat64(overlayDeflection,_T("Dolay"),7,5,_T("%5.2f"));
	   //----- COL 21 ---- 
      workerB.WriteFloat64(otherDeflection,_T("Dothr"),7,5,_T("%5.2f"));
	   //----- COL 22 ---- 
      workerB.WriteFloat64(totalDeflection,_T("Dtot "),7,5,_T("%5.2f"));
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
   }

	// ------ END OF RECORD ----- 
	workerB.WriteToFile(fp);

   // final debond data
   if (!isHarpedDesign)
   {
      writer.WriteFinalData(fp, isExtendedVersion, isIBeam);
   }

   // Write spec check results data for Test version
   if (is_test_output)
   {
      write_spec_check_results(fp, pBroker, span, gdr,designSucceeded);
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

   for (size_t is=0; is<colWidth; is++)
   {
      *(m_DashLineCursor++) = _T('-');
   }

   *m_TitleLineCursor = _T('\0');
   *m_DashLineCursor = _T('\0');
}

void TxDOTCadWriter::WriteInitialData(CadWriterWorkerBee& workerB)
{
   // first build our data structure
   Compute();

   // next write out data
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
         workerB.WriteBlankSpaces(20);
      }
   }
   else
   {
      // A little checking
      pgsPointOfInterest poi(m_Span,m_Girder,m_GirderLength/2);
      RowIndexType nrs = m_pStrandGeometry->GetNumRowsWithStrand(poi,pgsTypes::Straight);
      ATLASSERT((RowIndexType)m_Rows.size() == nrs); // could have more rows than rows with debonded strands

      // Where the rubber hits the road - Write first row
      const RowData& row = *(m_Rows.begin());
      WriteRowData(workerB, row);
   }
}

void TxDOTCadWriter::WriteFinalData(FILE *fp, bool isExtended, bool isIBeam)
{
   // fist write out remaining rows 
   if(!m_Rows.empty())
   {
      Int16 nLeadingSpaces;
      if (isIBeam)
      {
         nLeadingSpaces= isExtended ? 97 : 78; // more leading spaces for extended output
      }
      else
      {
         nLeadingSpaces= isExtended ? 96 : 77; // more leading spaces for extended output
      }

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

            WriteRowData(workerB, row);

	         // ------ END OF RECORD ----- 
            workerB.WriteToFile(fp);
            nrow++;
         }

         riter++;
      }
   }

   // lastly write any information
   if (m_OutCome==SectionMismatch || m_OutCome==SectionsNotSymmetrical)
   {
	   _ftprintf(fp, _T("Warning: Irregular, Non-standard debonding increments used for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(m_Girder),LABEL_SPAN(m_Span));
   }
   else if (m_OutCome==TooManySections)
   {
	   _ftprintf(fp, _T("Warning: The number of debonded sections exceeds ten for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(m_Girder),LABEL_SPAN(m_Span));
   }
   else if (m_OutCome==NonStandardSection)
   {
      Float64 spac = ::ConvertFromSysUnits(m_SectionSpacing , unitMeasure::Feet );
	   _ftprintf(fp, _T("Warning: Non-standard debonding increment of %6.3f ft used  for beam %s in span %2d. \n"),spac,LABEL_GIRDER(m_Girder),LABEL_SPAN(m_Span));
   }
}

void TxDOTCadWriter::WriteRowData(CadWriterWorkerBee& workerB, const RowData& row) const
{
   // elevation of row
   Float64 row_elev = ::ConvertFromSysUnits( row.m_Elevation, unitMeasure::Inch );

   workerB.WriteFloat64(row_elev,_T("Elev "),7,5,_T("%5.2f"));

   // total strands in row
   workerB.WriteInt16((Int16)row.m_NumTotalStrands,_T("Nsr"),6,2,_T("%2d"));

   // num debonded strands in row
   Int16 nsr = CountDebondsInRow(row);
   workerB.WriteInt16(nsr,_T("Ndb"),6,2,_T("%2d"));

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


void write_spec_check_results(FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, bool designSucceeded)
{
   _ftprintf(fp, _T("\n\n"));

   if (!designSucceeded)
   {
      _ftprintf(fp, _T("%s\n"), _T("Girder design was Not Successful"));
   }

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,gdr);

   if( pArtifact->Passed() )
   {
      _ftprintf(fp, _T("%s\n"), _T("The Specification Check was Successful"));
   }
   else
   {
      _ftprintf(fp, _T("%s\n"), _T("The Specification Check was Not Successful"));
     
      GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
      bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, gdr);

      // Build a list of our failures
      FailureList failures;

      // Allowable stress checks
      list_stress_failures(pBroker,failures,span,gdr,pArtifact,false);

      // Moment Capacity Checks
      list_momcap_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_momcap_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      //Stirrup Checks
      list_vertical_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_vertical_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      list_horizontal_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_horizontal_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      list_stirrup_detailing_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_stirrup_detailing_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      list_debonding_failures(pBroker,failures,span,gdr,pArtifact);
      list_splitting_zone_failures(pBroker,failures,span,gdr,pArtifact);
      list_confinement_zone_failures(pBroker,failures,span,gdr,pArtifact);

      list_various_failures(pBroker,failures,span,gdr,pArtifact,false);

      // Put failures into report
      for (FailureListIterator it=failures.begin(); it!=failures.end(); it++)
      {
         _ftprintf(fp, _T("%s\n"), it->c_str());
      }
   }
}


int TxDOT_WriteDistributionFactorsToFile (FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr)
{

   GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pDfEng);

   Float64 gpM, gpM1, gpM2;  // pos moment
   Float64 gnM, gnM1, gnM2;  // neg moment, ahead face
   Float64 gV,  gV1,  gV2;   // shear
   Float64 gR,  gR1,  gR2;   // reaction

   pDfEng->GetDFResultsEx(span,gdr,pgsTypes::StrengthI,
                          &gpM, &gpM1, &gpM2,
                          &gnM, &gnM1, &gnM2,
                          &gV,  &gV1,  &gV2,
                          &gR,  &gR1,  &gR2 );

	TCHAR	spanNumber[5+1];
	_stprintf_s(spanNumber, sizeof(spanNumber)/sizeof(TCHAR), _T("%d"), LABEL_SPAN(span));

	TCHAR  beamNumber[5+1];
	_stprintf_s(beamNumber, sizeof(beamNumber)/sizeof(TCHAR), _T("%s"), LABEL_GIRDER(gdr));

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

   workerB.WriteFloat64(gR, _T("  gR  "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(gR1,_T(" gR1  "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(gR2,_T(" gR2  "),8,6,_T("%6.2f"));

   workerB.WriteToFile(fp);

   _ftprintf(fp, _T("\n"));

   return CAD_SUCCESS;
}

std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi)
{
   OptionalDesignHarpedFillUtil::StrandRowSet strandrows = OptionalDesignHarpedFillUtil::GetStrandRowSet(pBroker, midPoi);

   // At this point, we have counted the number of strands per row. Now create string
   bool first = true;
   std::_tostringstream os;
   for (OptionalDesignHarpedFillUtil::StrandRowIter srit=strandrows.begin(); srit!=strandrows.end(); srit++)
   {
      if (!first)
         os<<_T(", ");
      else
         first=false;

      const OptionalDesignHarpedFillUtil::StrandRow& srow = *srit;
      Float64 elev_in = RoundOff(::ConvertFromSysUnits( srow.Elevation, unitMeasure::Inch ),0.001);
      os<<elev_in<<_T("(")<<srow.fillListString<<_T(")");
   }

   return os.str();
}

//////// TOGA Report
int TxDOT_WriteTOGAReportToFile (FILE *fp, IBroker* pBroker)
{
   // Use our worker bee to write results
   CadWriterWorkerBee workerB(true);

   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);
   GET_IFACE2(pBroker,IGetTogaData,pGetTogaData);
   const CTxDOTOptionalDesignData* pProjectData = pGetTogaData->GetTogaData();

   // Compressive stress - top
   Float64 stress_val_calc, stress_fac, stress_loc;
   pGetTogaResults->GetControllingCompressiveStress(&stress_val_calc, &stress_fac, &stress_loc);

   Float64 stress_val_input = ::ConvertFromSysUnits( pProjectData->GetFt(), unitMeasure::KSI );
   stress_val_calc = ::ConvertFromSysUnits( -stress_val_calc, unitMeasure::KSI );

   workerB.WriteFloat64(stress_val_input, _T("ftinp "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_val_calc, _T("ftcalc"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_fac, _T("ftfact"),8,6,_T("%6.2f"));

   // Tensile stress - bottom
   pGetTogaResults->GetControllingTensileStress(&stress_val_calc, &stress_fac, &stress_loc);

   stress_val_input = ::ConvertFromSysUnits( pProjectData->GetFb(), unitMeasure::KSI );
   stress_val_calc = ::ConvertFromSysUnits( -stress_val_calc, unitMeasure::KSI );

   workerB.WriteFloat64(stress_val_input, _T("fbinp "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_val_calc, _T("fbcalc"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_fac, _T("fbfact"),8,6,_T("%6.2f"));

   // Ultimate moment
   Float64 mu_input = ::ConvertFromSysUnits( pProjectData->GetMu(), unitMeasure::KipFeet);
   Float64 mu_orig  = ::ConvertFromSysUnits( pGetTogaResults->GetRequiredUltimateMoment(), unitMeasure::KipFeet );
   Float64 mu_fabr  = ::ConvertFromSysUnits( pGetTogaResults->GetUltimateMomentCapacity(), unitMeasure::KipFeet );

   workerB.WriteFloat64(mu_input,_T(" muinp  "),10,8,_T("%8.2f"));
   workerB.WriteFloat64(mu_orig, _T(" muorig "),10,8,_T("%8.2f"));
   workerB.WriteFloat64(mu_fabr, _T(" mufabr "),10,8,_T("%8.2f"));

   // Required concrete strengths
   Float64 input_fci = ::ConvertFromSysUnits(pProjectData->GetPrecasterDesignGirderData()->GetFci(), unitMeasure::KSI );
   Float64 reqd_fci  = ::ConvertFromSysUnits(pGetTogaResults->GetRequiredFci(), unitMeasure::KSI );

   workerB.WriteFloat64(input_fci,_T("fciinp"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(reqd_fci, _T("fcireq"),8,6,_T("%6.2f"));

   Float64 input_fc = ::ConvertFromSysUnits(pProjectData->GetPrecasterDesignGirderData()->GetFc(), unitMeasure::KSI );
   Float64 reqd_fc =  ::ConvertFromSysUnits(pGetTogaResults->GetRequiredFc(), unitMeasure::KSI );

   workerB.WriteFloat64(input_fc,_T("fc inp"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(reqd_fc, _T("fc req"),8,6,_T("%6.2f"));

   // Camber
   Float64 cbr_orig = ::ConvertFromSysUnits(pGetTogaResults->GetMaximumCamber(), unitMeasure::Feet );
   Float64 cbr_fabr = ::ConvertFromSysUnits(pGetTogaResults->GetFabricatorMaximumCamber(), unitMeasure::Feet );

   workerB.WriteFloat64(cbr_orig,_T("cbr orig"),10,8,_T("%8.4f"));
   workerB.WriteFloat64(cbr_fabr,_T("cbr fabr"),10,8,_T("%8.4f"));

   // Shear check
   bool passed = pGetTogaResults->ShearPassed();
   workerB.WriteString(passed?_T("Ok\0"):_T("Fail\n"),_T("Shear"),8,7,_T("%7s"));

   workerB.WriteToFile(fp);

   _ftprintf(fp, _T("\n"));

   return CAD_SUCCESS;
}
