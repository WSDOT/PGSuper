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
#include <IFace\GirderHandling.h>
#include <IFace\Intervals.h>

#if defined _DEBUG
#include <IFace\DocumentType.h>
#endif

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderArtifactTool.h>
#include <PgsExt\GirderLabel.h>

static void write_spec_check_results(FILE *fp, IBroker* pBroker, const CGirderKey& girderKey, bool designSucceeded);
static std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi);

// Return string for strand size
static int txdString_ftofrac	/* <=  Completion value                   */
(
LPTSTR      stringP,		      /* <=  Output text string                 */
size_t      size,             /* <= size of output string               */
Float64		value,			   /*  => Value to convert                   */
Float64		resolution		   /*  => Fractional resolution              */
)
{
	Float64	fraction = 0.0, whole = 0.0;
	int		index = 0;
	TCHAR table[][4] = {_T(" "),_T("1/8"),_T("1/4"),_T("3/8"),_T("1/2"),_T("5/8"),_T("3/4"),_T("7/8")};

	/* Validate arguments */
	if (stringP == NULL) return (ERROR);
	stringP[0] = 0;
	resolution = 0.125;	// temp

	/* Break number into whole & fraction */
	fraction = modf (value, &whole);	
	
	/* Create output string */
	if (whole > 0) _stprintf_s(stringP, size, _T("%.0lf "),whole);

	/* Apply resolution to fraction */
	index = (int)((fraction + (resolution / 2.0)) / resolution);

	/* Append fraction string */
	_tcscat_s(stringP,size, table[index]);

	return (CAD_SUCCESS);
}
	


