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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SegmentDesignArtifact.h>
#include <WBFLCore.h>
#include <IFace\Tools.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsSegmentDesignArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsSegmentDesignArtifact::pgsSegmentDesignArtifact()
{
   Init();
}

pgsSegmentDesignArtifact::pgsSegmentDesignArtifact(const CSegmentKey& segmentKey) :
m_SegmentKey(segmentKey)
{
   Init();
}

pgsSegmentDesignArtifact::pgsSegmentDesignArtifact(const pgsSegmentDesignArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsSegmentDesignArtifact::~pgsSegmentDesignArtifact()
{
}

//======================== OPERATORS  =======================================
pgsSegmentDesignArtifact& pgsSegmentDesignArtifact::operator= (const pgsSegmentDesignArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsSegmentDesignArtifact::SetOutcome(pgsSegmentDesignArtifact::Outcome outcome)
{
   m_Outcome = outcome;
}

pgsSegmentDesignArtifact::Outcome pgsSegmentDesignArtifact::GetOutcome() const
{
   return m_Outcome;
}

void pgsSegmentDesignArtifact::AddDesignNote(pgsSegmentDesignArtifact::DesignNote note)
{
   m_DesignNotes.push_back(note);
}

bool pgsSegmentDesignArtifact::DoDesignNotesExist() const
{
   return !m_DesignNotes.empty();
}

std::vector<pgsSegmentDesignArtifact::DesignNote> pgsSegmentDesignArtifact::GetDesignNotes() const
{
   return m_DesignNotes;
}

bool pgsSegmentDesignArtifact::DoPreviouslyFailedDesignsExist() const
{
   return !m_PreviouslyFailedDesigns.empty();
}

void pgsSegmentDesignArtifact::AddFailedDesign(const arDesignOptions& options)
{
   m_PreviouslyFailedDesigns.push_back(options.doDesignForFlexure);
}

void pgsSegmentDesignArtifact::UpdateFailedFlexuralDesigns(const pgsSegmentDesignArtifact& artifact) const
{
   const std::vector<arFlexuralDesignType>& vFails(artifact.GetPreviouslyFailedFlexuralDesigns());
   std::vector<arFlexuralDesignType>::const_reverse_iterator iter(vFails.rbegin());
   std::vector<arFlexuralDesignType>::const_reverse_iterator end(vFails.rend());
   for ( ; iter != end; iter++ )
   {
      arFlexuralDesignType failedDesignType = *iter;
      m_PreviouslyFailedDesigns.insert(m_PreviouslyFailedDesigns.begin(),failedDesignType);
   }
}

const std::vector<arFlexuralDesignType>& pgsSegmentDesignArtifact::GetPreviouslyFailedFlexuralDesigns() const
{
   return m_PreviouslyFailedDesigns;
}

const CSegmentKey& pgsSegmentDesignArtifact::GetSegmentKey() const
{
   return m_SegmentKey;
}

void pgsSegmentDesignArtifact::SetDesignOptions(const arDesignOptions& options)
{
   m_DesignOptions = options;
}

const arDesignOptions& pgsSegmentDesignArtifact::GetDesignOptions() const
{
   return m_DesignOptions;
}

arFlexuralDesignType pgsSegmentDesignArtifact::GetDoDesignFlexure() const
{
   return m_DesignOptions.doDesignForFlexure;
}

bool pgsSegmentDesignArtifact::GetDoDesignShear() const
{
   return m_DesignOptions.doDesignForShear != sdtNoDesign;
}

void pgsSegmentDesignArtifact::SetNumStraightStrands(StrandIndexType Ns)
{
   m_Ns = Ns;
}

StrandIndexType pgsSegmentDesignArtifact::GetNumStraightStrands() const
{
   return m_Ns;
}

void pgsSegmentDesignArtifact::SetNumTempStrands(StrandIndexType Nt)
{
   m_Nt = Nt;
}

StrandIndexType pgsSegmentDesignArtifact::GetNumTempStrands() const
{
   return m_Nt;
}

void pgsSegmentDesignArtifact::SetNumHarpedStrands(StrandIndexType Nh)
{
   m_Nh = Nh;
}

StrandIndexType pgsSegmentDesignArtifact::GetNumHarpedStrands() const
{
   if(dtDesignFullyBondedRaised==m_DesignOptions.doDesignForFlexure || 
      dtDesignForDebondingRaised==m_DesignOptions.doDesignForFlexure)
   {
      return PRESTRESSCONFIG::CountStrandsInFill(m_RaisedAdjustableStrandFill);
   }
   else
   {
      return m_Nh;
   }
}

pgsTypes::AdjustableStrandType pgsSegmentDesignArtifact::GetAdjustableStrandType() const
{
   // Strand type is based on design type
   if (m_DesignOptions.doDesignForFlexure==dtDesignForHarping)
   {
      return pgsTypes::asHarped;
   }
   else if (m_DesignOptions.doDesignForFlexure==dtDesignForDebonding || 
            m_DesignOptions.doDesignForFlexure==dtDesignFullyBonded  ||
            m_DesignOptions.doDesignForFlexure==dtDesignFullyBondedRaised ||
            m_DesignOptions.doDesignForFlexure==dtDesignForDebondingRaised)
   {
      return pgsTypes::asStraight;
   }
   else
   {
      ATLASSERT(m_DesignOptions.doDesignForFlexure==dtNoDesign); // This can happen during a pure shear design. Otherwise we have a new design type?
      return pgsTypes::asStraight;
   }
}

void pgsSegmentDesignArtifact::SetRaisedAdjustableStrands(const ConfigStrandFillVector& strandFill)
{
   ATLASSERT(dtDesignFullyBondedRaised==m_DesignOptions.doDesignForFlexure || 
             dtDesignForDebondingRaised==m_DesignOptions.doDesignForFlexure);

   m_RaisedAdjustableStrandFill = strandFill;
}

ConfigStrandFillVector pgsSegmentDesignArtifact::GetRaisedAdjustableStrands() const
{
   ATLASSERT(dtDesignFullyBondedRaised==m_DesignOptions.doDesignForFlexure ||
             dtDesignForDebondingRaised==m_DesignOptions.doDesignForFlexure);

   return m_RaisedAdjustableStrandFill;
}

void pgsSegmentDesignArtifact::SetPjackStraightStrands(Float64 Pj)
{
   m_PjS = Pj;
}

Float64 pgsSegmentDesignArtifact::GetPjackStraightStrands() const
{
   return m_PjS;
}

void pgsSegmentDesignArtifact::SetUsedMaxPjackStraightStrands(bool usedMax)
{
   m_PjSUsedMax = usedMax;
}

bool pgsSegmentDesignArtifact::GetUsedMaxPjackStraightStrands() const
{
   return m_PjSUsedMax;
}

void pgsSegmentDesignArtifact::SetPjackTempStrands(Float64 Pj)
{
   m_PjT = Pj;
}

Float64 pgsSegmentDesignArtifact::GetPjackTempStrands() const
{
   return m_PjT;
}

void pgsSegmentDesignArtifact::SetUsedMaxPjackTempStrands(bool usedMax)
{
   m_PjTUsedMax = usedMax;
}

bool pgsSegmentDesignArtifact::GetUsedMaxPjackTempStrands() const
{
   return m_PjTUsedMax;
}

void pgsSegmentDesignArtifact::SetPjackHarpedStrands(Float64 Pj)
{
   m_PjH = Pj;
}

Float64 pgsSegmentDesignArtifact::GetPjackHarpedStrands() const
{
   return m_PjH;
}

void pgsSegmentDesignArtifact::SetUsedMaxPjackHarpedStrands(bool usedMax)
{
   m_PjHUsedMax = usedMax;
}

bool pgsSegmentDesignArtifact::GetUsedMaxPjackHarpedStrands() const
{
   return m_PjHUsedMax;
}

void pgsSegmentDesignArtifact::SetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType,Float64 oe)
{
   m_HarpStrandOffsetEnd[endType] = oe;
}

Float64 pgsSegmentDesignArtifact::GetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType) const
{
   return m_HarpStrandOffsetEnd[endType];
}

