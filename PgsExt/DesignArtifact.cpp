///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <WBFLCore.h>
#include <IFace\Tools.h>
#include <IFace\Project.h>
#include <PgsExt\DesignArtifact.h>
#include <DesignConfigUtil.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsDesignArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsDesignArtifact::pgsDesignArtifact()
{
   Init();
}

pgsDesignArtifact::pgsDesignArtifact(SpanIndexType span,GirderIndexType gdr)
{
   Init();

   m_Span = span;
   m_Gdr  = gdr;
}

pgsDesignArtifact::pgsDesignArtifact(const pgsDesignArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsDesignArtifact::~pgsDesignArtifact()
{
}

//======================== OPERATORS  =======================================
pgsDesignArtifact& pgsDesignArtifact::operator= (const pgsDesignArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsDesignArtifact::SetOutcome(pgsDesignArtifact::Outcome outcome)
{
   m_Outcome = outcome;
}

pgsDesignArtifact::Outcome pgsDesignArtifact::GetOutcome() const
{
   return m_Outcome;
}

void pgsDesignArtifact::AddDesignNote(pgsDesignArtifact::DesignNote note)
{
   m_DesignNotes.push_back(note);
}

bool pgsDesignArtifact::DoDesignNotesExist() const
{
   return !m_DesignNotes.empty();
}

std::vector<pgsDesignArtifact::DesignNote> pgsDesignArtifact::GetDesignNotes() const
{
   return m_DesignNotes;
}

SpanIndexType pgsDesignArtifact::GetSpan() const
{
   return m_Span;
}

GirderIndexType pgsDesignArtifact::GetGirder() const
{
   return m_Gdr;
}

void pgsDesignArtifact::SetDesignOptions(arDesignOptions options)
{
   m_DesignOptions = options;
}

arDesignOptions pgsDesignArtifact::GetDesignOptions() const
{
   return m_DesignOptions;
}

arFlexuralDesignType pgsDesignArtifact::GetDoDesignFlexure() const
{
   return m_DesignOptions.doDesignForFlexure;
}

bool pgsDesignArtifact::GetDoDesignShear() const
{
   return m_DesignOptions.doDesignForShear;
}

void pgsDesignArtifact::SetNumStraightStrands(StrandIndexType Ns)
{
   m_Ns = Ns;
}

StrandIndexType pgsDesignArtifact::GetNumStraightStrands() const
{
   return m_Ns;
}

void pgsDesignArtifact::SetNumTempStrands(StrandIndexType Nt)
{
   m_Nt = Nt;
}

StrandIndexType pgsDesignArtifact::GetNumTempStrands() const
{
   return m_Nt;
}

void pgsDesignArtifact::SetNumHarpedStrands(StrandIndexType Nh)
{
   m_Nh = Nh;
}

StrandIndexType pgsDesignArtifact::GetNumHarpedStrands() const
{
   return m_Nh;
}

void pgsDesignArtifact::SetPjackStraightStrands(Float64 Pj)
{
   m_PjS = Pj;
}

Float64 pgsDesignArtifact::GetPjackStraightStrands() const
{
   return m_PjS;
}

void pgsDesignArtifact::SetUsedMaxPjackStraightStrands(bool usedMax)
{
   m_PjSUsedMax = usedMax;
}

bool pgsDesignArtifact::GetUsedMaxPjackStraightStrands() const
{
   return m_PjSUsedMax;
}

void pgsDesignArtifact::SetPjackTempStrands(Float64 Pj)
{
   m_PjT = Pj;
}

Float64 pgsDesignArtifact::GetPjackTempStrands() const
{
   return m_PjT;
}

void pgsDesignArtifact::SetUsedMaxPjackTempStrands(bool usedMax)
{
   m_PjTUsedMax = usedMax;
}

bool pgsDesignArtifact::GetUsedMaxPjackTempStrands() const
{
   return m_PjTUsedMax;
}

void pgsDesignArtifact::SetPjackHarpedStrands(Float64 Pj)
{
   m_PjH = Pj;
}

Float64 pgsDesignArtifact::GetPjackHarpedStrands() const
{
   return m_PjH;
}

void pgsDesignArtifact::SetUsedMaxPjackHarpedStrands(bool usedMax)
{
   m_PjHUsedMax = usedMax;
}

bool pgsDesignArtifact::GetUsedMaxPjackHarpedStrands() const
{
   return m_PjHUsedMax;
}

void pgsDesignArtifact::SetHarpStrandOffsetEnd(Float64 oe)
{
   m_HarpStrandOffsetEnd = oe;
}

Float64 pgsDesignArtifact::GetHarpStrandOffsetEnd() const
{
   return m_HarpStrandOffsetEnd;
}

void pgsDesignArtifact::SetHarpStrandOffsetHp(Float64 ohp)
{
   m_HarpStrandOffsetHp = ohp;
}

Float64 pgsDesignArtifact::GetHarpStrandOffsetHp() const
{
   return m_HarpStrandOffsetHp;
}

DebondInfoCollection pgsDesignArtifact::GetStraightStrandDebondInfo() const
{
   return m_SsDebondInfo;
}

void pgsDesignArtifact::SetStraightStrandDebondInfo(const DebondInfoCollection& dbinfo)
{
   m_SsDebondInfo = dbinfo;
}

void pgsDesignArtifact::ClearDebondInfo()
{
   m_SsDebondInfo.clear();
}

void pgsDesignArtifact::SetReleaseStrength(Float64 fci)
{
   m_Fci = fci;
}

Float64 pgsDesignArtifact::GetReleaseStrength() const
{
   return m_Fci;
}

void pgsDesignArtifact::SetConcrete(matConcreteEx concrete)
{
   m_Concrete = concrete;
}

const matConcreteEx& pgsDesignArtifact::GetConcrete() const
{
   return m_Concrete;
}

void pgsDesignArtifact::SetConcreteStrength(Float64 fc)
{
   double density = m_Concrete.GetDensity();
   double Ec = lrfdConcreteUtil::ModE(fc,density,false);
   m_Concrete.SetFc(fc);
   m_Concrete.SetE(Ec);
}

Float64 pgsDesignArtifact::GetConcreteStrength() const
{
   return m_Concrete.GetFc();
}

void pgsDesignArtifact::SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset)
{
   m_SlabOffset[end] = offset;
}

Float64 pgsDesignArtifact::GetSlabOffset(pgsTypes::MemberEndType end) const
{
   return m_SlabOffset[end];
}

void pgsDesignArtifact::SetLiftingLocations(Float64 left,Float64 right)
{
   m_LiftLocLeft  = left;
   m_LiftLocRight = right;
}

Float64 pgsDesignArtifact::GetLeftLiftingLocation() const
{
   return m_LiftLocLeft;
}

Float64 pgsDesignArtifact::GetRightLiftingLocation() const
{
   return m_LiftLocRight;
}

void pgsDesignArtifact::SetTruckSupportLocations(Float64 left,Float64 right)
{
   m_ShipLocLeft  = left;
   m_ShipLocRight = right;
}

Float64 pgsDesignArtifact::GetLeadingOverhang() const
{
   return m_ShipLocRight;
}

Float64 pgsDesignArtifact::GetTrailingOverhang() const
{
   return m_ShipLocLeft;
}

pgsTypes::TTSUsage pgsDesignArtifact::GetTemporaryStrandUsage() const
{
   return pgsTypes::ttsPretensioned;
}

GDRCONFIG pgsDesignArtifact::GetGirderConfiguration() const
{
   GDRCONFIG config;
   config.Nstrands[pgsTypes::Straight]  = GetNumStraightStrands();
   config.Nstrands[pgsTypes::Harped]    = GetNumHarpedStrands();
   config.Nstrands[pgsTypes::Temporary] = GetNumTempStrands();

   config.Pjack[pgsTypes::Straight]  = GetPjackStraightStrands();
   config.Pjack[pgsTypes::Harped]    = GetPjackHarpedStrands();
   config.Pjack[pgsTypes::Temporary] = GetPjackTempStrands();

   config.EndOffset = GetHarpStrandOffsetEnd();
   config.HpOffset  = GetHarpStrandOffsetHp();
   
   config.Debond[pgsTypes::Straight] = m_SsDebondInfo; // we only design debond for straight strands

   config.Fci       = GetReleaseStrength();
   config.Fc        = GetConcreteStrength();
   config.ConcType  = (pgsTypes::ConcreteType)m_Concrete.GetType();
   config.bHasFct   = m_Concrete.HasAggSplittingStrength();
   config.Fct       = m_Concrete.GetAggSplittingStrength();

   config.TempStrandUsage = GetTemporaryStrandUsage();

   // allow moduli to be computed
   config.bUserEci = m_IsUserEci;
   config.Eci      = m_UserEci;
   config.bUserEc  = m_IsUserEc;
   config.Ec       = m_UserEc;

   config.SlabOffset[pgsTypes::metStart] = GetSlabOffset(pgsTypes::metStart);
   config.SlabOffset[pgsTypes::metEnd]   = GetSlabOffset(pgsTypes::metEnd);

   WriteShearDataToStirrupConfig(m_ShearData, config.StirrupConfig);

//   WriteLongitudinalRebarDataToConfig(m_LongitudinalRebarData, config.LongitudinalRebarConfig);

   return config;
}

ZoneIndexType pgsDesignArtifact::GetNumberOfStirrupZonesDesigned() const
{
   return m_NumShearZones;
}

const CShearData& pgsDesignArtifact::GetShearData() const
{
   return m_ShearData;
}

void pgsDesignArtifact::SetNumberOfStirrupZonesDesigned(ZoneIndexType num)
{
   m_NumShearZones = num;
}

void pgsDesignArtifact::SetShearData(const CShearData& rdata)
{
   m_ShearData = rdata;
}

void pgsDesignArtifact::SetWasLongitudinalRebarForShearDesigned(bool isTrue)
{
   m_bWasLongitudinalRebarForShearDesigned = isTrue;
}

bool pgsDesignArtifact::GetWasLongitudinalRebarForShearDesigned() const
{
   return m_bWasLongitudinalRebarForShearDesigned;
}


CLongitudinalRebarData& pgsDesignArtifact::GetLongitudinalRebarData() 
{
   return m_LongitudinalRebarData;
}

const CLongitudinalRebarData& pgsDesignArtifact::GetLongitudinalRebarData() const
{
   return m_LongitudinalRebarData;
}

void pgsDesignArtifact::SetLongitudinalRebarData(const CLongitudinalRebarData& rdata)
{
   m_LongitudinalRebarData = rdata;
}

void pgsDesignArtifact::SetUserEc(Float64 Ec)
{
   PRECONDITION(Ec>0.0);
   m_IsUserEc = true;
   m_UserEc   = Ec;
}

void pgsDesignArtifact::SetUserEci(Float64 Eci)
{
   PRECONDITION(Eci>0.0);
   m_IsUserEci = true;
   m_UserEci   = Eci;
}


const pgsDesignArtifact::ConcreteStrengthDesignState& pgsDesignArtifact::GetReleaseDesignState() const
{
   return m_ConcreteReleaseDesignState;
}

const pgsDesignArtifact::ConcreteStrengthDesignState& pgsDesignArtifact::GetFinalDesignState() const
{
   return m_ConcreteFinalDesignState;
}

void pgsDesignArtifact::SetReleaseDesignState(const ConcreteStrengthDesignState& state)
{
   m_ConcreteReleaseDesignState = state;
}

void pgsDesignArtifact::SetFinalDesignState(const ConcreteStrengthDesignState& state)
{
   m_ConcreteFinalDesignState = state;
}


void pgsDesignArtifact::ConcreteStrengthDesignState::SetState(bool controlledByMin, pgsTypes::Stage stage, pgsTypes::StressType stressType, 
              pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
{
   m_MinimumControls = controlledByMin;
   m_Stage = stage;
   m_StressType = stressType;
   m_LimitState = limitState;
   m_StressLocation = stressLocation;
}

bool pgsDesignArtifact::ConcreteStrengthDesignState::WasControlledByMinimum() const
{
   return m_MinimumControls;
}

pgsTypes::Stage pgsDesignArtifact::ConcreteStrengthDesignState::Stage() const
{
   PRECONDITION(m_MinimumControls==false);
   return m_Stage;
}

pgsTypes::StressType pgsDesignArtifact::ConcreteStrengthDesignState::StressType() const
{
   PRECONDITION(m_MinimumControls==false);
   return m_StressType;
}

pgsTypes::LimitState pgsDesignArtifact::ConcreteStrengthDesignState::LimitState() const
{
   PRECONDITION(m_MinimumControls==false);
   return m_LimitState;
}

pgsTypes::StressLocation pgsDesignArtifact::ConcreteStrengthDesignState::StressLocation() const
{
   PRECONDITION(m_MinimumControls==false);
   return m_StressLocation;
}

inline LPCTSTR StageString(pgsTypes::Stage stage)
{
   switch(stage)
   {
   case pgsTypes::CastingYard:
      return _T("Casting Yard");
      break;
   case pgsTypes::Lifting:
      return _T("Lifting");
      break;
   case pgsTypes::Hauling:
      return _T("Hauling");
      break;
   case pgsTypes::GirderPlacement:
      return _T("Girder Placement");
      break;
   case pgsTypes::TemporaryStrandRemoval:
      return _T("Temporary Strand Removal");
      break;
   case pgsTypes::BridgeSite1:
      return _T("Bridge Site 1");
      break;
   case pgsTypes::BridgeSite2:
      return _T("Bridge Site 2");
      break;
   case pgsTypes::BridgeSite3:
      return _T("Bridge Site 3");
      break;
   default:
      ATLASSERT(0);
      return _T("Error in stage name");
   }
}

inline LPCTSTR LimitStateString(pgsTypes::LimitState limitState)
{
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
      ATLASSERT(0);
      return _T("Error in limit state name");
   }
}

inline LPCTSTR StressLocationString(pgsTypes::StressLocation loc)
{
   switch(loc)
   {
   case pgsTypes::BottomGirder:
      return _T("Bottom of Girder");
      break;
   case pgsTypes::TopGirder:
      return _T("Top of Girder");
      break;
   case pgsTypes::TopSlab:
      return _T("Top of Slab");
      break;
   default:
      ATLASSERT(0);
      return _T("Error in StressLocation");
   }
}

inline LPCTSTR StressTypeString(pgsTypes::StressType type)
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
      ATLASSERT(0);
      return _T("Error in StressType");
   }
}