int TxDOT_WriteCADDataToFile (FILE *fp, IBroker* pBroker, const CGirderKey& girderKey, TxDOTCadExportFormatType format, bool designSucceeded)
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
   GET_IFACE2(pBroker, IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker, ISegmentData,pSegmentData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
	GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2(pBroker, ILibrary,pLibrary);
   GET_IFACE2(pBroker, IMomentCapacity, pMomentCapacity);
   GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pDistFact);
   GET_IFACE2(pBroker, IMaterials, pMaterial);
   GET_IFACE2(pBroker, IIntervals, pIntervals);

   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval(segmentKey);

   // Use workerbee class to do actual writing of data
   bool is_test_output = (format== tcxTest) ? true : false;
   CadWriterWorkerBee workerB(is_test_output);//

   // Determine type of output and number of strands
   bool isHarpedDesign = !pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey) &&
                        0 < pStrandGeometry->GetMaxStrands(segmentKey, pgsTypes::Harped);

	/* Create pois at the start of girder and mid-span */
   pgsPointOfInterest pois(segmentKey, 0.0);
	std::vector<pgsPointOfInterest> pmid( pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT) );
	ATLASSERT(pmid.size() == 1);


   bool isExtendedVersion = (format==tcxExtended || format==tcxTest);

   // extended version writes data at front and back of line
   if (isExtendedVersion)
   {
	   /* 0a. ROADWAY WIDTH */
      value = pBridge->GetCurbToCurbWidth(0.00);

      Float64 roadwayWidth = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   /* 0b. NUMBER OF BEAMS */
      GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);

	   /* 0a. BEAM SPACING */
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      ATLASSERT( pBridgeDesc->GetGirderSpacingType() != pgsTypes::sbsGeneral );
      GirderIndexType spaceIdx = (segmentKey.girderIndex == nGirders-1 ? nGirders-2 : segmentKey.girderIndex);
      value = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing(spaceIdx);

      Float64 girderSpacing = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   //----- COL 0a ---- 
	   workerB.WriteFloat64(roadwayWidth,_T("RoadW"),5,_T("%5.2f"),true);
	   //----- COL 0b ----- 
	   workerB.WriteInt16((Int16)nGirders,_T("Ng "),3,_T("%3d"),true);
	   //----- COL 0c ----- 
	   workerB.WriteFloat64(girderSpacing,_T("Spcng"),5,_T("%5.2f"),true);
   }


	/* 1. SPAN NUMBER */
	TCHAR	spanNumber[5+1];
	_stprintf_s(spanNumber, sizeof(spanNumber)/sizeof(TCHAR), _T("%d"), LABEL_SPAN(segmentKey.groupIndex));

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
	TCHAR  strandPat[5+1]; 
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   if (pStrands->GetStrandDefinitionType() != CStrandData::sdtTotal)
   {
	   _tcscpy_s(strandPat, sizeof(strandPat)/sizeof(TCHAR), _T("*"));
   }
   else
   {
	   _tcscpy_s(strandPat, sizeof(strandPat)/sizeof(TCHAR), _T(" "));
   }

	/* 5. STRAND COUNT */
   StrandIndexType harpedCount   = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
   StrandIndexType straightCount = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);

	StrandIndexType strandNum = harpedCount + straightCount;


	/* 6. STRAND SIZE */
	TCHAR    strandSize[4+1];
   const matPsStrand* strandMatP = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   value = strandMatP->GetNominalDiameter();
   value = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* Convert value to fraction representation */
	txdString_ftofrac (charBuffer, sizeof(charBuffer)/sizeof(TCHAR), value, 0.125); 
	_tcscpy_s(strandSize, sizeof(strandSize)/sizeof(TCHAR), charBuffer);

   /* 7. STRAND STRENGTH */
	int strandStrength = (strandMatP->GetGrade() == matPsStrand::Gr1725 ?  250 :  270);

	/* 8. STRAND ECCENTRICITY AT CENTER LINE */
   Float64 nEff;
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pmid[0], pgsTypes::Permanent, &nEff );

	Float64 strandEccCL = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* 9. STRAND ECCENTRICITY AT END */
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pois, pgsTypes::Permanent, &nEff );

	Float64 strandEccEnd = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* 12. CONCRETE RELEASE STRENGTH */
   value = pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx);

	Float64 concreteRelStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 13. MINIMUM 28 DAY COMP. STRENGTH */
	value = pMaterial->GetSegmentFc(segmentKey,liveLoadIntervalIdx);

	Float64 min28dayCompStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 14. DESIGN LOAD COMPRESSIVE STRESS (TOP CL) */ 
   const pgsFlexuralStressArtifact* pArtifact;
   Float64 fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;

   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( liveLoadIntervalIdx, pgsTypes::ServiceI,pgsTypes::Compression,pmid[0].GetID() );
   fcTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
   fcBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);
	value = -fcTop;

	Float64 designLoadCompStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 15. DESIGN LOAD TENSILE STRESS (BOT CL) */
   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( liveLoadIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension,pmid[0].GetID() );
   ftTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
   ftBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);
	value = -ftBot;

	Float64 designLoadTensileStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

   /* 16. REQUIRED MINIMUM ULTIMATE MOMENT CAPACITY */
   MINMOMENTCAPDETAILS mmcd;
   pMomentCapacity->GetMinMomentCapacityDetails(liveLoadIntervalIdx,pmid[0],true,&mmcd);
   value = Max(mmcd.Mu,mmcd.MrMin);

	int reqMinUltimateMomentCapacity = (int)Round(::ConvertFromSysUnits( value, unitMeasure::KipFeet ));

	/* 17. LIVE LOAD DISTRIBUTION FACTOR */
   Float64 momentDistFactor = pDistFact->GetMomentDistFactor(spanKey,pgsTypes::StrengthI);

	/* 17aa. LIVE LOAD DISTRIBUTION FACTOR */
   Float64 shearDistFactor = pDistFact->GetShearDistFactor(spanKey,pgsTypes::StrengthI);

   /* 17a - Non-Standard Design Data */
   std::_tstring ns_strand_str;
   bool do_write_ns_data = isHarpedDesign && pStrands->GetStrandDefinitionType() != CStrandData::sdtTotal && !isExtendedVersion;
   if (do_write_ns_data)
   {
      ns_strand_str = MakeNonStandardStrandString(pBroker,pmid[0]);
   }

   // WRITE DATA TO OUTPUT FILE
	//----- COL 1 ----- 
   workerB.WriteString(spanNumber,_T("Span "),5,_T("%5s"),true);
	//----- COL 2 ----- 
   workerB.WriteString(beamNumber,_T(" Gdr "),5,_T("%5s"),true);
	//----- COL 3 ----- 
   workerB.WriteString(beamType,_T("Type "),5,_T("%5s"),true);
	//----- COL 4 ----- 
	workerB.WriteBlankSpaces(1);
   workerB.WriteString(strandPat,_T("N"),1,_T("%1s"),true);
	workerB.WriteBlankSpaces(2);
	//----- COL 5 ----- 
   workerB.WriteInt16((Int16)strandNum,_T("Ns "),3,_T("%3d"),true);
	//----- COL 6 ----- 
   workerB.WriteString(strandSize,_T("Size "),5,_T("%5s"),true);
	//----- COL 7 ----- 
   workerB.WriteInt16(strandStrength,_T("Strn"),4,_T("%4d"),true);
	//----- COL 8 ----- 
   workerB.WriteFloat64(strandEccCL,_T("EccCL"),5,_T("%5.2f"),true);
	//----- COL 9 ----- 
   workerB.WriteFloat64(strandEccEnd,_T("EccEn"),5,_T("%5.2f"),true);

   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

   // create debond writer in case we need em
   TxDOTCadWriter writer(segmentKey, girder_length, pStrandGeometry);

   if (isHarpedDesign)
   {
	   /* 10. COUNT OF DEPRESSED (HARPED) STRANDS */
	   StrandIndexType dstrandNum = harpedCount;

	   /* 11. DEPRESSED (HARPED) STRAND */
      pStrandGeometry->GetHighestHarpedStrandLocation(segmentKey, &value);

      Float64 dstrandTo = ::ConvertFromSysUnits( value, unitMeasure::Inch );

      // output
	   //----- COL 10 ---- 
      workerB.WriteInt16((Int16)dstrandNum,_T("Nh "),3,_T("%3d"),true);
	   //----- COL 11 ---- 
      workerB.WriteFloat64(dstrandTo,_T(" To "),4,_T("%4.1f"),true);
   }
   else
   {
      // debond or straight design
      writer.WriteInitialData(workerB);
   }

   // onward with common data for harped or debond designs
	//----- COL 12 ---- 
   workerB.WriteFloat64(concreteRelStrength,_T(" Fci  "),6,_T("%6.3f"),true);
	//----- COL 13 ---- 
   workerB.WriteFloat64(min28dayCompStrength,_T(" Fc   "),6,_T("%6.3f"),true);
	workerB.WriteBlankSpaces(1);
	//----- COL 14 ---- 
   workerB.WriteFloat64(designLoadCompStress,_T(" fcomp "),7,_T("%7.3f"),true);
	//----- COL 15 ---- 
   workerB.WriteFloat64(designLoadTensileStress,_T(" ftens  "),8,_T("%8.3f"),true);
	//----- COL 16 ---- 
   workerB.WriteInt16(reqMinUltimateMomentCapacity,_T("ultMom"),6,_T("%6d"),true);
	//----- COL 17 ---- 
   workerB.WriteFloat64(momentDistFactor,_T("LLDFmo"),6,_T("%6.3f"),true);
	//----- COL 17aa ---- 
   workerB.WriteFloat64(shearDistFactor,_T("LLDFsh"),6,_T("%6.3f"),true);

   if (do_write_ns_data)
   {
      StrandIndexType cnt = Max(ns_strand_str.size(), (size_t)7);
      workerB.WriteString(ns_strand_str.c_str(),_T("NS Data"),(Int16)cnt,_T("%s"),true);
   }


   // EXTENDED INFORMATION, IF REQUESTED // 
   if (isExtendedVersion)
   {
      GET_IFACE2(pBroker,ICamber,pCamber);
      GET_IFACE2(pBroker,IProductForces, pProductForces);
      GET_IFACE2(pBroker,ISpecification, pSpec );
      GET_IFACE2(pBroker,ILosses,pLosses);

      pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   	/* 18. ESTIMATED CAMBER IMMEDIATELY BEFORE SLAB CASTING (MAX) */
      value = pCamber->GetDCamberForGirderSchedule( pmid[0],CREEP_MAXTIME);

      Float64 initialCamber = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 19. DEFLECTION (SLAB AND DIAPHRAGMS)  */
      value = pProductForces->GetDeflection(castDeckIntervalIdx, pftSlab,      pmid[0], bat, rtIncremental, false )
            + pProductForces->GetDeflection(castDeckIntervalIdx, pftDiaphragm, pmid[0], bat, rtIncremental, false )
            + pProductForces->GetDeflection(castDeckIntervalIdx, pftShearKey,  pmid[0], bat, rtIncremental, false );

      Float64 slabDiaphDeflection = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 20. DEFLECTION (OVERLAY)  */
      value = pProductForces->GetDeflection(overlayIntervalIdx, pftOverlay, pmid[0], bat, rtIncremental, false );

      Float64 overlayDeflection = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 21. DEFLECTION (OTHER)  */
      value =  pProductForces->GetDeflection(railingSystemIntervalIdx, pftTrafficBarrier, pmid[0], bat, rtIncremental, false );
      value += pProductForces->GetDeflection(railingSystemIntervalIdx, pftSidewalk,       pmid[0], bat, rtIncremental, false );

      Float64 otherDeflection = ::ConvertFromSysUnits( value, unitMeasure::Feet );

   	/* 22. DEFLECTION (TOTAL)  */
      Float64 totalDeflection = slabDiaphDeflection + overlayDeflection + otherDeflection;

   	/* 23. LOSSES (INITIAL)  */
      Float64 aps = pStrandGeometry->GetAreaPrestressStrands(segmentKey,releaseIntervalIdx,false);
      value = pLosses->GetEffectivePrestressLoss(pmid[0],pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::Start) * aps;

      Float64 initialLoss = ::ConvertFromSysUnits( value, unitMeasure::Kip );

   	/* 24. LOSSES (FINAL)  */
      value = pLosses->GetEffectivePrestressLoss(pmid[0],pgsTypes::Permanent,railingSystemIntervalIdx,pgsTypes::Middle) * aps;

      Float64 finalLoss = ::ConvertFromSysUnits( value, unitMeasure::Kip );

   	/* 25. Lifting location  */
      GET_IFACE2(pBroker,ISegmentLifting,pLifting);
      Float64 liftLoc = ::ConvertFromSysUnits( pLifting->GetLeftLiftingLoopLocation(segmentKey), unitMeasure::Feet );

   	/* 26. Forward handling location  */
      GET_IFACE2(pBroker,ISegmentHauling,pHauling);
      Float64 fwdLoc = ::ConvertFromSysUnits( pHauling->GetLeadingOverhang(segmentKey), unitMeasure::Feet );

   	/* 27. Trailing handling location  */
      Float64 trlLoc = ::ConvertFromSysUnits( pHauling->GetTrailingOverhang(segmentKey), unitMeasure::Feet );

      /* WRITE TO FILE */
      //==================
	   //----- COL 18 ---- 
      workerB.WriteFloat64(initialCamber,_T("Dinit"),5,_T("%5.2f"),true);
	   //----- COL 19 ---- 
      workerB.WriteFloat64(slabDiaphDeflection,_T("Dslab"),5,_T("%5.2f"),true);
	   //----- COL 20 ---- 
      workerB.WriteFloat64(overlayDeflection,_T("Dolay"),5,_T("%5.2f"),true);
	   //----- COL 21 ---- 
      workerB.WriteFloat64(otherDeflection,_T("Dothr"),5,_T("%5.2f"),true);
	   //----- COL 22 ---- 
      workerB.WriteFloat64(totalDeflection,_T("Dtot "),5,_T("%5.2f"),true);
	   //----- COL 23 ---- 
      workerB.WriteFloat64(initialLoss,_T("LossIn"),6,_T("%6.2f"),true);
	   //----- COL 24 ---- 
      workerB.WriteFloat64(finalLoss,_T("LossFn"),6,_T("%6.2f"),true);
	   //----- COL 25 ---- 
      workerB.WriteFloat64(liftLoc,_T("LiftLc"),6,_T("%6.2f"),true);
	   //----- COL 26 ---- 
      workerB.WriteFloat64(fwdLoc,_T("fwHaul"),6,_T("%6.2f"),true);
	   //----- COL 27 ---- 
      workerB.WriteFloat64(trlLoc,_T("trHaul"),6,_T("%6.2f"),false);
   }

	// ------ END OF RECORD ----- 
	workerB.WriteToFile(fp);

   // final debond data
   if (!isHarpedDesign)
   {
      writer.WriteFinalData(fp,isExtendedVersion);
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


void CadWriterWorkerBee::WriteFloat64(Float64 val, LPCTSTR title, Int16 nchars, LPCTSTR format, bool doDelim)
{
   // title, format, and nchars must match in size. It's your job to deal with this
   ATLASSERT(std::_tstring(title).size()==nchars);

   int nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   m_DataLineCursor += nr;

   ATLASSERT(nr==nchars);

   if (doDelim)
   {
      nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), CAD_DELIM);
      m_DataLineCursor += nr;
   }

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, nchars, doDelim);
   }
}