void pgsSegmentDesignArtifact::SetHarpStrandOffsetHp(pgsTypes::MemberEndType endType,Float64 ohp)
{
   m_HarpStrandOffsetHp[endType] = ohp;
}

Float64 pgsSegmentDesignArtifact::GetHarpStrandOffsetHp(pgsTypes::MemberEndType endType) const
{
   return m_HarpStrandOffsetHp[endType];
}

DebondConfigCollection pgsSegmentDesignArtifact::GetStraightStrandDebondInfo() const
{
   return m_SsDebondInfo;
}

void pgsSegmentDesignArtifact::SetStraightStrandDebondInfo(const DebondConfigCollection& dbinfo)
{
   m_SsDebondInfo = dbinfo;
}

void pgsSegmentDesignArtifact::ClearDebondInfo()
{
   m_SsDebondInfo.clear();
}

void pgsSegmentDesignArtifact::SetReleaseStrength(Float64 fci)
{
   m_Fci = fci;
}

Float64 pgsSegmentDesignArtifact::GetReleaseStrength() const
{
   return m_Fci;
}

void pgsSegmentDesignArtifact::SetConcrete(matConcreteEx concrete)
{
   m_Concrete = concrete;
}

const matConcreteEx& pgsSegmentDesignArtifact::GetConcrete() const
{
   return m_Concrete;
}

void pgsSegmentDesignArtifact::SetConcreteStrength(Float64 fc)
{
   m_Concrete.SetFc(fc);

   // update Ec if not input by user
   if (!m_IsUserEc)
   {
      Float64 density = m_Concrete.GetDensity();
      Float64 Ec = lrfdConcreteUtil::ModE(m_Concrete.GetType(),fc,density,false);
      m_Concrete.SetE(Ec);
   }
}

Float64 pgsSegmentDesignArtifact::GetConcreteStrength() const
{
   return m_Concrete.GetFc();
}

void pgsSegmentDesignArtifact::SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset)
{
   m_SlabOffset[end] = offset;
}

Float64 pgsSegmentDesignArtifact::GetSlabOffset(pgsTypes::MemberEndType end) const
{
   return m_SlabOffset[end];
}

