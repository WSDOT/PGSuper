///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include "TxDOTLegacyCadWriter.h"

#include "TOGATestFileWriter.h"

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
#include <EAF\EAFAutoProgress.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Workhorse for writing debond information

class TxDOTCadWriter : public TxDOTDebondTool
{
public:

   TxDOTCadWriter(const CSegmentKey& segmentKey, Float64 girderLength, bool isUBeam, IStrandGeometry* pStrandGeometry):
   TxDOTDebondTool(segmentKey, girderLength, pStrandGeometry), 
   m_isUBeam(isUBeam)
   {;}

   void WriteInitialData(CadWriterWorkerBee& workerBee);
   void WriteFinalData(FILE *fp, bool isExtended, bool isIBeam, Int16 extraSpacesForSlabOffset);

   StrandIndexType GetTotalNumDebonds() const
   {
      return this->m_NumDebonded;
   }

private:
   void WriteRowData(CadWriterWorkerBee& workerBee, const RowData& row,Float64 Hg) const;
   bool m_isUBeam;
};

//////////// Useful functions /////////////////
static std::_tstring MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi);
static int TxDOT_WriteCADDataForGirder(FILE *fp, IBroker* pBroker, const CGirderKey& girderKey);


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
        return E_FAIL;
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

	return S_OK;
}

////////////////// Main function for writing legacy cad file ///////////////////////////
int TxDOT_WriteLegacyCADDataToFile(CString& filePath, IBroker* pBroker, const std::vector<CGirderKey>& girderKeys)
{

   bool did_throw=false;

   // Create progress window in own scope
   try
   {
	   /* Create progress bar (before needing one) to remain alive during this task */
	   /* (otherwise, progress bars will be repeatedly created & destroyed on the fly) */
      GET_IFACE2(pBroker,IProgress,pProgress);

      bool multi = girderKeys.size()>1;
      DWORD mask = multi ? PW_ALL : PW_ALL|PW_NOGAUGE; // Progress window has a cancel button,
      CEAFAutoProgress ap(pProgress,0,mask); 

      if (multi)
         pProgress->Init(0,(short)girderKeys.size(),1);  // and for multi-girders, a gauge.

		/* Open/create the specified text file */
      FILE	*fp = nullptr;
      if (_tfopen_s(&fp,LPCTSTR(filePath), _T("w+")) != 0 || fp == nullptr)
      {
			AfxMessageBox (_T("Warning: File Cannot be Created."));
			return S_OK;
		}

	   /* Write CAD data to text file */
      for (std::vector<CGirderKey>::const_iterator it = girderKeys.begin(); it!= girderKeys.end(); it++)
      {
         const CGirderKey& girderKey(*it);

	      if (S_OK != TxDOT_WriteCADDataForGirder(fp, pBroker, girderKey) )
         {
		      AfxMessageBox (_T("Warning: An error occured while writing to File"));
		      return S_OK;
         }

         pProgress->Increment();
      }

		/* Close the open text file */
		fclose (fp);

   } // autoprogress scope
   catch(...)
   {
      // must catch so progress window goes out of scope and gets destroyed
      // must rethrow to get the exception into MFC
      throw; 
   }

	/* Notify completion */
	CString msg(_T("File: "));
	msg += filePath + _T(" creation complete.");
   AfxMessageBox(msg,MB_ICONINFORMATION|MB_OK);

   return S_OK;
}