void CadWriterWorkerBee::WriteInt16(Int16 val, LPCTSTR title, Int16 nchars, LPCTSTR format, bool doDelim)
{
   int nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   m_DataLineCursor += nr;

   ATLASSERT(nr==nchars);

   if (doDelim)
   {
      nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), CAD_DELIM);
      m_DataLineCursor += nr;
   }

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, nchars, doDelim);
   }
}

void CadWriterWorkerBee::WriteString(LPCTSTR val, LPCTSTR title, Int16 nchars, LPCTSTR format, bool doDelim)
{
   int nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   m_DataLineCursor += Max((int)nchars,nr);

//   ATLASSERT(nr==nchars);

   if (doDelim)
   {
      nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), CAD_DELIM);
      m_DataLineCursor += nr;
   }

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, nchars, doDelim);
   }
}

void CadWriterWorkerBee::WriteBlankSpaces(Int16 ns)
{
#if defined _UNICODE
   wmemset(m_DataLineCursor,_T(' '),ns);
   m_DataLineCursor += ns;

   if (m_DoWriteTitles)
   {
      wmemset(m_TitleLineCursor,_T(' '),ns);
      m_TitleLineCursor += ns;

      wmemset(m_DashLineCursor,_T(' '),ns);
      m_DashLineCursor += ns;
   }
#else
   memset(m_DataLineCursor,' ',ns);
   m_DataLineCursor += ns;

   if (m_DoWriteTitles)
   {
      memset(m_TitleLineCursor,' ',ns);
      m_TitleLineCursor += ns;

      memset(m_DashLineCursor,' ',ns);
      m_DashLineCursor += ns;
   }
#endif
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
#if defined _UNICODE
    wmemset(m_DataLine,_T('\0'),BF_SIZ);
    wmemset(m_TitleLine,_T('\0'),BF_SIZ);
    wmemset(m_DashLine,_T('\0'),BF_SIZ);
#else
    memset(m_DataLine,'\0',BF_SIZ);
    memset(m_TitleLine,'\0',BF_SIZ);
    memset(m_DashLine,'\0',BF_SIZ);
#endif

    // First column is blank
    m_DataLine[0]  = _T(' ');
    m_TitleLine[0] = _T(' ');
    m_DashLine[0]  = _T(' ');

    m_DataLineCursor  = m_DataLine+1;
    m_TitleLineCursor = m_TitleLine+1;
    m_DashLineCursor  = m_DashLine+1;
}

