///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#pragma once

#include <Details.h>
#include <EAF\EAFDisplayUnits.h>

class WBFL::LRFD::Losses;

namespace WBFL
{
   namespace EAF
   {
      class Broker;
   };
};

namespace PGS
{
   namespace Beams
   {
      class PsLossEngineer
      {
         enum LossAgency{laWSDOT, laTxDOT, laAASHTO};
      public:
         PsLossEngineer() = delete;
         PsLossEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID);
         PsLossEngineer(const PsLossEngineer&) = delete;
         ~PsLossEngineer() = default;

         PsLossEngineer& operator=(const PsLossEngineer&) = delete;

      public:
         enum BeamType { IBeam, UBeam, SolidSlab, BoxBeam, SingleT };

         virtual LOSSDETAILS ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi);
         virtual LOSSDETAILS ComputeLossesForDesign(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config);
         virtual void BuildReport(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
         virtual void ReportFinalLosses(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

      private:
         std::shared_ptr<WBFL::EAF::Broker> GetBroker() { return m_pBroker.lock(); }
         std::weak_ptr<WBFL::EAF::Broker> m_pBroker;
         StatusGroupIDType m_StatusGroupID;
         StatusCallbackIDType m_scidUnknown;
         StatusCallbackIDType m_scidGirderDescriptionError;
         StatusCallbackIDType m_scidGirderDescriptionWarning;
         StatusCallbackIDType m_scidLRFDVersionError;
         StatusCallbackIDType m_scidConcreteTypeError;

         LOSSDETAILS ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig);

         void LossesByRefinedEstimate(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses,LossAgency lossAgency);
         void LossesByRefinedEstimateBefore2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses);
         void LossesByRefinedEstimate2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses,LossAgency lossAgency);
         void LossesByApproxLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses,bool isWsdot);
         void LossesByGeneralLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses);
         void LossesByRefinedEstimateTxDOT2013(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses);
         WBFL::LRFD::ElasticShortening::FcgpComputationMethod LossesByRefinedEstimateTxDOT2013_Compute(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses);

         void ReportRefinedMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level,LossAgency lossAgency);
         void ReportApproxLumpSumMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level,bool isWsdot);
         void ReportGeneralLumpSumMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,bool bDesign,Uint16 level);
         void ReportLumpSumMethod(rptChapter* pChapter,PsLossEngineer::BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,bool bDesign,Uint16 level);

         void ReportRefinedMethodBefore2005(rptChapter* pChapter,PsLossEngineer::BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);
         void ReportRefinedMethod2005(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);
         void ReportApproxMethod(rptChapter* pChapter,PsLossEngineer::BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level,bool isWsdot);
         void ReportApproxMethod2005(rptChapter* pChapter,PsLossEngineer::BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);
         void ReportRefinedMethodTxDOT2013(rptChapter* pChapter,PsLossEngineer::BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);

         void ReportLocation( rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
         void ReportLocation2(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

         void ReportInitialRelaxation(rptChapter* pChapter,bool bTemporaryStrands,const WBFL::LRFD::Losses* pLosses,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);
         void ReportLumpSumTimeDependentLossesAtShipping(rptChapter* pChapter,const LOSSDETAILS* pDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);
         void ReportLumpSumTimeDependentLosses(rptChapter* pChapter,const LOSSDETAILS* pDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);

         void GetLossParameters(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,
                                 WBFL::LRFD::Losses::SectionPropertiesType* pSectionProperties,
                                 WBFL::Materials::PsStrand::Grade* pGradePerm,
                                 WBFL::Materials::PsStrand::Type* pTypePerm,
                                WBFL::Materials::PsStrand::Coating* pCoatingPerm,
                                 WBFL::Materials::PsStrand::Grade* pGradeTemp,
                                 WBFL::Materials::PsStrand::Type* pTypeTemp,
                                WBFL::Materials::PsStrand::Coating* pCoatingTemp,
                                 Float64* pFpjPerm,
                                 Float64* pFpjTTS,
                                 Float64* pPerimeter,
                                 Float64* pAg,
                                 Float64* pIxx,
                                 Float64* pIyy,
                                 Float64* pIxy,
                                 Float64* pYbg,
                                 Float64* pAc1,
                                 Float64* pIc1,
                                 Float64* pYbc1,
                                 Float64* pAc2,
                                 Float64* pIc2,
                                 Float64* pYbc2,
                                 Float64* pAn,
                                 Float64* pIxxn,
                                 Float64* pIyyn,
                                 Float64* pIxyn,
                                 Float64* pYbn,
                                 Float64* pAcn,
                                 Float64* pIcn,
                                 Float64* pYbcn,
                                 Float64* pAd,
                                 Float64* ped,
                                 Float64* pKsh,
                                 WBFL::Geometry::Point2d* pepermRelease,// eccentricity of the permanent strands on the non-composite section
                                 WBFL::Geometry::Point2d* pepermFinal,
                                 WBFL::Geometry::Point2d* petemp,
                                 Float64* paps,  // area of one prestress strand
                                 Float64* pApsPerm,
                                 Float64* pApsTTS,
                                 Float64* pMdlg,
                                 std::vector<std::pair<Float64, Float64>>* pMadlg,
                                 std::vector<std::pair<Float64, Float64>>* pMsidl1,
                                 std::vector<std::pair<Float64, Float64>>* pMsidl2,
                                 Float64* prh,
                                 Float64* pti,
                                 Float64* pth,
                                 Float64* ptd,
                                 Float64* ptf,
                                 Float64* pPjS,
                                 Float64* pPjH,
                                 Float64* pPjT,
                                 Float64* pGdrShrinkageK1,
                                 Float64* pGdrShrinkageK2,
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
                                 WBFL::LRFD::Losses::TempStrandUsage* pUsage,
                                 Float64* pAnchorSet,
                                 Float64* pWobble,
                                 Float64* pCoeffFriction,
                                 Float64* pAngleChange
                                 );

         void ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,LossAgency lossAgency);
         void ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
         void ReportFinalLossesRefinedMethodBefore2005(rptChapter* pChapter,PsLossEngineer::BeamType beamType,const CGirderKey& girderKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

         void GetPointsOfInterest(const CGirderKey& girderKey,PoiList* pPoiList);

         bool m_bComputingLossesForDesign = false;
      };
   };
};
