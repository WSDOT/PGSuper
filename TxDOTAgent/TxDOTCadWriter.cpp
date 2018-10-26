///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
static std::string MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi);

// Return string for strand size
static int txdString_ftofrac	/* <=  Completion value                   */
(
char		*stringP,		      /* <=  Output text string                 */
double		value,			   /*  => Value to convert                   */
double		resolution		   /*  => Fractional resolution              */
)
{
	double	fraction = 0.0, whole = 0.0;
	int		index = 0;
	char table[][4] = {" ","1/8","1/4","3/8","1/2","5/8","3/4","7/8"};

	/* Validate arguments */
	if (stringP == NULL) return (ERROR);
	stringP[0] = 0;
	resolution = 0.125;	// temp

	/* Break number into whole & fraction */
	fraction = modf (value, &whole);	
	
	/* Create output string */
	if (whole > 0) sprintf_s(stringP, sizeof(stringP), "%.0lf ",whole);

	/* Apply resolution to fraction */
	index = (int)((fraction + (resolution / 2.0)) / resolution);

	/* Append fraction string */
	strcat_s(stringP,sizeof(stringP), table[index]);

	return (CAD_SUCCESS);
}
	


int TxDOT_WriteCADDataToFile (FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, TxDOTCadExportFormatType format, bool designSucceeded)
{
// Get data first and convert to correct units. then write it all at end of function
// Note that Units are hard-coded into this routine. TxDOT has no use for SI units
	char	charBuffer[32];
   Float64 value;


	/* Regenerate bridge data */
	GET_IFACE2(pBroker, IArtifact, pIArtifact);
 	const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span, gdr);
   if(!(pGdrArtifact->Passed()))
	{
//		AfxMessageBox("The Specification Check was NOT Successful",MB_OK);
	}

	/* Interfaces to all relevant agents */
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker, IGirderData, pGirderData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, ISectProp2, pSectProp2);
	GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2(pBroker, ILibrary,pLibrary);
   GET_IFACE2(pBroker, IMomentCapacity, pMomentCapacity);
   GET_IFACE2(pBroker, ILiveLoadDistributionFactors, pDistFact);
   GET_IFACE2(pBroker, IBridgeMaterial, pMaterial);

   // Use workerbee class to do actual writing of data
   bool is_test_output = (format== tcxTest) ? true : false;
   CadWriterWorkerBee workerB(is_test_output);//

   // determine type of output
   bool isHarpedDesign = 0 < pStrandGeometry->GetMaxStrands(span, gdr, pgsTypes::Harped);

	/* Create pois at the start of girder and mid-span */
	pgsPointOfInterest pois(span, gdr, 0.0);
	std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(pgsTypes::BridgeSite1, span, gdr, POI_MIDSPAN);
	CHECK(pmid.size() == 1);


   bool isExtendedVersion = (format==tcxExtended || format==tcxTest);

   // extended version writes data at front and back of line
   if (isExtendedVersion)
   {
	   /* 0a. ROADWAY WIDTH */
      value = pBridge->GetCurbToCurbWidth(0.00);

      Float64 roadwayWidth = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   /* 0b. NUMBER OF BEAMS */
      GirderIndexType nGirders = pBridge->GetGirderCount(span);

	   /* 0a. BEAM SPACING */
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      ATLASSERT( pBridgeDesc->GetGirderSpacingType() != pgsTypes::sbsGeneral );
      GirderIndexType spaceIdx = (gdr == nGirders-1 ? nGirders-2 : gdr);
      value = pBridgeDesc->GetSpan(span)->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(spaceIdx);

      Float64 girderSpacing = ::ConvertFromSysUnits( value, unitMeasure::Feet );

	   //----- COL 0a ---- 
	   workerB.WriteFloat64(roadwayWidth,"RoadW",5,"%5.2f",true);
	   //----- COL 0b ----- 
	   workerB.WriteInt16((Int16)nGirders,"Ng ",3,"%3d",true);
	   //----- COL 0c ----- 
	   workerB.WriteFloat64(girderSpacing,"Spcng",5,"%5.2f",true);
   }


	/* 1. SPAN NUMBER */
	char	spanNumber[5+1];
	sprintf_s(spanNumber, sizeof(spanNumber), "%d", LABEL_SPAN(span));

	/* 1. GIRDER NUMBER */
	char  beamNumber[5+1];
	sprintf_s(beamNumber, sizeof(beamNumber), "%s", LABEL_GIRDER(gdr));

	/* 3. BEAM TYPE */
	char beamType[5+1];
   beamType[5] = '\0';
   memset( beamType,' ',5);
   std::string str = pIBridgeDesc->GetBridgeDescription()->GetSpan(span)->GetGirderTypes()->GetGirderName(gdr);

   // assume that last contiguous string is type
   size_t start = str.rfind(" ");
   str.erase(0,start+1);
   size_t cnt = min(5, str.length());

   // if string doesn't fill, leave first char blank
   char* cnxt = beamType;
   size_t count = sizeof(beamType);
   if (cnt<5)
   {
      cnxt++;
      count--;
   }

   strncpy_s(cnxt, count, str.c_str(), cnt);

	/* 4. STRAND PATTERN */
	char  strandPat[5+1]; 
   CGirderData girderData = pGirderData->GetGirderData(span, gdr);
   if (girderData.NumPermStrandsType != NPS_TOTAL_NUMBER)
   {
	   strcpy_s(strandPat, sizeof(strandPat), "*");
   }
   else
   {
	   strcpy_s(strandPat, sizeof(strandPat), " ");
   }

	/* 5. STRAND COUNT */
   int harpedCount   = pStrandGeometry->GetNumStrands(span, gdr,pgsTypes::Harped);
   int straightCount = pStrandGeometry->GetNumStrands(span, gdr,pgsTypes::Straight);

	int strandNum = harpedCount + straightCount;


	/* 6. STRAND SIZE */
	char    strandSize[4+1];
	const matPsStrand* strandMatP = pGirderData->GetStrandMaterial(span, gdr);
   value = strandMatP->GetNominalDiameter();
   value = ::ConvertFromSysUnits( value, unitMeasure::Inch );

	/* Convert value to fraction representation */
	txdString_ftofrac (charBuffer, value, 0.125); 
	strcpy_s(strandSize, sizeof(strandSize), charBuffer);

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
   double fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;

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
   Float64 liveLoadDistFactor = pDistFact->GetMomentDistFactor(span, gdr, pgsTypes::StrengthI);

   /* 17a - Non-Standard Design Data */
   std::string ns_strand_str;
   bool do_write_ns_data = isHarpedDesign && girderData.NumPermStrandsType != NPS_TOTAL_NUMBER && !isExtendedVersion;
   if (do_write_ns_data)
   {
      ns_strand_str = MakeNonStandardStrandString(pBroker,pmid[0]);
   }

   // WRITE DATA TO OUTPUT FILE
	//----- COL 1 ----- 
   workerB.WriteString(spanNumber,"Span ",5,"%5s",true);
	//----- COL 2 ----- 
   workerB.WriteString(beamNumber," Gdr ",5,"%5s",true);
	//----- COL 3 ----- 
   workerB.WriteString(beamType,"Type ",5,"%5s",true);
	//----- COL 4 ----- 
	workerB.WriteBlankSpaces(1);
   workerB.WriteString(strandPat,"N",1,"%1s",true);
	workerB.WriteBlankSpaces(2);
	//----- COL 5 ----- 
   workerB.WriteInt16(strandNum,"Ns ",3,"%3d",true);
	//----- COL 6 ----- 
   workerB.WriteString(strandSize,"Size ",5,"%5s",true);
	//----- COL 7 ----- 
   workerB.WriteInt16(strandStrength,"Strn",4,"%4d",true);
	//----- COL 8 ----- 
   workerB.WriteFloat64(strandEccCL,"EccCL",5,"%5.2f",true);
	//----- COL 9 ----- 
   workerB.WriteFloat64(strandEccEnd,"EccEn",5,"%5.2f",true);

   Float64 girder_length = pBridge->GetGirderLength(span, gdr);

   // create debond writer in case we need em
   TxDOTCadWriter writer(span, gdr, girder_length, pStrandGeometry);

   if (isHarpedDesign)
   {
	   /* 10. COUNT OF DEPRESSED (HARPED) STRANDS */
	   int dstrandNum = harpedCount;

	   /* 11. DEPRESSED (HARPED) STRAND */
      pStrandGeometry->GetHighestHarpedStrandLocation(span, gdr, &value);

      Float64 dstrandTo = ::ConvertFromSysUnits( value, unitMeasure::Inch );

      // output
	   //----- COL 10 ---- 
      workerB.WriteInt16(dstrandNum,"Nh ",3,"%3d",true);
	   //----- COL 11 ---- 
      workerB.WriteFloat64(dstrandTo," To ",4,"%4.1f",true);
   }
   else
   {
      // debond or straight design
      writer.WriteInitialData(workerB);
   }

   // onward with common data for harped or debond designs
	//----- COL 12 ---- 
   workerB.WriteFloat64(concreteRelStrength," Fci  ",6,"%6.3f",true);
	//----- COL 13 ---- 
   workerB.WriteFloat64(min28dayCompStrength," Fc   ",6,"%6.3f",true);
	workerB.WriteBlankSpaces(1);
	//----- COL 14 ---- 
   workerB.WriteFloat64(designLoadCompStress," fcomp ",7,"%7.3f",true);
	//----- COL 15 ---- 
   workerB.WriteFloat64(designLoadTensileStress," ftens  ",8,"%8.3f",true);
	//----- COL 16 ---- 
   workerB.WriteInt16(reqMinUltimateMomentCapacity,"ultMom",6,"%6d",true);
	//----- COL 17 ---- 
   workerB.WriteFloat64(liveLoadDistFactor," lldf ",6,"%6.3f",true);

   if (do_write_ns_data)
   {
      int cnt = max(ns_strand_str.size(), 7);
      workerB.WriteString(ns_strand_str.c_str(),"NS Data",cnt,"%s",true);
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

   	/* 19. DEFLECTION (SLAB AND DIAPHRAGMS)"  */
      value = pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftSlab,      pmid[0], SimpleSpan )
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
      const CHandlingData& handlingData = girderData.HandlingData;
      Float64 liftLoc = ::ConvertFromSysUnits( handlingData.LeftLiftPoint, unitMeasure::Feet );

   	/* 26. Forward handling location  */
      Float64 fwdLoc = ::ConvertFromSysUnits( handlingData.LeadingSupportPoint, unitMeasure::Feet );

   	/* 27. Trailing handling location  */
      Float64 trlLoc = ::ConvertFromSysUnits( handlingData.TrailingSupportPoint, unitMeasure::Feet );

      /* WRITE TO FILE */
      //==================
	   //----- COL 18 ---- 
      workerB.WriteFloat64(initialCamber,"Dinit",5,"%5.2f",true);
	   //----- COL 19 ---- 
      workerB.WriteFloat64(slabDiaphDeflection,"Dslab",5,"%5.2f",true);
	   //----- COL 20 ---- 
      workerB.WriteFloat64(overlayDeflection,"Dolay",5,"%5.2f",true);
	   //----- COL 21 ---- 
      workerB.WriteFloat64(otherDeflection,"Dothr",5,"%5.2f",true);
	   //----- COL 22 ---- 
      workerB.WriteFloat64(totalDeflection,"Dtot ",5,"%5.2f",true);
	   //----- COL 23 ---- 
      workerB.WriteFloat64(initialLoss,"LossIn",6,"%6.2f",true);
	   //----- COL 24 ---- 
      workerB.WriteFloat64(finalLoss,"LossFn",6,"%6.2f",true);
	   //----- COL 25 ---- 
      workerB.WriteFloat64(liftLoc,"LiftLc",6,"%6.2f",true);
	   //----- COL 26 ---- 
      workerB.WriteFloat64(fwdLoc,"fwHaul",6,"%6.2f",true);
	   //----- COL 27 ---- 
      workerB.WriteFloat64(trlLoc,"trHaul",6,"%6.2f",false);
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
      write_spec_check_results(fp, pBroker, span, gdr,designSucceeded);
      fprintf(fp, "\n");
      fprintf(fp, "\n");
   }

   return CAD_SUCCESS;
}