void CadWriterWorkerBee::WriteTitle(LPCTSTR title, Int16 nchars, bool doDelim)
{
   // Write title line and dash line since
#if defined _UNICODE
   wmemcpy(m_TitleLineCursor, title, nchars);
#else
   memcpy(m_TitleLineCursor, title, nchars);
#endif
   m_TitleLineCursor += nchars;

#if defined _UNICODE
   wmemset(m_DashLineCursor,_T('-'), nchars);
#else
   memset(m_DashLineCursor,'-', nchars);
#endif
   m_DashLineCursor += nchars;

   if (doDelim)
   {
#if defined _UNICODE
      wmemset(m_TitleLineCursor, _T(' '), 1);
#else
      memset(m_TitleLineCursor, ' ', 1);
#endif
      m_TitleLineCursor++;

#if defined _UNICODE
      wmemset(m_DashLineCursor, _T(' '), 1);
#else
      memset(m_DashLineCursor, ' ', 1);
#endif
      m_DashLineCursor++;
   }
}

void TxDOTCadWriter::WriteInitialData(CadWriterWorkerBee& workerB)
{
   // first build our data structure
   Compute();

   // next write out data
	//----- COL 10 ---- 
   workerB.WriteInt16((Int16)m_NumDebonded,_T("Ndb"),3,_T("%3d"),true);

   if (m_Rows.empty() || m_OutCome==SectionMismatch || m_OutCome==TooManySections || m_OutCome==SectionsNotSymmetrical)
   {
      // row height, srands in row, and debonds in row are zero
	   workerB.WriteFloat64(0.0,_T("Debnd"),5,_T("%5.2f"),true);
      workerB.WriteInt16(0,_T("   "),3,_T("%3d"),true);
      workerB.WriteInt16(0,_T("   "),3,_T("%3d"),true);

      if (m_Rows.empty())
      {
         // no use searching for nothing
   	   //----- COL 11-23 ---- 
         for (int i=0; i<10; i++)
         {
            workerB.WriteInt16(0,_T("  "),2,_T("%2d"),true);
         }
      }
      else
      {
         // this is an error condition, just right out blanks to fill space
	      //----- COL 11-23 ---- 
         workerB.WriteBlankSpaces(30);
      }
   }
   else
   {
      // A little checking
      RowIndexType nrs = m_pStrandGeometry->GetNumRowsWithStrand(m_SegmentKey,pgsTypes::Straight);
      ATLASSERT((RowIndexType)m_Rows.size() == nrs); // could have more rows than rows with debonded strands

      // Where the rubber hits the road - Write first row
      const RowData& row = *(m_Rows.begin());
      WriteRowData(workerB, row);
   }
}

