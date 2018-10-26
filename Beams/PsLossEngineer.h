///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// PsLossEngineer.h : Declaration of the CPsLossEngineer

#ifndef __PSLOSSENGINEER_H_
#define __PSLOSSENGINEER_H_

#include "resource.h"       // main symbols
#include <Details.h>
#include <EAF\EAFDisplayUnits.h>

class lrfdLosses;

/////////////////////////////////////////////////////////////////////////////
// CPsLossEngineer
class CPsLossEngineer
{
   enum LossAgency{laWSDOT, laTxDOT, laAASHTO};
public:
 	CPsLossEngineer()
	{
      m_bComputingLossesForDesign = false;
	}

   void Init(IBroker* pBroker,StatusGroupIDType statusGroupID);

public:
   enum BeamType { IBeam, UBeam, SolidSlab, BoxBeam, SingleT };

   virtual LOSSDETAILS ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi);
   virtual LOSSDETAILS ComputeLossesForDesign(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void BuildReport(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

private:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidUnknown;
   StatusCallbackIDType m_scidGirderDescriptionError;
   StatusCallbackIDType m_scidGirderDescriptionWarning;
   StatusCallbackIDType m_scidLRFDVersionError;
   StatusCallbackIDType m_scidConcreteTypeError;

   LOSSDETAILS ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config);

   void LossesByRefinedEstimate(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,LossAgency lossAgency);
   void LossesByRefinedEstimateBefore2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses);
   void LossesByRefinedEstimate2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,LossAgency lossAgency);
   void LossesByApproxLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,bool isWsdot);
   void LossesByGeneralLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses);
   void LossesByRefinedEstimateTxDOT2013(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses);
   lrfdElasticShortening::FcgpComputationMethod LossesByRefinedEstimateTxDOT2013_Compute(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses);

   void ReportRefinedMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,Uint16 level,LossAgency lossAgency);
   void ReportApproxLumpSumMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,Uint16 level,bool isWsdot);
   void ReportGeneralLumpSumMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,bool bDesign,Uint16 level);
   void ReportLumpSumMethod(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,bool bDesign,Uint16 level);

   void ReportRefinedMethodBefore2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
   void ReportRefinedMethod2005(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
   void ReportApproxMethod(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level,bool isWsdot);
   void ReportApproxMethod2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
   void ReportRefinedMethodTxDOT2013(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

   typedef std::set< std::pair<pgsPointOfInterest,IntervalIndexType> > PoiSet;
   typedef std::vector<pgsPointOfInterest> PoiVector;

   void ReportLocation( rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,IEAFDisplayUnits* pDisplayUnits);
   void ReportLocation2(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,IEAFDisplayUnits* pDisplayUnits);

   void ReportInitialRelaxation(rptChapter* pChapter,bool bTemporaryStrands,const lrfdLosses* pLosses,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
   void ReportLumpSumTimeDependentLossesAtShipping(rptChapter* pChapter,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
   void ReportLumpSumTimeDependentLosses(rptChapter* pChapter,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

   void ReportTotalPrestressLoss(rptChapter* pChapter,const CSegmentKey& segmentKey,PoiVector::iterator begin,PoiVector::iterator end,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

   void GetLossParameters(const pgsPointOfInterest& poi,const GDRCONFIG& config,
                           lrfdLosses::SectionPropertiesType* pSectionProperties,
                           matPsStrand::Grade* pGradePerm,
                           matPsStrand::Type* pTypePerm,
                           matPsStrand::Coating* pCoatingPerm,
                           matPsStrand::Grade* pGradeTemp,
                           matPsStrand::Type* pTypeTemp,
                           matPsStrand::Coating* pCoatingTemp,
                           Float64* pFpjPerm,
                           Float64* pFpjTTS,
                           Float64* pPerimeter,
                           Float64* pAg,
                           Float64* pIg,
                           Float64* pYbg,
                           Float64* pAc,
                           Float64* pIc,
                           Float64* pYbc,
                           Float64* pAn,
                           Float64* pIn,
                           Float64* pYbn,
                           Float64* pAcn,
                           Float64* pIcn,
                           Float64* pYbcn,
                           Float64* pVolume,
                           Float64* pSurfaceArea,
                           Float64* pAd,
                           Float64* ped,
                           Float64* pKsh,
                           Float64* pepermRelease,// eccentricity of the permanent strands on the non-composite section
                           Float64* pepermFinal,
                           Float64* petemp,
                           Float64* paps,  // area of one prestress strand
                           Float64* pApsPerm,
                           Float64* pApsTTS,
                           Float64* pMdlg,
                           Float64* pMadlg,
                           Float64* pMsidl,
                           Float64* pMllim,
                           Float64* prh,
                           Float64* pti,
                           Float64* pth,
                           Float64* ptd,
                           Float64* ptf,
                           Float64* pPjS,
                           Float64* pPjH,
                           Float64* pPjT,
                           StrandIndexType* pNs,
                           StrandIndexType* pNh,
                           StrandIndexType* pNt,
                           Float64* pGdrCreepK1,
                           Float64* pGdrCreepK2,
                           Float64* pGdrShrinkageK1,
                           Float64* pGdrShrinkageK2,
                           Float64* pDeckCreepK1,
                           Float64* pDeckCreepK2,
                           Float64* pDeckShrinkageK1,
                           Float64* pDeckShrinkageK2,
                           Float64* pFci,
                           Float64* pFc,
                           Float64* pFcSlab,
                           Float64* pEci,
                           Float64* pEc,
                           Float64* pEcSlab,
                           Float64* pGirderLength,
                           Float64* pSpanLength,
                           Float64* pAslab,
                           Float64* pPslab,
                           lrfdLosses::TempStrandUsage* pUsage,
                           Float64* pAnchorSet,
                           Float64* pWobble,
                           Float64* pCoeffFriction,
                           Float64* pAngleChange
                           );

   void ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,LossAgency lossAgency);
   void ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits);
   void ReportFinalLossesRefinedMethodBefore2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits);

   std::vector<pgsPointOfInterest> GetPointsOfInterest(const CGirderKey& girderKey);

   bool m_bComputingLossesForDesign; 
};

#endif //__PSLOSSENGINEER_H_
