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


// Miscellaneous helper functions
#pragma once

#include <Beams\BeamsExp.h>

#include <IFace\BeamFamily.h>

#include <IFace\BeamFactory.h>

#include <EAF\EAFDisplayUnits.h>
#include <LRFD\LiveLoadDistributionFactorBase.h>

#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\GirderGroupData.h>
#include <IFace\AgeAdjustedMaterial.h>

class rptParagraph;
class pgsPoiMgr;

void BEAMSFUNC ReportLeverRule(rptParagraph* pPara,bool isMoment, Float64 specialFactor, lrfdILiveLoadDistributionFactor::LeverRuleMethod& lrd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits);
void BEAMSFUNC ReportRigidMethod(rptParagraph* pPara,lrfdILiveLoadDistributionFactor::RigidMethod& rd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits);
void BEAMSFUNC ReportLanesBeamsMethod(rptParagraph* pPara,lrfdILiveLoadDistributionFactor::LanesBeamsMethod& rd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits);

IndexType BEAMSFUNC GetBeamTypeCount();
CLSID BEAMSFUNC GetBeamCLSID(IndexType idx);
CATID BEAMSFUNC GetBeamCATID(IndexType idx);

void BEAMSFUNC BuildAgeAdjustedGirderMaterialModel(IBroker* pBroker,const CPrecastSegmentData* pSegment,ISuperstructureMemberSegment* segment,IAgeAdjustedMaterial** ppMaterial);
void BEAMSFUNC BuildAgeAdjustedJointMaterialModel(IBroker* pBroker, const CPrecastSegmentData* pSegment, ISuperstructureMemberSegment* segment, IAgeAdjustedMaterial** ppMaterial);

void BEAMSFUNC MakeRectangle(Float64 width, Float64 depth, Float64 xOffset, Float64 yOffset,IShape** ppShape);

bool BEAMSFUNC IsInEndBlock(Float64 Xs, pgsTypes::SectionBias sectionBias, Float64 leftEndBlockLength, Float64 rightEndBlockLength, Float64 Lg);
bool BEAMSFUNC IsInEndBlock(Float64 Xs, pgsTypes::SectionBias sectionBias, Float64 endBlockLength, Float64 Lg);

bool BEAMSFUNC IsSupportedDeckType(pgsTypes::SupportedDeckType deckType, const IBeamFactory* pFactory, pgsTypes::SupportedBeamSpacing spacingType);

void BEAMSFUNC LayoutIBeamEndBlockPointsOfInterest(const CSegmentKey& segmentKey, const CPrecastSegmentData* pSegment, Float64 segmentLength, pgsPoiMgr* pPoiMgr);


/////////////////////////////////////////////////////////////////////////////
// IBeamFamilyImpl
class BEAMSCLASS IBeamFamilyImpl :
   public IBeamFamily
{
public:
   // IBeamFamily
   virtual CString GetName() override;
   virtual void RefreshFactoryList() override;
   virtual const std::vector<CString>& GetFactoryNames() override;
   virtual CLSID GetFactoryCLSID(LPCTSTR strName) override;
   virtual HRESULT CreateFactory(LPCTSTR strName, IBeamFactory** ppFactory) override;

protected:
   HRESULT Init();

   virtual const CLSID& GetCLSID() = 0;
   virtual const CATID& GetCATID() = 0;

   typedef std::map<CString, CLSID> FactoryContainer;
   FactoryContainer m_Factories;
   std::vector<CString> m_Names;
};