int TxDOT_WriteCADDataForGirder(FILE *fp, IBroker* pBroker, const CGirderKey& girderKey)
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
   bool is_test_output = false;
   CadWriterWorkerBee workerB(is_test_output);//

	/* Create pois at the start of girder and mid-span */
   PoiList vPoi;
   pPointOfInterest->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& pois(vPoi.front());
	
   vPoi.clear();// recycle list
   pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
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
      Float64 nEff;
      Float64 hs_ecc_end = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pois,pgsTypes::Harped,&nEff);
      Float64 hs_ecc_mid = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pmid,pgsTypes::Harped,&nEff);
      are_harped_bent = !IsEqual(hs_ecc_end, hs_ecc_mid);
   }

   bool isExtendedVersion = false;

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

      Float64 roadwayWidth = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   /* 0b. NUMBER OF BEAMS */
      GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);

	   /* 0a. BEAM SPACING */
      ATLASSERT( pBridgeDesc->GetGirderSpacingType() != pgsTypes::sbsGeneral );
      GirderIndexType spaceIdx = (segmentKey.girderIndex == nGirders-1 ? nGirders-2 : segmentKey.girderIndex);
      value = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing(spaceIdx);

      Float64 girderSpacing = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   //----- COL 0a ---- 
	   workerB.WriteFloat64(roadwayWidth,_T("RoadW"),7,5,_T("%5.2f"));
	   //----- COL 0b ----- 
	   workerB.WriteInt16((Int16)nGirders,_T("Ng "),5,3,_T("%3d"));
	   //----- COL 0c ----- 
	   workerB.WriteFloat64(girderSpacing,_T("Spcng"),7,5,_T("%5.2f"));
   }


	/* 1. SPAN NUMBER */
	TCHAR	spanNumber[5+1];
	_stprintf_s(spanNumber, sizeof(spanNumber)/sizeof(TCHAR), _T("%d"), (int)LABEL_SPAN(segmentKey.groupIndex));

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
   bool do_write_ns_data = !IsTxDOTStandardStrands( isHarpedDesign, pStrands->GetStrandDefinitionType(), segmentKey, pBroker );
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
   const matPsStrand* strandMatP = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   value = strandMatP->GetNominalDiameter();
   value = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* Convert value to fraction representation */
	txdString_ftofrac (charBuffer, sizeof(charBuffer)/sizeof(TCHAR), value); 
	_tcscpy_s(strandSize, sizeof(strandSize)/sizeof(TCHAR), charBuffer);

   /* 7. STRAND STRENGTH */
	int strandStrength = (strandMatP->GetGrade() == matPsStrand::Gr1725 ?  250 :  270);

	/* 8. STRAND ECCENTRICITY AT CENTER LINE */
   Float64 nEff;
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pmid, pgsTypes::Permanent, &nEff );

	Float64 strandEccCL = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* 9. STRAND ECCENTRICITY AT END */
   value = pStrandGeometry->GetEccentricity( releaseIntervalIdx, pois, pgsTypes::Permanent, &nEff );

	Float64 strandEccEnd = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* 12. CONCRETE RELEASE STRENGTH */
   value = pMaterial->GetSegmentDesignFc(segmentKey,releaseIntervalIdx);

	Float64 concreteRelStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 13. MINIMUM 28 DAY COMP. STRENGTH */
	value = pMaterial->GetSegmentDesignFc(segmentKey,lastIntervalIdx);

	Float64 min28dayCompStrength = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 14. DESIGN LOAD COMPRESSIVE STRESS (TOP CL) */ 
   const pgsFlexuralStressArtifact* pArtifact;
   Float64 fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;

   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( lastIntervalIdx, pgsTypes::ServiceI,pgsTypes::Compression,pmid.GetID() );
   fcTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
	value = -fcTop;

	Float64 designLoadCompStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

	/* 15. DESIGN LOAD TENSILE STRESS (BOT CL) */
   pArtifact = pGdrArtifact->GetFlexuralStressArtifactAtPoi( lastIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension,pmid.GetID() );
   ftBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);
	value = -ftBot;

	Float64 designLoadTensileStress = ::ConvertFromSysUnits( value, unitMeasure::KSI );

   /* 16. REQUIRED MINIMUM ULTIMATE MOMENT CAPACITY */
   const MINMOMENTCAPDETAILS* mmcd = pMomentCapacity->GetMinMomentCapacityDetails(lastIntervalIdx,pmid,true);
   value = Max(mmcd->Mu,mmcd->MrMin);

	int reqMinUltimateMomentCapacity = (int)Round(::ConvertFromSysUnits( value, unitMeasure::KipFeet ));

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
   if (isExtendedVersion && pBridge->GetDeckType()!=pgsTypes::sdtNone)
   {
      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(segmentKey.groupIndex, &startPierIdx, &endPierIdx);

      Float64 astart = pBridge->GetSlabOffset(segmentKey, pgsTypes::metStart);
      Float64 aend = pBridge->GetSlabOffset(segmentKey, pgsTypes::metEnd);


      astart = ::ConvertFromSysUnits( astart, unitMeasure::Inch );
      aend = ::ConvertFromSysUnits( aend, unitMeasure::Inch );

      workerB.WriteFloat64(astart,_T("Astart"),7,5,_T("%5.2f"));
      workerB.WriteFloat64(aend,_T("Aend"),7,5,_T("%5.2f"));

      extraSpacesForSlabOffset = 14; // width of two data fields above = 7+7

      GET_IFACE2(pBroker,ISpecification,pSpec);
      if (pSpec->IsAssumedExcessCamberInputEnabled())
      {
         value = pIBridgeDesc->GetAssumedExcessCamber(segmentKey.groupIndex, segmentKey.girderIndex);
      	Float64 aecamber = ::ConvertFromSysUnits( value, unitMeasure::Inch );
         workerB.WriteFloat64(aecamber,_T("AECmbr"),7,5,_T("%5.2f"));
         extraSpacesForSlabOffset += 7;
      }
    }

   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

   Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, pois);

   // create debond writer in case we need it
   TxDOTCadWriter writer(segmentKey, girder_length, isUBeam, pStrandGeometry);

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

         dstrandToEnd = ::ConvertFromSysUnits( value, unitMeasure::Inch );

         pStrandGeometry->GetHighestHarpedStrandLocationHPs(segmentKey, &value);
         value += Hg;

         dstrandToCL = ::ConvertFromSysUnits( value, unitMeasure::Inch );
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
         dstrandToEnd = ::ConvertFromSysUnits( value+Hg, unitMeasure::Inch );

         pStrandGeometry->GetHighestHarpedStrandLocationHPs(segmentKey, &value);
         dstrandToCL = ::ConvertFromSysUnits( value+Hg, unitMeasure::Inch );

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
   workerB.WriteInt16(reqMinUltimateMomentCapacity,_T("ultMo"),9,5,_T("%5d"));
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

      Float64 initialCamber = ::ConvertFromSysUnits( value, unitMeasure::Inch );

   	/* 19. DEFLECTION (SLAB AND DIAPHRAGMS)  */
      value = pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftSlab,      pmid, bat, rtCumulative, false )
            + pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftSlabPad,   pmid, bat, rtCumulative, false )
            + pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftDiaphragm, pmid, bat, rtCumulative, false )
            + pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftShearKey,  pmid, bat, rtCumulative, false );
      value = IsZero(value) ? 0 : value;

      Float64 slabDiaphDeflection = ::ConvertFromSysUnits( value, unitMeasure::Inch );

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

      Float64 overlayDeflection = ::ConvertFromSysUnits( value, unitMeasure::Inch );

   	/* 21. DEFLECTION (OTHER)  */
      value =  pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftTrafficBarrier, pmid, bat, rtCumulative, false );
      value += pProductForces->GetDeflection(lastIntervalIdx, pgsTypes::pftSidewalk,       pmid, bat, rtCumulative, false );
      value = IsZero(value) ? 0 : value;

      Float64 otherDeflection = ::ConvertFromSysUnits( value, unitMeasure::Inch );

   	/* 22. DEFLECTION (TOTAL)  */
      Float64 totalDeflection = slabDiaphDeflection + overlayDeflection + otherDeflection;

   	/* 23. LOSSES (INITIAL)  */
      Float64 aps = pStrandGeometry->GetAreaPrestressStrands(segmentKey,releaseIntervalIdx,false);
      value = pLosses->GetEffectivePrestressLoss(pmid,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::End) * aps;

      Float64 initialLoss = ::ConvertFromSysUnits( value, unitMeasure::Kip );

   	/* 24. LOSSES (FINAL)  */
      value = pLosses->GetEffectivePrestressLoss(pmid,pgsTypes::Permanent,lastIntervalIdx,pgsTypes::End) * aps;

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
      _ftprintf(fp, _T("Warning: Beam %s in span %2d has mixed harped and debonded strands. This is an invalid strand configuration for TxDOT, and is not supported by the TxDOT CAD Data Export feature. Strand data may not be reported correctly.\n"), LABEL_GIRDER(gdrIdx), (int)LABEL_SPAN(spanIdx));
   }

   return S_OK;
}