void pgsSegmentDesignArtifact::SetAssumedExcessCamber(Float64 f)
{
   m_AssumedExcessCamber = f;
}

Float64 pgsSegmentDesignArtifact::GetAssumedExcessCamber() const
{
   return m_AssumedExcessCamber;
}

void pgsSegmentDesignArtifact::SetLiftingLocations(Float64 left,Float64 right)
{
   m_LiftLocLeft  = left;
   m_LiftLocRight = right;
}

Float64 pgsSegmentDesignArtifact::GetLeftLiftingLocation() const
{
   return m_LiftLocLeft;
}

Float64 pgsSegmentDesignArtifact::GetRightLiftingLocation() const
{
   return m_LiftLocRight;
}

void pgsSegmentDesignArtifact::SetTruckSupportLocations(Float64 left,Float64 right)
{
   m_ShipLocLeft  = left;
   m_ShipLocRight = right;
}

Float64 pgsSegmentDesignArtifact::GetLeadingOverhang() const
{
   return m_ShipLocRight;
}

Float64 pgsSegmentDesignArtifact::GetTrailingOverhang() const
{
   return m_ShipLocLeft;
}

void pgsSegmentDesignArtifact::SetHaulTruck(LPCTSTR lpszHaulTruck)
{
   m_strHaulTruck = lpszHaulTruck;
}

LPCTSTR pgsSegmentDesignArtifact::GetHaulTruck() const
{
   return m_strHaulTruck.c_str();
}

void pgsSegmentDesignArtifact::SetTemporaryStrandUsage(pgsTypes::TTSUsage usage)
{
   m_TTSUsage = usage;
}

pgsTypes::TTSUsage pgsSegmentDesignArtifact::GetTemporaryStrandUsage() const
{
   return m_TTSUsage;
}

GDRCONFIG pgsSegmentDesignArtifact::GetSegmentConfiguration() const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   GDRCONFIG config;
   config.SegmentKey = m_SegmentKey;

   config.PrestressConfig.SetStrandFill(pgsTypes::Straight,  pStrandGeometry->ComputeStrandFill(m_SegmentKey, pgsTypes::Straight,  GetNumStraightStrands()));

   // Raised designs store strand fill directly
   if(dtDesignFullyBondedRaised  == m_DesignOptions.doDesignForFlexure || 
      dtDesignForDebondingRaised == m_DesignOptions.doDesignForFlexure)
   {
      config.PrestressConfig.SetStrandFill(pgsTypes::Harped, m_RaisedAdjustableStrandFill );
   }
   else
   {
      config.PrestressConfig.SetStrandFill(pgsTypes::Harped, pStrandGeometry->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, GetNumHarpedStrands()));
   }

   config.PrestressConfig.SetStrandFill(pgsTypes::Temporary, pStrandGeometry->ComputeStrandFill(m_SegmentKey, pgsTypes::Temporary, GetNumTempStrands()));

   config.PrestressConfig.Pjack[pgsTypes::Straight]  = GetPjackStraightStrands();
   config.PrestressConfig.Pjack[pgsTypes::Harped]    = GetPjackHarpedStrands();
   config.PrestressConfig.Pjack[pgsTypes::Temporary] = GetPjackTempStrands();

   for ( int i = 0; i < 2; i++ )
   {
      config.PrestressConfig.EndOffset[i] = GetHarpStrandOffsetEnd((pgsTypes::MemberEndType)i);
      config.PrestressConfig.HpOffset[i]  = GetHarpStrandOffsetHp((pgsTypes::MemberEndType)i);
   }

   config.PrestressConfig.AdjustableStrandType = GetAdjustableStrandType();
   
   config.PrestressConfig.Debond[pgsTypes::Straight] = m_SsDebondInfo; // we only design debond for straight strands

   config.fci       = GetReleaseStrength();
   config.fc        = GetConcreteStrength();
   config.fc28      = GetConcreteStrength();
   config.ConcType  = (pgsTypes::ConcreteType)m_Concrete.GetType();
   config.bHasFct   = m_Concrete.HasAggSplittingStrength();
   config.Fct       = m_Concrete.GetAggSplittingStrength();

   config.PrestressConfig.TempStrandUsage = GetTemporaryStrandUsage();

   // allow moduli to be computed
   config.bUserEci = m_IsUserEci;
   if(m_IsUserEci)
   {
      config.Eci      = m_UserEci;
   }
   else
   {
      config.Eci = lrfdConcreteUtil::ModE( (matConcrete::Type)config.ConcType, config.fci, m_Concrete.GetDensity(), false);
   }

   config.bUserEc  = m_IsUserEc;
   if(m_IsUserEc)
   {
      config.Ec       = m_UserEc;
   }
   else
   {
      config.Ec = lrfdConcreteUtil::ModE((matConcrete::Type)config.ConcType, config.fc, m_Concrete.GetDensity(), false);
   }

   config.SlabOffset[pgsTypes::metStart] = GetSlabOffset(pgsTypes::metStart);
   config.SlabOffset[pgsTypes::metEnd]   = GetSlabOffset(pgsTypes::metEnd);

   config.AssumedExcessCamber = GetAssumedExcessCamber();

   WriteShearDataToStirrupConfig(&m_ShearData, config.StirrupConfig);

   return config;
}