void TxDOTCadWriter::WriteFinalData(FILE *fp, bool isExtended)
{
   // fist write out remaining rows 
   if(!m_Rows.empty())
   {
      Int16 nLeadingSpaces = isExtended ? 69 : 53; // more leading spaces for extended output
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

   SpanIndexType spanIdx = m_SegmentKey.groupIndex;
   GirderIndexType gdrIdx = m_SegmentKey.girderIndex;
   ATLASSERT(m_SegmentKey.segmentIndex == 0);

   // lastly write any information
   if (m_OutCome==SectionMismatch || m_OutCome==SectionsNotSymmetrical)
   {
	   _ftprintf(fp, _T("Warning: Irregular, Non-standard debonding increments used for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
   }
   else if (m_OutCome==TooManySections)
   {
	   _ftprintf(fp, _T("Warning: The number of debonded sections exceeds ten for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
   }
   else if (m_OutCome==NonStandardSection)
   {
      Float64 spac = ::ConvertFromSysUnits(m_SectionSpacing , unitMeasure::Feet );
	   _ftprintf(fp, _T("Warning: Non-standard debonding increment of %6.3f ft used  for beam %s in span %2d. \n"),spac,LABEL_GIRDER(gdrIdx),LABEL_SPAN(spanIdx));
   }
}

void TxDOTCadWriter::WriteRowData(CadWriterWorkerBee& workerB, const RowData& row) const
{
	//----- COL 11 ----- 
   // elevation of row
   Float64 row_elev = ::ConvertFromSysUnits( row.m_Elevation, unitMeasure::Inch );

   workerB.WriteFloat64(row_elev,_T("Elev "),5,_T("%5.2f"),true);

	//----- COL 12 ---- 
   // total strands in row
   workerB.WriteInt16((Int16)row.m_NumTotalStrands,_T("Nsr"),3,_T("%3d"),true);

	//----- COL 13 ---- 
   // num debonded strands in row
   Int16 nsr = CountDebondsInRow(row);
   workerB.WriteInt16(nsr,_T("Ndb"),3,_T("%3d"),true);

	//----- COL 14-23 ---- 
   // we have 10 columns to write no matter what
   SectionListConstIter scit = row.m_Sections.begin();

   TCHAR buff[4];
   for (Int16 icol=0; icol<10; icol++)
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

      workerB.WriteInt16(db_cnt,buff,2,_T("%2d"),true);
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


int TxDOT_WriteDistributionFactorsToFile (FILE *fp, IBroker* pBroker, const CGirderKey& girderKey)
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
   Float64 gR,  gR1,  gR2;   // reaction

   pDfEng->GetDFResultsEx(spanKey,pgsTypes::StrengthI,
                          &gpM, &gpM1, &gpM2,
                          &gnM, &gnM1, &gnM2,
                          &gV,  &gV1,  &gV2,
                          &gR,  &gR1,  &gR2 );

	TCHAR	spanNumber[5+1];
	_stprintf_s(spanNumber, sizeof(spanNumber)/sizeof(TCHAR), _T("%d"), LABEL_SPAN(spanIdx));

	TCHAR  beamNumber[5+1];
	_stprintf_s(beamNumber, sizeof(beamNumber)/sizeof(TCHAR), _T("%s"), LABEL_GIRDER(gdrIdx));

   // have our data, now need to write it
   CadWriterWorkerBee workerB(true);

   workerB.WriteString(spanNumber,_T("Span "),5,_T("%5s"),true);
   workerB.WriteString(beamNumber,_T(" Gdr "),5,_T("%5s"),true);

   workerB.WriteFloat64(gpM, _T(" gpM  "),6,_T("%6.3f"),true);
   workerB.WriteFloat64(gpM1,_T(" gpM1 "),6,_T("%6.3f"),true);
   workerB.WriteFloat64(gpM2,_T(" gpM2 "),6,_T("%6.3f"),true);

   workerB.WriteFloat64(gnM, _T(" gnM  "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(gnM1,_T(" gnM1 "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(gnM2,_T(" gnM2 "),6,_T("%6.2f"),true);

   workerB.WriteFloat64(gV, _T("  gV  "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(gV1,_T(" gV1  "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(gV2,_T(" gV2  "),6,_T("%6.2f"),true);

   workerB.WriteFloat64(gR, _T("  gR  "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(gR1,_T(" gR1  "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(gR2,_T(" gR2  "),6,_T("%6.2f"),false);

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
      Float64 elev_in = RoundOff(::ConvertFromSysUnits( srow.Elevation, unitMeasure::Inch ),0.001);
      os<<elev_in<<_T("(")<<srow.Count<<_T(")");
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

   workerB.WriteFloat64(stress_val_input, _T("ftinp "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(stress_val_calc, _T("ftcalc"),6,_T("%6.2f"),true);
   workerB.WriteFloat64(stress_fac, _T("ftfact"),6,_T("%6.2f"),true);

   // Tensile stress - bottom
   pGetTogaResults->GetControllingTensileStress(&stress_val_calc, &stress_fac, &stress_loc);

   stress_val_input = ::ConvertFromSysUnits( pProjectData->GetFb(), unitMeasure::KSI );
   stress_val_calc = ::ConvertFromSysUnits( -stress_val_calc, unitMeasure::KSI );

   workerB.WriteFloat64(stress_val_input, _T("fbinp "),6,_T("%6.2f"),true);
   workerB.WriteFloat64(stress_val_calc, _T("fbcalc"),6,_T("%6.2f"),true);
   workerB.WriteFloat64(stress_fac, _T("fbfact"),6,_T("%6.2f"),true);

   // Ultimate moment
   Float64 mu_input = ::ConvertFromSysUnits( pProjectData->GetMu(), unitMeasure::KipFeet);
   Float64 mu_orig  = ::ConvertFromSysUnits( pGetTogaResults->GetRequiredUltimateMoment(), unitMeasure::KipFeet );
   Float64 mu_fabr  = ::ConvertFromSysUnits( pGetTogaResults->GetUltimateMomentCapacity(), unitMeasure::KipFeet );

   workerB.WriteFloat64(mu_input,_T(" muinp  "),8,_T("%8.2f"),true);
   workerB.WriteFloat64(mu_orig, _T(" muorig "),8,_T("%8.2f"),true);
   workerB.WriteFloat64(mu_fabr, _T(" mufabr "),8,_T("%8.2f"),true);

   // Required concrete strengths
   Float64 input_fci = ::ConvertFromSysUnits(pProjectData->GetPrecasterDesignGirderData()->GetFci(), unitMeasure::KSI );
   Float64 reqd_fci  = ::ConvertFromSysUnits(pGetTogaResults->GetRequiredFci(), unitMeasure::KSI );

   workerB.WriteFloat64(input_fci,_T("fciinp"),6,_T("%6.2f"),true);
   workerB.WriteFloat64(reqd_fci, _T("fcireq"),6,_T("%6.2f"),true);

   Float64 input_fc = ::ConvertFromSysUnits(pProjectData->GetPrecasterDesignGirderData()->GetFc(), unitMeasure::KSI );
   Float64 reqd_fc =  ::ConvertFromSysUnits(pGetTogaResults->GetRequiredFc(), unitMeasure::KSI );

   workerB.WriteFloat64(input_fc,_T("fc inp"),6,_T("%6.2f"),true);
   workerB.WriteFloat64(reqd_fc, _T("fc req"),6,_T("%6.2f"),true);

   // Camber
   Float64 cbr_orig = ::ConvertFromSysUnits(pGetTogaResults->GetMaximumCamber(), unitMeasure::Feet );
   Float64 cbr_fabr = ::ConvertFromSysUnits(pGetTogaResults->GetFabricatorMaximumCamber(), unitMeasure::Feet );

   workerB.WriteFloat64(cbr_orig,_T("cbr orig"),8,_T("%8.4f"),true);
   workerB.WriteFloat64(cbr_fabr,_T("cbr fabr"),8,_T("%8.4f"),true);

   // Shear check
   bool passed = pGetTogaResults->ShearPassed();
   workerB.WriteString(passed?_T("Ok\0"):_T("Fail\n"),_T("Shear"),7,_T("%7s"),true);

   workerB.WriteToFile(fp);

   _ftprintf(fp, _T("\n"));

   return CAD_SUCCESS;
}