void TxDOTCadWriter::WriteInitialData(CadWriterWorkerBee& workerB)
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

void TxDOTCadWriter::WriteFinalData(FILE *fp, bool isExtended, bool isIBeam, Int16 extraSpacesForSlabOffset)
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
	   _ftprintf(fp, _T("Warning: Irregular, Non-standard debonding increments used for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),(int)LABEL_SPAN(spanIdx));
   }
   else if (m_OutCome==TooManySections)
   {
	   _ftprintf(fp, _T("Warning: The number of debonded sections exceeds ten for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n"),LABEL_GIRDER(gdrIdx),(int)LABEL_SPAN(spanIdx));
   }
   else if (m_OutCome==NonStandardSection)
   {
      Float64 spac = ::ConvertFromSysUnits(m_SectionSpacing , unitMeasure::Feet );
	   _ftprintf(fp, _T("Warning: Non-standard debonding increment of %6.3f ft used  for beam %s in span %2d. \n"),spac,LABEL_GIRDER(gdrIdx),(int)LABEL_SPAN(spanIdx));
   }
}

void TxDOTCadWriter::WriteRowData(CadWriterWorkerBee& workerB, const RowData& row, Float64 Hg) const
{
	//----- COL 11 ----- 
   // elevation of row
   Float64 row_elev = ::ConvertFromSysUnits( Hg + row.m_Elevation, unitMeasure::Inch );

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