CPrecastSegmentData pgsSegmentDesignArtifact::GetSegmentData() const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   CPrecastSegmentData segmentData = *(pIBridgeDesc->GetPrecastSegmentData(m_SegmentKey));

   if (GetDesignOptions().doDesignForFlexure != dtNoDesign)
   {
      ModSegmentDataForFlexureDesign(pBroker,&segmentData);
   }

   if (GetDesignOptions().doDesignForShear)
   {
      ModSegmentDataForShearDesign(pBroker,&segmentData);
   }

   return segmentData;
}

ZoneIndexType pgsSegmentDesignArtifact::GetNumberOfStirrupZonesDesigned() const
{
   return m_NumShearZones;
}

const CShearData2* pgsSegmentDesignArtifact::GetShearData() const
{
   return &m_ShearData;
}

void pgsSegmentDesignArtifact::SetNumberOfStirrupZonesDesigned(ZoneIndexType num)
{
   m_NumShearZones = num;
}

void pgsSegmentDesignArtifact::SetShearData(const CShearData2& rdata)
{
   m_ShearData = rdata;
}

void pgsSegmentDesignArtifact::SetWasLongitudinalRebarForShearDesigned(bool isTrue)
{
   m_bWasLongitudinalRebarForShearDesigned = isTrue;
}

bool pgsSegmentDesignArtifact::GetWasLongitudinalRebarForShearDesigned() const
{
   return m_bWasLongitudinalRebarForShearDesigned;
}


CLongitudinalRebarData& pgsSegmentDesignArtifact::GetLongitudinalRebarData() 
{
   return m_LongitudinalRebarData;
}

const CLongitudinalRebarData& pgsSegmentDesignArtifact::GetLongitudinalRebarData() const
{
   return m_LongitudinalRebarData;
}

void pgsSegmentDesignArtifact::SetLongitudinalRebarData(const CLongitudinalRebarData& rdata)
{
   m_LongitudinalRebarData = rdata;
}

void pgsSegmentDesignArtifact::SetUserEc(Float64 Ec)
{
   PRECONDITION(Ec>0.0);
   m_IsUserEc = true;
   m_UserEc   = Ec;
}

void pgsSegmentDesignArtifact::SetUserEci(Float64 Eci)
{
   PRECONDITION(Eci>0.0);
   m_IsUserEci = true;
   m_UserEci   = Eci;
}


const pgsSegmentDesignArtifact::ConcreteStrengthDesignState& pgsSegmentDesignArtifact::GetReleaseDesignState() const
{
   return m_ConcreteReleaseDesignState;
}

const pgsSegmentDesignArtifact::ConcreteStrengthDesignState& pgsSegmentDesignArtifact::GetFinalDesignState() const
{
   return m_ConcreteFinalDesignState;
}

void pgsSegmentDesignArtifact::SetReleaseDesignState(const ConcreteStrengthDesignState& state)
{
   ATLASSERT(state.Interval() != INVALID_INDEX);
   m_ConcreteReleaseDesignState = state;
}

void pgsSegmentDesignArtifact::SetFinalDesignState(const ConcreteStrengthDesignState& state)
{
   ATLASSERT(state.Interval() != INVALID_INDEX);
   m_ConcreteFinalDesignState = state;
}