void CadWriterWorkerBee::WriteFloat64(Float64 val, const char* title, Int16 nchars, const char* format, bool doDelim)
{
   // title, format, and nchars must match in size. It's your job to deal with this
   ATLASSERT(std::string(title).size()==nchars);

   int nr = sprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   m_DataLineCursor += nr;

   ATLASSERT(nr==nchars);

   if (doDelim)
   {
      nr = sprintf_s(m_DataLineCursor, DataBufferRemaining(), CAD_DELIM);
      m_DataLineCursor += nr;
   }

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, nchars, doDelim);
   }
}

void CadWriterWorkerBee::WriteInt16(Int16 val, const char* title, Int16 nchars, const char* format, bool doDelim)
{
   int nr = sprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   m_DataLineCursor += nr;

   ATLASSERT(nr==nchars);

   if (doDelim)
   {
      nr = sprintf_s(m_DataLineCursor, DataBufferRemaining(), CAD_DELIM);
      m_DataLineCursor += nr;
   }

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, nchars, doDelim);
   }
}

void CadWriterWorkerBee::WriteString(const char* val, const char* title, Int16 nchars, const char* format, bool doDelim)
{
   int nr = sprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   m_DataLineCursor += max(nchars,nr);

//   ATLASSERT(nr==nchars);

   if (doDelim)
   {
      nr = sprintf_s(m_DataLineCursor, DataBufferRemaining(), CAD_DELIM);
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
   memset(m_DataLineCursor,' ',ns);
   m_DataLineCursor += ns;

   if (m_DoWriteTitles)
   {
      memset(m_TitleLineCursor,' ',ns);
      m_TitleLineCursor += ns;

      memset(m_DashLineCursor,' ',ns);
      m_DashLineCursor += ns;
   }
}

void CadWriterWorkerBee::WriteToFile(FILE* fp)
{
   // Now that we've filled up our buffers, write them to the file
   if (m_DoWriteTitles)
   {
      fprintf(fp, "%s", m_TitleLine);
      fprintf(fp, "\n");

      fprintf(fp, "%s", m_DashLine);
      fprintf(fp, "\n");
   }

   fprintf(fp, "%s", m_DataLine);
   fprintf(fp, "\n");
}

CadWriterWorkerBee::CadWriterWorkerBee(bool doWriteTitles):
m_DoWriteTitles(doWriteTitles)
{
    memset(m_DataLine,'\0',BF_SIZ);
    memset(m_TitleLine,'\0',BF_SIZ);
    memset(m_DashLine,'\0',BF_SIZ);

    // First column is blank
    m_DataLine[0]  = ' ';
    m_TitleLine[0] = ' ';
    m_DashLine[0]  = ' ';

    m_DataLineCursor  = m_DataLine+1;
    m_TitleLineCursor = m_TitleLine+1;
    m_DashLineCursor  = m_DashLine+1;
}

void CadWriterWorkerBee::WriteTitle(const char* title, Int16 nchars, bool doDelim)
{
   // Write title line and dash line since
   memcpy(m_TitleLineCursor, title, nchars);
   m_TitleLineCursor += nchars;

   memset(m_DashLineCursor,'-', nchars);
   m_DashLineCursor += nchars;

   if (doDelim)
   {
      memset(m_TitleLineCursor, ' ', 1);
      m_TitleLineCursor++;

      memset(m_DashLineCursor, ' ', 1);
      m_DashLineCursor++;
   }
}

void TxDOTCadWriter::WriteInitialData(CadWriterWorkerBee& workerB)
{
   // first build our data structure
   Compute();

   // next write out data
	//----- COL 10 ---- 
   workerB.WriteInt16((Int16)m_NumDebonded,"Ndb",3,"%3d",true);

   if (m_NumDebonded==0 || m_OutCome==SectionMismatch || m_OutCome==TooManySections || m_OutCome==SectionsNotSymmetrical)
   {
      // row height, srands in row, and debonds in row are zero
	   workerB.WriteFloat64(0.0,"Debnd",5,"%5.2f",true);
      workerB.WriteInt16(0,"   ",3,"%3d",true);
      workerB.WriteInt16(0,"   ",3,"%3d",true);

      if (m_OutCome==SectionMismatch || m_OutCome==TooManySections )
      {
         // this is an error condition, just right out blanks to fill space
   	   //----- COL 11-23 ---- 
         workerB.WriteBlankSpaces(30);
      }
      else if (m_NumDebonded==0)
      {
         // no use searching for nothing
   	   //----- COL 11-23 ---- 
         for (int i=0; i<10; i++)
         {
            workerB.WriteInt16(0,"  ",2,"%2d",true);
         }
      }
   }
   else
   {
      // A little checking
      RowIndexType nrs = m_pStrandGeometry->GetNumRowsWithStrand(m_Span,m_Girder,pgsTypes::Straight);
      ATLASSERT((RowIndexType)m_Rows.size() <= nrs); // could have more rows than rows with debonded strands

      // where the rubber hits the road
      if(!m_Rows.empty())
      {
         // write our first row
         StrandIndexType nsrow = m_pStrandGeometry->GetNumStrandInRow(m_Span,m_Girder,0,pgsTypes::Straight);

         const RowData& row = *(m_Rows.begin());
         WriteRowData(workerB, row, (Int16)nsrow);
      }
      else
      {
         ATLASSERT(0); // this should be caught
      }
   }
}

void TxDOTCadWriter::WriteFinalData(FILE *fp, bool isExtended)
{
   // fist write out remaining rows 
   if(!m_Rows.empty())
   {
      Int16 nLeadingSpaces = isExtended ? 70 : 54; // more leading spaces for extended output
      Int16 nrow = 1;
      RowListIter riter = m_Rows.begin();
      riter++;
      while(riter != m_Rows.end())
      {
         CadWriterWorkerBee workerB(false); // no title lines for last lines

         // leading blank spaces
         workerB.WriteBlankSpaces(nLeadingSpaces);

         // write our first row
         StrandIndexType nsrow = m_pStrandGeometry->GetNumStrandInRow(m_Span,m_Girder,nrow,pgsTypes::Straight);

         const RowData& row = *riter;
         WriteRowData(workerB, row, (Int16)nsrow);

	      // ------ END OF RECORD ----- 
         workerB.WriteToFile(fp);

         riter++;
         nrow++;
      }
   }

   // lastly write any information
   if (m_OutCome==SectionMismatch || m_OutCome==SectionsNotSymmetrical)
   {
	   fprintf(fp, "Warning: Irregular, Non-standard debonding increments used for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n",LABEL_GIRDER(m_Girder),LABEL_SPAN(m_Span));
   }
   else if (m_OutCome==TooManySections)
   {
	   fprintf(fp, "Warning: The number of debonded sections exceeds ten for beam %s in span %2d. Cannot write debonding information to TxDOT CAD format.\n",LABEL_GIRDER(m_Girder),LABEL_SPAN(m_Span));
   }
   else if (m_OutCome==NonStandardSection)
   {
      Float64 spac = ::ConvertFromSysUnits(m_SectionSpacing , unitMeasure::Feet );
	   fprintf(fp, "Warning: Non-standard debonding increment of %6.3f ft used  for beam %s in span %2d. \n",spac,LABEL_GIRDER(m_Girder),LABEL_SPAN(m_Span));
   }
}

void TxDOTCadWriter::WriteRowData(CadWriterWorkerBee& workerB, const RowData& row, Int16 strandsInRow) const
{
	//----- COL 11 ----- 
   // elevation of row
   Float64 row_elev = ::ConvertFromSysUnits( row.m_Elevation, unitMeasure::Inch );

   workerB.WriteFloat64(row_elev,"Elev ",5,"%5.2f",true);

	//----- COL 12 ---- 
   // total strands in row
   workerB.WriteInt16(strandsInRow,"Nsr",3,"%3d",true);

	//----- COL 13 ---- 
   // num debonded strands in row
   Int16 nsr = CountDebondsInRow(row);
   workerB.WriteInt16(nsr,"Ndb",3,"%3d",true);

	//----- COL 14-23 ---- 
   // we have 10 columns to write no matter what
   SectionListConstIter scit = row.m_Sections.begin();

   char buff[4];
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

      sprintf_s(buff,sizeof(buff),"%2d",icol+1);

      workerB.WriteInt16(db_cnt,buff,2,"%2d",true);
   }

   ATLASSERT(scit==row.m_Sections.end()); // we didn't find all of our sections - bug
}