std::_tstring pgsDesignArtifact::ConcreteStrengthDesignState::AsString() const
{
   if (m_MinimumControls)
   {
      return std::_tstring(_T("Minimum"));
   }
   else
   {
      std::_tostringstream sstr;
      sstr<< StageString(m_Stage)<<_T(", ")<<LimitStateString(m_LimitState)<<_T(", ")<<StressTypeString(m_StressType)<<_T(", at ")<<StressLocationString(m_StressLocation);
      return sstr.str();
   }
}

bool pgsDesignArtifact::ConcreteStrengthDesignState::operator==(const ConcreteStrengthDesignState& rOther) const
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
   else
   {
      return m_Stage==rOther.m_Stage && m_StressType==rOther.m_StressType &&
             m_LimitState==rOther.m_LimitState && m_StressLocation==rOther.m_StressLocation;
   }
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsDesignArtifact::MakeCopy(const pgsDesignArtifact& rOther)
{
   m_Outcome = rOther.m_Outcome;

   m_DesignNotes = rOther.m_DesignNotes;

   m_Span = rOther.m_Span;
   m_Gdr = rOther.m_Gdr;

   m_DesignOptions  = rOther.m_DesignOptions;

   m_Ns                  = rOther.m_Ns;
   m_Nh                  = rOther.m_Nh;
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
}

void pgsDesignArtifact::MakeAssignment(const pgsDesignArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsDesignArtifact::Init()
{
   m_Outcome = Success;

   m_DesignNotes.clear();

   m_Span = 0;
   m_Gdr  = 0;

   m_DesignOptions = arDesignOptions();

   m_Ns                  = 0;
   m_Nh                  = 0;
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
}
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