void pgsSegmentDesignArtifact::ConcreteStrengthDesignState::SetStressState(bool controlledByMin, const CSegmentKey& segmentKey,IntervalIndexType intervalIdx, pgsTypes::StressType stressType, 
                    pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
{
   ATLASSERT(intervalIdx != INVALID_INDEX);
   m_Action = actStress;
   m_MinimumControls = controlledByMin;
   m_SegmentKey      = segmentKey;
   m_IntervalIdx     = intervalIdx;
   m_StressType = stressType;
   m_LimitState = limitState;
   m_StressLocation = stressLocation;
}

void pgsSegmentDesignArtifact::ConcreteStrengthDesignState::SetShearState(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx, pgsTypes::LimitState limitState)
{
   ATLASSERT(intervalIdx != INVALID_INDEX);
   m_Action = actShear;
   m_MinimumControls = false;
   m_SegmentKey      = segmentKey;
   m_IntervalIdx     = intervalIdx;
   m_LimitState = limitState;
}

bool pgsSegmentDesignArtifact::ConcreteStrengthDesignState::WasControlledByMinimum() const
{
   return m_MinimumControls;
}

pgsSegmentDesignArtifact::ConcreteStrengthDesignState::Action pgsSegmentDesignArtifact::ConcreteStrengthDesignState::GetAction() const
{
   return m_Action;
}

IntervalIndexType pgsSegmentDesignArtifact::ConcreteStrengthDesignState::Interval() const
{
   return m_IntervalIdx;
}

pgsTypes::StressType pgsSegmentDesignArtifact::ConcreteStrengthDesignState::StressType() const
{
   PRECONDITION(m_MinimumControls==false && m_Action==actStress);
   return m_StressType;
}

pgsTypes::LimitState pgsSegmentDesignArtifact::ConcreteStrengthDesignState::LimitState() const
{
   PRECONDITION(m_MinimumControls==false);
   return m_LimitState;
}

pgsTypes::StressLocation pgsSegmentDesignArtifact::ConcreteStrengthDesignState::StressLocation() const
{
   PRECONDITION(m_MinimumControls==false && m_Action==actStress);
   return m_StressLocation;
}

void pgsSegmentDesignArtifact::ConcreteStrengthDesignState::SetRequiredAdditionalRebar(bool wasReqd)
{
   m_RequiredAdditionalRebar = wasReqd;
}

bool pgsSegmentDesignArtifact::ConcreteStrengthDesignState::GetRequiredAdditionalRebar() const
{
#if defined _DEBUG
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   ATLASSERT(m_IntervalIdx <= pIntervals->GetHaulSegmentInterval(m_SegmentKey));
#endif
   return m_RequiredAdditionalRebar;
}

std::_tstring pgsSegmentDesignArtifact::ConcreteStrengthDesignState::AsString() const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   std::_tstring strDesc = pIntervals->GetDescription(m_IntervalIdx);

   if (m_MinimumControls)
   {
      return std::_tstring(_T("Minimum"));
   }
   else if (m_Action==actStress)
   {
      std::_tostringstream sstr;
      sstr<< _T("flexural stress in Interval ") << LABEL_INTERVAL(m_IntervalIdx) << _T(" ") << strDesc << _T(", ") << GetLimitStateString(m_LimitState)<<_T(", ") << GetStressTypeString(m_StressType)<<_T(", at ") << GetStressLocationString(m_StressLocation);
      return sstr.str();
   }
   else if (m_Action==actShear)
   {
      std::_tostringstream sstr;
      sstr<< _T("ultimate shear stress in Interval ") << LABEL_INTERVAL(m_IntervalIdx) << _T(" ") << strDesc << _T(", ") << GetLimitStateString(m_LimitState);
      return sstr.str();
   }
   else
   {
      ATLASSERT(false);
      return std::_tstring(_T("unknown design state. "));
   }
}

