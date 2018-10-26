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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SegmentDesignArtifact.h>
#include <WBFLCore.h>
#include <IFace\Tools.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\DesignConfigUtil.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Intervals.h>

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
   return m_DesignOptions.doDesignForShear;
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
      ATLASSERT(false);
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

void pgsSegmentDesignArtifact::SetHarpStrandOffsetEnd(Float64 oe)
{
   m_HarpStrandOffsetEnd = oe;
}

Float64 pgsSegmentDesignArtifact::GetHarpStrandOffsetEnd() const
{
   return m_HarpStrandOffsetEnd;
}

void pgsSegmentDesignArtifact::SetHarpStrandOffsetHp(Float64 ohp)
{
   m_HarpStrandOffsetHp = ohp;
}

Float64 pgsSegmentDesignArtifact::GetHarpStrandOffsetHp() const
{
   return m_HarpStrandOffsetHp;
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
      Float64 Ec = lrfdConcreteUtil::ModE(fc,density,false);
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

pgsTypes::TTSUsage pgsSegmentDesignArtifact::GetTemporaryStrandUsage() const
{
   return pgsTypes::ttsPretensioned;
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

   config.PrestressConfig.EndOffset = GetHarpStrandOffsetEnd();
   config.PrestressConfig.HpOffset  = GetHarpStrandOffsetHp();

   config.PrestressConfig.AdjustableStrandType = GetAdjustableStrandType();
   
   config.PrestressConfig.Debond[pgsTypes::Straight] = m_SsDebondInfo; // we only design debond for straight strands

   config.Fci       = GetReleaseStrength();
   config.Fc        = GetConcreteStrength();
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
      config.Eci = lrfdConcreteUtil::ModE( config.Fci, m_Concrete.GetDensity(), false);
   }

   config.bUserEc  = m_IsUserEc;
   if(m_IsUserEc)
   {
      config.Ec       = m_UserEc;
   }
   else
   {
      config.Ec = lrfdConcreteUtil::ModE( config.Fc, m_Concrete.GetDensity(), false);
   }

   config.SlabOffset[pgsTypes::metStart] = GetSlabOffset(pgsTypes::metStart);
   config.SlabOffset[pgsTypes::metEnd]   = GetSlabOffset(pgsTypes::metEnd);

   WriteShearDataToStirrupConfig(&m_ShearData, config.StirrupConfig);

   return config;
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
   m_ConcreteReleaseDesignState = state;
}

void pgsSegmentDesignArtifact::SetFinalDesignState(const ConcreteStrengthDesignState& state)
{
   m_ConcreteFinalDesignState = state;
}

void pgsSegmentDesignArtifact::ConcreteStrengthDesignState::SetStressState(bool controlledByMin, const CSegmentKey& segmentKey,IntervalIndexType intervalIdx, pgsTypes::StressType stressType, 
                    pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
{
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
   PRECONDITION(m_MinimumControls==false);
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

LPCTSTR LimitStateString(pgsTypes::LimitState limitState)
{
#pragma Reminder("REVIEW: I think this is a duplicate of what's on the IStageMap interface")
   // review the need for this method and eliminate if it is unnecessary
   switch(limitState)
   {
   case pgsTypes::ServiceI:
      return _T("Service I");
      break;
   case pgsTypes::ServiceIA:
      return _T("Service IA");
      break;
   case pgsTypes::ServiceIII:
      return _T("Service III");
      break;
   case pgsTypes::StrengthI:
      return _T("Strength I");
      break;
   case pgsTypes::FatigueI:
      return _T("Fatigue I");
      break;
   default:
      ATLASSERT(false);
      return _T("Error in limit state name");
   }
}

LPCTSTR StressLocationString(pgsTypes::StressLocation loc)
{
   switch(loc)
   {
   case pgsTypes::BottomGirder:
      return _T("Bottom of Girder");
      break;
   case pgsTypes::TopGirder:
      return _T("Top of Girder");
      break;
   case pgsTypes::TopDeck:
      return _T("Top of Slab");
      break;
   default:
      ATLASSERT(false);
      return _T("Error in StressLocation");
   }
}

LPCTSTR StressTypeString(pgsTypes::StressType type)
{
   switch(type)
   {
   case pgsTypes::Tension:
      return _T("Tension");
      break;
   case pgsTypes::Compression:
      return _T("Compression");
      break;
   default:
      ATLASSERT(false);
      return _T("Error in StressType");
   }
}


std::_tstring pgsSegmentDesignArtifact::ConcreteStrengthDesignState::AsString() const
{
   if (m_MinimumControls)
   {
      return std::_tstring(_T("Minimum"));
   }
   else if (m_Action==actStress)
   {
      std::_tostringstream sstr;
      sstr<< _T("flexural stress in Interval ") << LABEL_INTERVAL(m_IntervalIdx) << _T(", ") << LimitStateString(m_LimitState)<<_T(", ") << StressTypeString(m_StressType)<<_T(", at ") << StressLocationString(m_StressLocation);
      return sstr.str();
   }
   else if (m_Action==actShear)
   {
      std::_tostringstream sstr;
      sstr<< _T("ultimate shear stress in Interval ") << LABEL_INTERVAL(m_IntervalIdx) << _T(", ") << LimitStateString(m_LimitState);
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

   m_HarpStrandOffsetEnd = rOther.m_HarpStrandOffsetEnd;
   m_HarpStrandOffsetHp  = rOther.m_HarpStrandOffsetHp;
   m_Fci                 = rOther.m_Fci;
   m_SsDebondInfo        = rOther.m_SsDebondInfo;
   m_SlabOffset[pgsTypes::metStart] = rOther.m_SlabOffset[pgsTypes::metStart];
   m_SlabOffset[pgsTypes::metEnd]   = rOther.m_SlabOffset[pgsTypes::metEnd];

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
   m_HarpStrandOffsetEnd = 0;
   m_HarpStrandOffsetHp  = 0;
   m_SsDebondInfo.clear();

   m_Fci                 = 0;
   m_SlabOffset[pgsTypes::metStart] = 0;
   m_SlabOffset[pgsTypes::metEnd] = 0;
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