void write_spec_check_results(FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, bool designSucceeded)
{
   fprintf(fp, "\n\n");

   if (!designSucceeded)
   {
      fprintf(fp, "%s\n", "Girder design was Not Successful");
   }

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,gdr);

   if( pArtifact->Passed() )
   {
      fprintf(fp, "%s\n", "The Specification Check was Successful");
   }
   else
   {
      fprintf(fp, "%s\n", "The Specification Check was Not Successful");
     
      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
      bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

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
         fprintf(fp, "%s\n", it->c_str());
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

	char	spanNumber[5+1];
	sprintf_s(spanNumber, sizeof(spanNumber), "%d", LABEL_SPAN(span));

	char  beamNumber[5+1];
	sprintf_s(beamNumber, sizeof(beamNumber), "%s", LABEL_GIRDER(gdr));

   // have our data, now need to write it
   CadWriterWorkerBee workerB(true);

   workerB.WriteString(spanNumber,"Span ",5,"%5s",true);
   workerB.WriteString(beamNumber," Gdr ",5,"%5s",true);

   workerB.WriteFloat64(gpM, " gpM  ",6,"%6.3f",true);
   workerB.WriteFloat64(gpM1," gpM1 ",6,"%6.3f",true);
   workerB.WriteFloat64(gpM2," gpM2 ",6,"%6.3f",true);

   workerB.WriteFloat64(gnM, " gnM  ",6,"%6.2f",true);
   workerB.WriteFloat64(gnM1," gnM1 ",6,"%6.2f",true);
   workerB.WriteFloat64(gnM2," gnM2 ",6,"%6.2f",true);

   workerB.WriteFloat64(gV, "  gV  ",6,"%6.2f",true);
   workerB.WriteFloat64(gV1," gV1  ",6,"%6.2f",true);
   workerB.WriteFloat64(gV2," gV2  ",6,"%6.2f",true);

   workerB.WriteFloat64(gR, "  gR  ",6,"%6.2f",true);
   workerB.WriteFloat64(gR1," gR1  ",6,"%6.2f",true);
   workerB.WriteFloat64(gR2," gR2  ",6,"%6.2f",false);

   workerB.WriteToFile(fp);

   fprintf(fp, "\n");

   return CAD_SUCCESS;
}

std::string MakeNonStandardStrandString(IBroker* pBroker, const pgsPointOfInterest& midPoi)
{
   StrandRowUtil::StrandRowSet strandrows = StrandRowUtil::GetStrandRowSet(pBroker, midPoi);

   // At this point, we have counted the number of strands per row. Now create string
   bool first = true;
   std::stringstream os;
   for (StrandRowUtil::StrandRowIter srit=strandrows.begin(); srit!=strandrows.end(); srit++)
   {
      if (!first)
         os<<",";
      else
         first=false;

      const StrandRowUtil::StrandRow& srow = *srit;
      Float64 elev_in = RoundOff(::ConvertFromSysUnits( srow.Elevation, unitMeasure::Inch ),0.001);
      os<<elev_in<<"("<<srow.Count<<")";
   }

   return os.str();
}