bool pgsSegmentDesignArtifact::ConcreteStrengthDesignState::operator==(const ConcreteStrengthDesignState& rOther) const
{
   if (m_MinimumControls!=rOther.m_MinimumControls)
   {
      return false;
   }
   else if (m_MinimumControls==true)
   {
      // both are true
      return true;
   }
   else if (m_Action != rOther.m_Action)
   {
      return false;
   }
   else
   {
      if (m_Action==actStress)
      {
         return m_IntervalIdx==rOther.m_IntervalIdx && m_StressType==rOther.m_StressType &&
                m_LimitState==rOther.m_LimitState && m_StressLocation==rOther.m_StressLocation &&
                m_RequiredAdditionalRebar==rOther.m_RequiredAdditionalRebar;
      }
      else
      {
         ATLASSERT(rOther.m_Action==actShear);
         return m_IntervalIdx==rOther.m_IntervalIdx && m_LimitState==rOther.m_LimitState;
      }
   }
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsSegmentDesignArtifact::MakeCopy(const pgsSegmentDesignArtifact& rOther)
{
   m_Outcome = rOther.m_Outcome;

   m_DesignNotes = rOther.m_DesignNotes;

   m_SegmentKey = rOther.m_SegmentKey;

   m_DesignOptions  = rOther.m_DesignOptions;

   m_TTSUsage = rOther.m_TTSUsage;
   m_Ns                  = rOther.m_Ns;
   m_Nh                  = rOther.m_Nh;
   m_RaisedAdjustableStrandFill = rOther.m_RaisedAdjustableStrandFill;
   m_Nt                  = rOther.m_Nt;
   m_PjS                 = rOther.m_PjS;
   m_PjH                 = rOther.m_PjH;
   m_PjT                 = rOther.m_PjT;
   m_PjSUsedMax          = rOther.m_PjSUsedMax;
   m_PjHUsedMax          = rOther.m_PjHUsedMax;
   m_PjTUsedMax          = rOther.m_PjTUsedMax;

   m_Concrete            = rOther.m_Concrete;
   m_LiftLocLeft         = rOther.m_LiftLocLeft;
   m_LiftLocRight        = rOther.m_LiftLocRight;
   m_ShipLocLeft         = rOther.m_ShipLocLeft;
   m_ShipLocRight        = rOther.m_ShipLocRight;
   m_strHaulTruck        = rOther.m_strHaulTruck;

   for ( int i = 0; i < 2; i++ )
   {
      m_HarpStrandOffsetEnd[i] = rOther.m_HarpStrandOffsetEnd[i];
      m_HarpStrandOffsetHp[i]  = rOther.m_HarpStrandOffsetHp[i];
   }

   m_Fci                 = rOther.m_Fci;
   m_SsDebondInfo        = rOther.m_SsDebondInfo;
   m_SlabOffset[pgsTypes::metStart] = rOther.m_SlabOffset[pgsTypes::metStart];
   m_SlabOffset[pgsTypes::metEnd]   = rOther.m_SlabOffset[pgsTypes::metEnd];
   m_AssumedExcessCamber                = rOther.m_AssumedExcessCamber;

   m_NumShearZones       = rOther.m_NumShearZones;
   m_ShearData           = rOther.m_ShearData;

   m_bWasLongitudinalRebarForShearDesigned = rOther.m_bWasLongitudinalRebarForShearDesigned;
   m_LongitudinalRebarData = rOther.m_LongitudinalRebarData;

   m_IsUserEc            = rOther.m_IsUserEc;
   m_UserEc              = rOther.m_UserEc;
   m_IsUserEci           = rOther.m_IsUserEci;
   m_UserEci             = rOther.m_UserEci;

   m_ConcreteReleaseDesignState = rOther.m_ConcreteReleaseDesignState;
   m_ConcreteFinalDesignState = rOther.m_ConcreteFinalDesignState;

   m_PreviouslyFailedDesigns = rOther.m_PreviouslyFailedDesigns;
}

void pgsSegmentDesignArtifact::MakeAssignment(const pgsSegmentDesignArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsSegmentDesignArtifact::Init()
{
   m_Outcome = NoDesignRequested;

   m_DesignNotes.clear();

   m_DesignOptions = arDesignOptions();

   m_TTSUsage = pgsTypes::ttsPretensioned;

   m_Ns                  = 0;
   m_Nh                  = 0;
   m_RaisedAdjustableStrandFill.clear();
   m_Nt                  = 0;
   m_PjS                 = 0;
   m_PjH                 = 0;
   m_PjT                 = 0;
   m_PjSUsedMax          = false;
   m_PjHUsedMax          = false;
   m_PjTUsedMax          = false;

   for ( int i = 0; i < 2; i++ )
   {
      m_HarpStrandOffsetEnd[i] = 0;
      m_HarpStrandOffsetHp[i]  = 0;
   }

   m_SsDebondInfo.clear();

   m_Fci                 = 0;
   m_SlabOffset[pgsTypes::metStart] = 0;
   m_SlabOffset[pgsTypes::metEnd] = 0;
   m_AssumedExcessCamber = 0;
   m_NumShearZones       = 0;
   m_bWasLongitudinalRebarForShearDesigned = false;
   m_LiftLocLeft         = 0.0;
   m_LiftLocRight        = 0.0;
   m_ShipLocLeft         = 0.0;
   m_ShipLocRight        = 0.0;

   m_IsUserEc            = false;
   m_UserEc              = 0.0;
   m_IsUserEci           = false;
   m_UserEci             = 0.0;

   m_ConcreteReleaseDesignState.Init();
   m_ConcreteFinalDesignState.Init();

   m_PreviouslyFailedDesigns.clear();
}
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================


void pgsSegmentDesignArtifact::ModSegmentDataForFlexureDesign(IBroker* pBroker, CPrecastSegmentData* pSegmentData) const
{
   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_SegmentKey.girderIndex);
   std::_tstring gdrName = pGirder->GetGirderName();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 HgStart, HgHp1, HgHp2, HgEnd;
   pStrandGeom->GetHarpedStrandControlHeights(m_SegmentKey,&HgStart,&HgHp1,&HgHp2,&HgEnd);

   arDesignOptions design_options = GetDesignOptions();

   if (dtDesignFullyBondedRaised  != design_options.doDesignForFlexure &&
       dtDesignForDebondingRaised != design_options.doDesignForFlexure)
   {
      // Strand Designs using continuous fill for all stands
      pgsTypes::AdjustableStrandType adjType;
      if(dtDesignForHarping == design_options.doDesignForFlexure)
      {
         adjType = pgsTypes::asHarped;
      }
      else
      {
         adjType = pgsTypes::asStraight;
      }

      pSegmentData->Strands.SetAdjustableStrandType(adjType);

      ConfigStrandFillVector harpfillvec = pStrandGeometry->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, GetNumHarpedStrands());

      // Convert Adjustable strand offset data
      // offsets are absolute measure in the design artifact
      // Convert them to the measurement basis that the CGirderData object is using, unless it's the default,
      // then let's use a favorite
      if (hsoLEGACY == pSegmentData->Strands.GetHarpStrandOffsetMeasurementAtEnd())
      {
         pSegmentData->Strands.SetHarpStrandOffsetMeasurementAtEnd(hsoBOTTOM2BOTTOM);
      }

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         pSegmentData->Strands.SetHarpStrandOffsetAtEnd(endType,pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(gdrName.c_str(), endType, adjType,
                                                                                             HgStart, HgHp1, HgHp2, HgEnd,
                                                                                             harpfillvec, 
                                                                                             pSegmentData->Strands.GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                             GetHarpStrandOffsetEnd(endType)));
      }

      if (hsoLEGACY == pSegmentData->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint())
      {
         pSegmentData->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint(hsoBOTTOM2BOTTOM);
      }

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         pSegmentData->Strands.SetHarpStrandOffsetAtHarpPoint(endType,pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), endType, adjType,
                                                                                           HgStart, HgHp1, HgHp2, HgEnd,
                                                                                           harpfillvec, 
                                                                                           pSegmentData->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                                           GetHarpStrandOffsetHp(endType)));
      }

      // See if strand design data fits in grid
      bool fills_grid=false;
      StrandIndexType num_permanent = GetNumHarpedStrands() + GetNumStraightStrands();
      StrandIndexType ns(0), nh(0);
      if (design_options.doStrandFillType == ftGridOrder)
      {
         // we asked design to fill using grid, but this may be a non-standard design - let's check
         if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, m_SegmentKey, &ns, &nh))
         {
            if (ns == GetNumStraightStrands() && nh == GetNumHarpedStrands() )
            {
               fills_grid = true;
            }
         }
      }

      if ( fills_grid )
      {
         ATLASSERT(num_permanent==ns+nh);
         pSegmentData->Strands.SetTotalPermanentNstrands(num_permanent, ns, nh);
         pSegmentData->Strands.SetPjack(pgsTypes::Permanent,GetPjackStraightStrands() + GetPjackHarpedStrands());
         pSegmentData->Strands.IsPjackCalculated(pgsTypes::Permanent, GetUsedMaxPjackStraightStrands());
      }
      else
      {
         pSegmentData->Strands.SetHarpedStraightNstrands(GetNumStraightStrands(), GetNumHarpedStrands());
      }

      pSegmentData->Strands.SetTemporaryNstrands(GetNumTempStrands());
   }
   else
   {
      // Raised straight design
      pSegmentData->Strands.SetAdjustableStrandType(pgsTypes::asStraight);

      // Raised straight adjustable strands are filled directly, but others use fill order.
      // must convert all to DirectStrandFillCollection
      ConfigStrandFillVector strvec = pStrandGeometry->ComputeStrandFill(m_SegmentKey, pgsTypes::Straight, GetNumStraightStrands());
      CDirectStrandFillCollection strfill = ConvertConfigToDirectStrandFill(strvec);
      pSegmentData->Strands.SetDirectStrandFillStraight(strfill);

      CDirectStrandFillCollection harpfill =  ConvertConfigToDirectStrandFill(GetRaisedAdjustableStrands());
      pSegmentData->Strands.SetDirectStrandFillHarped(harpfill);

      ConfigStrandFillVector tempvec = pStrandGeometry->ComputeStrandFill(m_SegmentKey, pgsTypes::Temporary, GetNumTempStrands());
      CDirectStrandFillCollection tempfill =  ConvertConfigToDirectStrandFill(tempvec);
      pSegmentData->Strands.SetDirectStrandFillTemporary(tempfill);

      // Convert Adjustable strand offset data. This is typically zero from library, but must be converted to input datum
      // offsets are absolute measure in the design artifact
      // convert them to the measurement basis that the CGirderData object is using
      ConfigStrandFillVector harpfillvec = GetRaisedAdjustableStrands();
         
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         pSegmentData->Strands.SetHarpStrandOffsetAtEnd(endType,pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(gdrName.c_str(), endType, pgsTypes::asStraight,
                                                                                             HgStart, HgHp1, HgHp2, HgEnd,
                                                                                             harpfillvec, 
                                                                                             pSegmentData->Strands.GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                             GetHarpStrandOffsetEnd(endType)));

         pSegmentData->Strands.SetHarpStrandOffsetAtHarpPoint(endType,pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), endType, pgsTypes::asStraight,
                                                                                           HgStart, HgHp1, HgHp2, HgEnd,
                                                                                           harpfillvec, 
                                                                                           pSegmentData->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                                           GetHarpStrandOffsetHp(endType)));
      }
   }

   pSegmentData->Strands.SetPjack(pgsTypes::Harped,             GetPjackHarpedStrands());
   pSegmentData->Strands.SetPjack(pgsTypes::Straight,           GetPjackStraightStrands());
   pSegmentData->Strands.SetPjack(pgsTypes::Temporary,          GetPjackTempStrands());
   pSegmentData->Strands.IsPjackCalculated(pgsTypes::Harped,    GetUsedMaxPjackHarpedStrands());
   pSegmentData->Strands.IsPjackCalculated(pgsTypes::Straight,  GetUsedMaxPjackStraightStrands());
   pSegmentData->Strands.IsPjackCalculated(pgsTypes::Temporary, GetUsedMaxPjackTempStrands());
   pSegmentData->Strands.SetLastUserPjack(pgsTypes::Harped,     GetPjackHarpedStrands());
   pSegmentData->Strands.SetLastUserPjack(pgsTypes::Straight,   GetPjackStraightStrands());
   pSegmentData->Strands.SetLastUserPjack(pgsTypes::Temporary,  GetPjackTempStrands());

   pSegmentData->Strands.SetTemporaryStrandUsage( GetTemporaryStrandUsage() );

   // Designer doesn't do extended strands
   pSegmentData->Strands.ClearExtendedStrands(pgsTypes::Straight,pgsTypes::metStart);
   pSegmentData->Strands.ClearExtendedStrands(pgsTypes::Straight,pgsTypes::metEnd);

   // Get debond information from design artifact
   pSegmentData->Strands.ClearDebondData();
   pSegmentData->Strands.IsSymmetricDebond(true);  // design is always symmetric

   // TRICKY: Mapping from DEBONDCONFIG to CDebondInfo is tricky because
   //         former designates individual strands and latter stores strands
   //         in grid order.
   // Use utility tool to make the strand indexing conversion
   ConfigStrandFillVector strtfillvec = pStrandGeometry->ComputeStrandFill(m_SegmentKey, pgsTypes::Straight, GetNumStraightStrands());
   ConfigStrandFillTool fillTool( strtfillvec );

   DebondConfigCollection dbcoll = GetStraightStrandDebondInfo();
   // sort this collection by strand idices to ensure we get it right
   std::sort( dbcoll.begin(), dbcoll.end() ); // default < operator is by index

   for (DebondConfigConstIterator dbit = dbcoll.begin(); dbit!=dbcoll.end(); dbit++)
   {
      const DEBONDCONFIG& rdbrinfo = *dbit;

      CDebondData cdbi;

      StrandIndexType gridIndex, otherPos;
      fillTool.StrandPositionIndexToGridIndex(rdbrinfo.strandIdx, &gridIndex, &otherPos);

      cdbi.strandTypeGridIdx = gridIndex;

      // If there is another position, this is a pair. Increment to next position
      if (otherPos != INVALID_INDEX)
      {
         dbit++;

#ifdef _DEBUG
         const DEBONDCONFIG& ainfo = *dbit;
         StrandIndexType agrid;
         fillTool.StrandPositionIndexToGridIndex(ainfo.strandIdx, &agrid, &otherPos);
         ATLASSERT(agrid==gridIndex); // must have the same grid index
#endif
      }

      cdbi.Length[pgsTypes::metStart] = rdbrinfo.DebondLength[pgsTypes::metStart];
      cdbi.Length[pgsTypes::metEnd]   = rdbrinfo.DebondLength[pgsTypes::metEnd];

      pSegmentData->Strands.GetDebonding(pgsTypes::Straight).push_back(cdbi);
   }
   
   // concrete
   pSegmentData->Material.Concrete.Fci = GetReleaseStrength();
   if (!pSegmentData->Material.Concrete.bUserEci)
   {
      pSegmentData->Material.Concrete.Eci = lrfdConcreteUtil::ModE( (matConcrete::Type)pSegmentData->Material.Concrete.Type,pSegmentData->Material.Concrete.Fci, 
                                                             pSegmentData->Material.Concrete.StrengthDensity, 
                                                             false  ); // ignore LRFD range checks 
      pSegmentData->Material.Concrete.Eci *= (pSegmentData->Material.Concrete.EcK1*pSegmentData->Material.Concrete.EcK2);
   }

   pSegmentData->Material.Concrete.Fc  = GetConcreteStrength();
   if (!pSegmentData->Material.Concrete.bUserEc)
   {
      pSegmentData->Material.Concrete.Ec = lrfdConcreteUtil::ModE((matConcrete::Type)pSegmentData->Material.Concrete.Type, pSegmentData->Material.Concrete.Fc,
                                                            pSegmentData->Material.Concrete.StrengthDensity, 
                                                            false );// ignore LRFD range checks 
      pSegmentData->Material.Concrete.Ec *= (pSegmentData->Material.Concrete.EcK1*pSegmentData->Material.Concrete.EcK2);
   }

   // lifting
   if ( design_options.doDesignLifting )
   {
      pSegmentData->HandlingData.LeftLiftPoint  = GetLeftLiftingLocation();
      pSegmentData->HandlingData.RightLiftPoint = GetRightLiftingLocation();
   }

   // shipping
   if ( design_options.doDesignHauling )
   {
      pSegmentData->HandlingData.LeadingSupportPoint  = GetLeadingOverhang();
      pSegmentData->HandlingData.TrailingSupportPoint = GetTrailingOverhang();
      pSegmentData->HandlingData.HaulTruckName        = GetHaulTruck();
   }
}

void pgsSegmentDesignArtifact::ModSegmentDataForShearDesign(IBroker* pBroker, CPrecastSegmentData* pSegmentData) const
{
   // get the design data
   pSegmentData->ShearData.ShearZones.clear();

   ZoneIndexType nShearZones = GetNumberOfStirrupZonesDesigned();
   if (0 < nShearZones)
   {
      pSegmentData->ShearData = *GetShearData();
   }
   else
   {
      // if no shear zones were designed, we had a design failure.
      // create a single zone with no stirrups in it.
      CShearZoneData2 dat;
      pSegmentData->ShearData.ShearZones.push_back(dat);
   }

   if(GetWasLongitudinalRebarForShearDesigned())
   {
      // Rebar data was changed during shear design
      pSegmentData->LongitudinalRebarData = GetLongitudinalRebarData();
   }

   // It is possible for shear stress to control final concrete strength
   // Make sure it is updated if no flexural design was requested
   if (GetDesignOptions().doDesignForFlexure == dtNoDesign)
   {
      pSegmentData->Material.Concrete.Fc = GetConcreteStrength();
      if ( !pSegmentData->Material.Concrete.bUserEc )
      {
         pSegmentData->Material.Concrete.Ec = lrfdConcreteUtil::ModE((matConcrete::Type)pSegmentData->Material.Concrete.Type, pSegmentData->Material.Concrete.Fc,
                                                               pSegmentData->Material.Concrete.StrengthDensity, 
                                                               false );// ignore LRFD range checks 
         pSegmentData->Material.Concrete.Ec *= (pSegmentData->Material.Concrete.EcK1*pSegmentData->Material.Concrete.EcK2);
      }
   }
}
