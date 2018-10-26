///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\KdotHaulingAnalysisArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include "PGSuperUnits.h"

#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\Project.h>

#include <Reporting\ReportNotes.h>
#include <PgsExt\ReportStyleHolder.h>
#include <EAF\EAFDisplayUnits.h>



/****************************************************************************
CLASS
   pgsKdotHaulingStressAnalysisArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsKdotHaulingStressAnalysisArtifact::pgsKdotHaulingStressAnalysisArtifact():
m_EffectiveHorizPsForce(0.0),
m_EccentricityPsForce(0.0),
m_Moment(0.0),
m_TopFiberStressPrestress(0.0),
m_TopFiberStress(0.0),
m_BottomFiberStressPrestress(0.0),
m_BottomFiberStress(0.0),
m_AllowableCompression(0.0),
m_ReqdCompConcreteStrength(0.0),
m_ReqdTensConcreteStrengthNoRebar(0.0),
m_ReqdTensConcreteStrengthWithRebar(0.0),
m_WasRebarReqd(false),
m_Yna(0.0),
m_At(0.0),
m_T(0.0),
m_AsReqd(0.0),
m_AsProvd(0.0),
m_fAllow(0.0)
{
}

pgsKdotHaulingStressAnalysisArtifact::pgsKdotHaulingStressAnalysisArtifact(const pgsKdotHaulingStressAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsKdotHaulingStressAnalysisArtifact::~pgsKdotHaulingStressAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsKdotHaulingStressAnalysisArtifact& pgsKdotHaulingStressAnalysisArtifact::operator= (const pgsKdotHaulingStressAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsKdotHaulingStressAnalysisArtifact::TensionPassed() const
{
   Float64 fTop, fBottom, Capacity;
   GetConcreteTensileStress(&fTop, &fBottom, &Capacity);
   if ( IsGE(Capacity,fTop) )
   {
      return false;
   }
   else if ( IsGE(Capacity,fBottom) )
   {
      return false;
   }

   return true;
}

bool pgsKdotHaulingStressAnalysisArtifact::CompressionPassed() const
{
   Float64 comp_stress = GetMaximumConcreteCompressiveStress();

   Float64 max_comp_stress = GetCompressiveCapacity();

   if (comp_stress < max_comp_stress)
      return false;

   return true;
}

bool pgsKdotHaulingStressAnalysisArtifact::Passed() const
{
    return TensionPassed() && CompressionPassed();
}

//======================== ACCESS     =======================================
void pgsKdotHaulingStressAnalysisArtifact::SetCompressiveCapacity(Float64 fAllowable)
{
   m_AllowableCompression = fAllowable;
}

Float64 pgsKdotHaulingStressAnalysisArtifact::GetCompressiveCapacity() const
{
   return m_AllowableCompression;
}


Float64 pgsKdotHaulingStressAnalysisArtifact::GetEffectiveHorizPsForce() const
{
   return m_EffectiveHorizPsForce;
}

void pgsKdotHaulingStressAnalysisArtifact::SetEffectiveHorizPsForce(Float64 f)
{
   m_EffectiveHorizPsForce = f;
}

Float64 pgsKdotHaulingStressAnalysisArtifact::GetEccentricityPsForce() const
{
   return m_EccentricityPsForce;
}

void pgsKdotHaulingStressAnalysisArtifact::SetEccentricityPsForce(Float64 f)
{
   m_EccentricityPsForce = f;
}

void pgsKdotHaulingStressAnalysisArtifact::SetMoment(Float64 moment)
{
   m_Moment = moment;
}

Float64 pgsKdotHaulingStressAnalysisArtifact::GetMoment() const
{
   return m_Moment;
}

void pgsKdotHaulingStressAnalysisArtifact::GetTopFiberStress(Float64* pPs, Float64* pStress) const
{
    *pPs       = m_TopFiberStressPrestress;
    *pStress   = m_TopFiberStress;
}

void pgsKdotHaulingStressAnalysisArtifact::SetTopFiberStress(Float64 Ps, Float64 stress)
{
   m_TopFiberStressPrestress= Ps;
   m_TopFiberStress = stress;
}

void pgsKdotHaulingStressAnalysisArtifact::GetBottomFiberStress(Float64* pPs, Float64* pStress) const
{
    *pPs     = m_BottomFiberStressPrestress;
    *pStress = m_BottomFiberStress;
}

void pgsKdotHaulingStressAnalysisArtifact::SetBottomFiberStress(Float64 Ps,Float64 stress)
{
   m_BottomFiberStressPrestress= Ps;
   m_BottomFiberStress         = stress;
}

Float64 pgsKdotHaulingStressAnalysisArtifact::GetMaximumConcreteCompressiveStress() const
{
   return min(m_TopFiberStress, m_BottomFiberStress);
}

Float64 pgsKdotHaulingStressAnalysisArtifact::GetMaximumConcreteTensileStress() const
{
   return max(m_TopFiberStress, m_BottomFiberStress);
}

void pgsKdotHaulingStressAnalysisArtifact::GetConcreteTensileStress(Float64* fTop, Float64* fBottom, Float64* Capacity) const
{
   *fTop = m_TopFiberStress;
   *fBottom = m_BottomFiberStress;
   *Capacity = m_fAllow;
}

void pgsKdotHaulingStressAnalysisArtifact::SetAlternativeTensileStressParameters(Float64 Yna,   Float64 At,   Float64 T,
                                                                             Float64 AsProvd,  Float64 AsReqd,  Float64 fAllow)
{
   m_Yna = Yna;
   m_At  = At;
   m_T   = T;
   m_AsReqd  = AsReqd;
   m_AsProvd = AsProvd;
   m_fAllow  = fAllow;
}

void pgsKdotHaulingStressAnalysisArtifact::GetAlternativeTensileStressParameters(Float64* Yna,   Float64* At,   Float64* T,  
                                                                             Float64* AsProvd,  Float64* AsReqd,  Float64* fAllow) const
{
   *Yna   = m_Yna;
   *At    = m_At;
   *T     = m_T;
   *AsReqd   = m_AsReqd;
   *AsProvd  = m_AsProvd;
   *fAllow   = m_fAllow;
}

void pgsKdotHaulingStressAnalysisArtifact::SetRequiredConcreteStrength(Float64 fciComp,Float64 fciTensNoRebar,Float64 fciTensWithRebar)
{
   m_ReqdCompConcreteStrength = fciComp;
   m_ReqdTensConcreteStrengthNoRebar = fciTensNoRebar;
   m_ReqdTensConcreteStrengthWithRebar = fciTensWithRebar;
}

void pgsKdotHaulingStressAnalysisArtifact::GetRequiredConcreteStrength(Float64* pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const
{
   *pfciComp = m_ReqdCompConcreteStrength;
   *pfciTensNoRebar = m_ReqdTensConcreteStrengthNoRebar;
   *pfciTensWithRebar = m_ReqdTensConcreteStrengthWithRebar;   
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsKdotHaulingStressAnalysisArtifact::MakeCopy(const pgsKdotHaulingStressAnalysisArtifact& rOther)
{
   m_EffectiveHorizPsForce     = rOther.m_EffectiveHorizPsForce;
   m_EccentricityPsForce       = rOther.m_EccentricityPsForce;

   m_Moment                    = rOther.m_Moment;
   m_TopFiberStressPrestress   = rOther.m_TopFiberStressPrestress;
   m_TopFiberStress            = rOther.m_TopFiberStress;
   m_BottomFiberStressPrestress= rOther.m_BottomFiberStressPrestress;
   m_BottomFiberStress         = rOther.m_BottomFiberStress;

   m_Yna     = rOther.m_Yna;
   m_At      = rOther.m_At;
   m_T       = rOther.m_T;
   m_AsReqd  = rOther.m_AsReqd;
   m_AsProvd = rOther.m_AsProvd;
   m_fAllow  = rOther.m_fAllow;

   m_AllowableCompression = rOther.m_AllowableCompression;
   m_ReqdCompConcreteStrength = rOther.m_ReqdCompConcreteStrength;
   m_ReqdTensConcreteStrengthNoRebar   = rOther.m_ReqdTensConcreteStrengthNoRebar;
   m_ReqdTensConcreteStrengthWithRebar = rOther.m_ReqdTensConcreteStrengthWithRebar;
   m_WasRebarReqd = rOther.m_WasRebarReqd;
}

void pgsKdotHaulingStressAnalysisArtifact::MakeAssignment(const pgsKdotHaulingStressAnalysisArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG

void pgsKdotHaulingStressAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsKdotHaulingStressAnalysisArtifact" << endl;
}
#endif // _DEBUG


/****************************************************************************
CLASS
   pgsKdotHaulingAnalysisArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsKdotHaulingAnalysisArtifact::pgsKdotHaulingAnalysisArtifact():
m_DesignOutcome(doNoDesignDone),
m_DesignOverhang(0.0),
m_GirderLength(0.0),
m_GirderWeightPerLength(0.0),
m_gOverhang(0.0),
m_gInterior(0.0),
m_ElasticModulusOfGirderConcrete(0.0),
m_LeadingOverhang(0.0),
m_TrailingOverhang(0.0),
m_HardOverhangLimit(0.0),
m_SoftOverhangLimit(0.0),
m_GirderWeight(0.0) // total girder weight
{
}

pgsKdotHaulingAnalysisArtifact::pgsKdotHaulingAnalysisArtifact(const pgsKdotHaulingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsKdotHaulingAnalysisArtifact::~pgsKdotHaulingAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsKdotHaulingAnalysisArtifact& pgsKdotHaulingAnalysisArtifact::operator= (const pgsKdotHaulingAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsKdotHaulingAnalysisArtifact::Passed() const
{
   return PassedStressCheck();
}

bool pgsKdotHaulingAnalysisArtifact::PassedStressCheck() const
{
   for (std::map<Float64,pgsKdotHaulingStressAnalysisArtifact,Float64_less>::const_iterator is = m_HaulingStressAnalysisArtifacts.begin(); 
        is!=m_HaulingStressAnalysisArtifacts.end(); is++)
   {
      Float64 distFromStart = is->first;
      const pgsKdotHaulingStressAnalysisArtifact& rart = is->second;

      if (!rart.Passed())
         return false;
   }

   return true;
}

void pgsKdotHaulingAnalysisArtifact::GetRequiredConcreteStrength(Float64* pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const
{
   Float64 maxFciComp = -Float64_Max;
   Float64 maxFciTensnobar = -Float64_Max;
   Float64 maxFciTenswithbar = -Float64_Max;

   std::map<Float64,pgsKdotHaulingStressAnalysisArtifact,Float64_less>::const_iterator is = m_HaulingStressAnalysisArtifacts.begin();
   std::map<Float64,pgsKdotHaulingStressAnalysisArtifact,Float64_less>::const_iterator iend = m_HaulingStressAnalysisArtifacts.end();
   for ( ; is!=iend; is++)
   {
      Float64 fciComp, fciTensNoRebar, fciTensWithRebar;
      is->second.GetRequiredConcreteStrength(&fciComp, &fciTensNoRebar, &fciTensWithRebar);

      // Use inline function for comparison
      maxFciComp        = CompareConcreteStrength(maxFciComp, fciComp);
      maxFciTensnobar   = CompareConcreteStrength(maxFciTensnobar, fciTensNoRebar);
      maxFciTenswithbar = CompareConcreteStrength(maxFciTenswithbar, fciTensWithRebar);
   }

   *pfciComp            = maxFciComp;
   *pfciTensNoRebar   = maxFciTensnobar;
   *pfciTensWithRebar = maxFciTenswithbar;
}

Float64 pgsKdotHaulingAnalysisArtifact::GetLeadingOverhang() const
{
   return m_LeadingOverhang;
}

Float64 pgsKdotHaulingAnalysisArtifact::GetTrailingOverhang() const
{
   return m_TrailingOverhang;
}

void pgsKdotHaulingAnalysisArtifact::SetHaulingPointsOfInterest(const std::vector<pgsPointOfInterest>& rPois)
{
   m_HaulingPois = rPois;
}

std::vector<pgsPointOfInterest> pgsKdotHaulingAnalysisArtifact::GetHaulingPointsOfInterest() const
{
   return m_HaulingPois;
}

void pgsKdotHaulingAnalysisArtifact::AddHaulingStressAnalysisArtifact(Float64 distFromStart,
                                   const pgsKdotHaulingStressAnalysisArtifact& artifact)
{
   m_HaulingStressAnalysisArtifacts.insert(std::make_pair(distFromStart,artifact));
}

const pgsKdotHaulingStressAnalysisArtifact* pgsKdotHaulingAnalysisArtifact::GetHaulingStressAnalysisArtifact(Float64 distFromStart) const
{
   std::map<Float64,pgsKdotHaulingStressAnalysisArtifact,Float64_less>::const_iterator found;
   found = m_HaulingStressAnalysisArtifacts.find( distFromStart );
   if ( found == m_HaulingStressAnalysisArtifacts.end() )
      return 0;

   return &(*found).second;
}

void pgsKdotHaulingAnalysisArtifact::SetDesignOutcome(DesignOutcome outcome)
{
   m_DesignOutcome = outcome;
}

pgsKdotHaulingAnalysisArtifact::DesignOutcome pgsKdotHaulingAnalysisArtifact::GetDesignOutcome() const
{
   return m_DesignOutcome;
}

void pgsKdotHaulingAnalysisArtifact::SetDesignOverhang(Float64 val)
{
   m_DesignOverhang = val;
}

Float64 pgsKdotHaulingAnalysisArtifact::GetDesignOverhang() const
{
   return m_DesignOverhang;
}

pgsHaulingAnalysisArtifact* pgsKdotHaulingAnalysisArtifact::Clone() const
{
   std::auto_ptr<pgsKdotHaulingAnalysisArtifact> clone(new pgsKdotHaulingAnalysisArtifact());
   *clone = *this;

   return clone.release();
}

void pgsKdotHaulingAnalysisArtifact::BuildHaulingCheckReport(SpanIndexType span,GirderIndexType girder,rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Check for Hauling to Bridge Site [5.5.4.3]")<<rptNewLine;
   *pTitle << _T("Hauling Stresses for Girder with KDOT Dynamic Effects")<<rptNewLine;

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, slen_u,   pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress_u, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   rptCapacityToDemand cap_demand;

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return;
   }

   Float64 loh = this->GetLeadingOverhang();
   Float64 toh = this->GetTrailingOverhang();
   Float64 min_oh = min(loh, toh);

   *p <<_T("The current hauling bunk point location is ")<<slen_u.SetValue(min_oh)<<_T(" - ");
   if(this->Passed())
   {
      *p << RPT_PASS <<rptNewLine<<rptNewLine;
   }
   else
   {
      *p << RPT_FAIL << rptNewLine<<rptNewLine;
   }

   Float64 hard_limit, soft_limit;
   this->GetOverhangLimits(&hard_limit, &soft_limit);

   if (min_oh < hard_limit)
   {
      *p <<color(Red)<<_T("Warning: the hauling bunk point location of ")<<slen_u.SetValue(min_oh)<<_T(" is less than the minimum allowable of ")<<slen_u.SetValue(hard_limit)<<_T(".")<<color(Black)<<rptNewLine;
   }
   else if (min_oh < soft_limit)
   {
      *p <<color(Red)<<_T("Warning: the hauling bunk point location of ")<<slen_u.SetValue(min_oh)<<_T(" conforms to the minimum allowable of ")<<slen_u.SetValue(hard_limit)
         <<_T(", but is less than the recommended minimum allowable of ")<<slen_u.SetValue(soft_limit)<<_T(". Hence, a note should be made in the plan set.")<<color(Black)<<rptNewLine;
   }

   DesignOutcome outcome = this->GetDesignOutcome();
   if(outcome==doNoDesignDone)
   {
      ATLASSERT(0); // the check should always attempt a design
      *p<<rptNewLine;
   }
   else if(outcome==doFailed)
   {
      *p<<_T(" - There is no location that will make the spec check pass.")<<rptNewLine;
   }
   else if(outcome==doSuccess || outcome==doSuccessInSoftZone)
   {
      Float64 desloc = this->GetDesignOverhang();

      *p<<_T(" - The maximum location for the spec check to pass is ")<<slen_u.SetValue(desloc)<<_T("."<<rptNewLine);
   }
   else
   {
      ATLASSERT(0); // new outcome type?
   }

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   Float64 c; // compression coefficient
   Float64 t; // tension coefficient
   Float64 t_max; // maximum allowable tension
   bool b_t_max; // true if max allowable tension is applicable

   c = pSpecEntry->GetHaulingCompStress();
   t = pSpecEntry->GetMaxConcreteTensHauling();
   pSpecEntry->GetAbsMaxConcreteTensHauling(&b_t_max,&t_max);

   Float64 t2 = pSpecEntry->GetMaxConcreteTensWithRebarHauling();

   Float64 capCompression = pGirderHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(span,girder);

   *p <<_T("Maximum allowable concrete compressive stress = -") << c << RPT_FC << _T(" = ") << 
      stress.SetValue(capCompression)<< _T(" ") <<
      stress.GetUnitTag()<< rptNewLine;
   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FC;
   if ( b_t_max )
      *p << _T(" but not more than: ") << stress.SetValue(t_max);
   *p << _T(" = ") << stress.SetValue(pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStress(span,girder))<< _T(" ") <<
      stress.GetUnitTag()<< rptNewLine;

   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t2) << symbol(ROOT) << RPT_FC
      << _T(" = ") << stress.SetValue(pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStress(span,girder)) << _T(" ") << stress.GetUnitTag()
      << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

   Float64 fc_reqd_comp,fc_reqd_tens, fc_reqd_tens_wrebar;
   this->GetRequiredConcreteStrength(&fc_reqd_comp,&fc_reqd_tens,&fc_reqd_tens_wrebar);

   *p << RPT_FC << _T(" required for Compressive stress = ");
   if ( 0 < fc_reqd_comp )
      *p << stress_u.SetValue( fc_reqd_comp ) << rptNewLine;
   else
      *p << symbol(INFINITY) << rptNewLine;

   *p << RPT_FC << _T(" required for Tensile stress without sufficient reinforcement = ");
   if ( 0 < fc_reqd_tens )
      *p << stress_u.SetValue( fc_reqd_tens ) << rptNewLine;
   else
      *p << symbol(INFINITY) << rptNewLine;

   *p << RPT_FC << _T(" required for Tensile stress with sufficient reinforcement to resist the tensile force in the concrete = ");
   if ( 0 < fc_reqd_tens_wrebar )
      *p << stress_u.SetValue( fc_reqd_tens_wrebar ) << rptNewLine;
   else
      *p << symbol(INFINITY) << rptNewLine;

   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,girder,POI_FLEXURESTRESS);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(7,_T(""));
   *p << p_table;

   int col1=0;
   int col2=0;
   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);

   (*p_table)(0,col1++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Demand");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Tensile") << rptNewLine << _T("Capacity");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,SKIP_CELL);

   Float64 overhang = this->GetTrailingOverhang();

   RowIndexType row=2;
   for (std::vector<pgsPointOfInterest>::const_iterator i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsKdotHaulingStressAnalysisArtifact* stressArtifact = this->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());

      if (stressArtifact==NULL)
      {
         ATLASSERT(0); // this should not happen
         continue;
      }
      (*p_table)(row,0) << location.SetValue( pgsTypes::Hauling,poi,overhang );

      // Tension
      Float64 fTensTop, fTensBottom, tensCapacity;
      stressArtifact->GetConcreteTensileStress(&fTensTop, &fTensBottom, &tensCapacity);

      // Compression
      Float64 fPsTop, fTop;
      stressArtifact->GetTopFiberStress(&fPsTop, &fTop);

      Float64 fPsBot, fBot;
      stressArtifact->GetBottomFiberStress(&fPsBot, &fBot);

      ColumnIndexType col = 1;
      (*p_table)(row,col++) << stress.SetValue(fTop);
      (*p_table)(row,col++) << stress.SetValue(fBot);

      Float64 fTens(0.0); // controlling tension
      if (fTop>0)
      {
         fTens = fTop;
         (*p_table)(row,col++) << stress.SetValue(tensCapacity);
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      if (fBot>0)
      {
         fTens = fBot;
         (*p_table)(row,col++) << stress.SetValue(tensCapacity);
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      // C/D's
      if ( stressArtifact->TensionPassed() )
          (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(tensCapacity,fTens,true)<<_T(")");
      else
          (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(tensCapacity,fTens,false)<<_T(")");

      Float64 fComp = min(fTop, fBot);
      
      if ( stressArtifact->CompressionPassed() )
          (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,true)<<_T(")");
      else
          (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,false)<<_T(")");

      row++;
   }
}

void  pgsKdotHaulingAnalysisArtifact::BuildHaulingDetailsReport(SpanIndexType span,GirderIndexType girder, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,            pDisplayUnits->GetSpanLengthUnit(),    false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, mod_e,          pDisplayUnits->GetModEUnit(),          false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,         pDisplayUnits->GetMomentUnit(),        false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle,           pDisplayUnits->GetRadAngleUnit(),      false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, wt_len, pDisplayUnits->GetForcePerLengthUnit(),false );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area,           pDisplayUnits->GetAreaUnit(),      false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_I,  pDisplayUnits->GetMomentOfInertiaUnit(),         true );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Details for Check for Hauling to Bridge Site [5.5.4.3]")<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;


   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
   }

   *p << Sub2(_T("l"),_T("g")) << _T(" = Overall Length of girder = ")<<loc.SetValue(this->GetGirderLength())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;

   Float64 leadingOH  = this->GetLeadingOverhang();
   Float64 trailingOH = this->GetTrailingOverhang();

   FormatDimension(leadingOH,pDisplayUnits->GetSpanLengthUnit());
   *p << _T("Leading Overhang = ")<<loc.SetValue(leadingOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("Trailing Overhang = ")<<loc.SetValue(trailingOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("l"),_T("l"))<<_T(" = Clear span length between supports = ")<<loc.SetValue(this->GetClearSpanBetweenSupportLocations())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("w = average girder weight/length = ")<<wt_len.SetValue(this->GetAvgGirderWeightPerLength())<<_T(" ")<<_T(" ")<<wt_len.GetUnitTag()<<rptNewLine;
   *p << _T("W = girder weight = ")<<force.SetValue(this->GetGirderWeight())<<_T(" ")<<_T(" ")<<force.GetUnitTag()<<rptNewLine;

   GET_IFACE2(pBroker,IKdotGirderHaulingSpecCriteria,pKdotCriteria);

   Float64 overhangG, interiorG;
   pKdotCriteria->GetHaulingGFactors(&overhangG, &interiorG);
   *p << _T("Dynamic factor applied at overhangs = ") << scalar.SetValue(overhangG)<<rptNewLine;
   *p << _T("Dynamic factor applied in-span = ") << scalar.SetValue(interiorG);

   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Hauling Forces"));
   *p << p_table<<rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(_T("Effective") << rptNewLine << _T("Prestress") << rptNewLine << _T("Force"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*p_table)(0,2) << COLHDR(_T("Eccentricity"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,3) << COLHDR(_T("Dynamic") << rptNewLine << _T("Moment"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   Float64 overhang = this->GetTrailingOverhang();

   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,girder,POI_FLEXURESTRESS);

   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsKdotHaulingStressAnalysisArtifact* stressArtifact =  this->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( pgsTypes::Hauling, poi, overhang );
      (*p_table)(row,1) << force.SetValue( stressArtifact->GetEffectiveHorizPsForce());
      (*p_table)(row,2) << dim.SetValue( stressArtifact->GetEccentricityPsForce());

      Float64 M = stressArtifact->GetMoment();
      (*p_table)(row,3) << moment.SetValue(M);

      row++;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(7,_T("Hauling Stresses"));
   *p << p_table;

   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,SKIP_CELL);
   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,1,3);
   (*p_table)(0,1) << _T("Top Stress, ") << RPT_FTOP;

   p_table->SetColumnSpan(0,2,3);
   (*p_table)(0,2) << _T("Bottom Stress ") << RPT_FBOT;

   p_table->SetColumnSpan(0,3,SKIP_CELL);
   p_table->SetColumnSpan(0,4,SKIP_CELL);
   p_table->SetColumnSpan(0,5,SKIP_CELL);
   p_table->SetColumnSpan(0,6,SKIP_CELL);

   (*p_table)(1,1) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,2) << COLHDR(_T("Dynamic") << rptNewLine << _T("Force"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,3) << COLHDR(_T("Total"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,4) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,5) << COLHDR(_T("Dynamic") << rptNewLine << _T("Force"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,6) << COLHDR(_T("Total"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   RowIndexType row1 = 2;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsKdotHaulingStressAnalysisArtifact* stressArtifact =  this->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());
 
      (*p_table)(row1,0) << location.SetValue( pgsTypes::Hauling, poi,overhang );
      
      Float64 ps, tot, dmo;
      stressArtifact->GetTopFiberStress(&ps, &tot);
      dmo = tot - ps;
      (*p_table)(row1,1) << stress.SetValue( ps );
      (*p_table)(row1,2) << stress.SetValue( dmo );
      (*p_table)(row1,3) << stress.SetValue( tot );
      
      stressArtifact->GetBottomFiberStress(&ps,&tot);
      dmo = tot - ps;
      (*p_table)(row1,4) << stress.SetValue( ps );
      (*p_table)(row1,5) << stress.SetValue( dmo );
      (*p_table)(row1,6) << stress.SetValue( tot );

      row1++;
   }

   // Rebar requirements tables
   BuildRebarTable(pBroker, pChapter, span, girder);

}

void pgsKdotHaulingAnalysisArtifact::BuildRebarTable(IBroker* pBroker,rptChapter* pChapter, SpanIndexType span, GirderIndexType girder) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area,        pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   rptCapacityToDemand cap_demand;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   Float64 overhang = this->GetTrailingOverhang();

   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> vPoi = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,girder,POI_FLEXURESTRESS);
   CHECK(vPoi.size()>0);

   std::_tstring tablename(_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2] - Hauling"));

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(10,tablename);
   *p << pTable << rptNewLine;

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << _T("Tension") << rptNewLine << _T("Face");
   (*pTable)(0,col++) << COLHDR(Sub2(_T("Y"),_T("na")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("f"),_T("ci"))<<rptNewLine<<_T("Demand"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("* A"),_T("s"))<< rptNewLine << _T("Provided"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("s"))<< rptNewLine << _T("Required"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Tensile")<<rptNewLine<<_T("Capacity"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) <<_T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   Int16 row=1;
   for (std::vector<pgsPointOfInterest>::iterator i = vPoi.begin(); i!= vPoi.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsKdotHaulingStressAnalysisArtifact* stressArtifact =  this->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());
      if(stressArtifact==NULL)
      {
         ATLASSERT(0);
         continue;
      }

      Float64 Yna, At, T, AsProvd, AsReqd, fAllow;
      stressArtifact->GetAlternativeTensileStressParameters(&Yna, &At, &T, &AsProvd, &AsReqd, &fAllow);

      (*pTable)(row,0) << location.SetValue( pgsTypes::Hauling, poi, overhang );

      if (Yna < 0 )
      {
         // Entire section is in compression - blank out row
         (*pTable)(row,1) << _T("Neither");
         for ( ColumnIndexType ic = 2; ic < pTable->GetNumberOfColumns(); ic++ )
         {
           (*pTable)(row,ic) << _T("-");
         }
      }
      else
      {
         // Stress demand
         Float64 fpsTop, fTop;
         Float64 fpsBottom, fBottom;
         stressArtifact->GetTopFiberStress(&fpsTop, &fTop);
         stressArtifact->GetBottomFiberStress(&fpsBottom, &fBottom);

         Float64 fTens;
         if (fTop>0.0)
         {
            fTens = fTop;
            (*pTable)(row,1) << _T("Top");
         }
         else
         {
            fTens = fBottom;
            (*pTable)(row,1) << _T("Bottom");
         }

         (*pTable)(row,2) << dim.SetValue(Yna);
         (*pTable)(row,3) << stress.SetValue(fTens);
         (*pTable)(row,4) << area.SetValue(At);
         (*pTable)(row,5) << force.SetValue(T);
         (*pTable)(row,6) << area.SetValue(AsProvd);
         (*pTable)(row,7) << area.SetValue(AsReqd);
         (*pTable)(row,8) << stress.SetValue(fAllow);
         (*pTable)(row,9) <<_T("(")<< cap_demand.SetValue(fAllow,fTens,true)<<_T(")");
      }

      row++;
   }

   *p << _T("* Bars must be fully developed and lie within tension area of section before they are considered.");
}

void pgsKdotHaulingAnalysisArtifact::Write1250Data(SpanIndexType span,GirderIndexType gdr,std::_tofstream& resultsFile, std::_tofstream& poiFile, IBroker* pBroker,
                                                    const std::_tstring& pid, const std::_tstring& bridgeId) const
{
   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);

   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,gdr,POI_BUNKPOINT|POI_HARPINGPOINT|POI_MIDSPAN,POIFIND_OR);

   for (std::vector<pgsPointOfInterest>::iterator it=poi_vec.begin(); it!=poi_vec.end(); it++)
   {
      pgsPointOfInterest& poi = *it;
      Float64 loc = poi.GetDistFromStart();

      const pgsKdotHaulingStressAnalysisArtifact* hStress = this->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());

      Float64 fTop, fBottom, Capacity;
      hStress->GetConcreteTensileStress(&fTop, &fBottom, &Capacity);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100005, ")<<loc<<_T(", ")<< ::ConvertFromSysUnits(fTop , unitMeasure::MPa) <<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100006, ")<<loc<<_T(", ")<< ::ConvertFromSysUnits(fBottom , unitMeasure::MPa) <<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100007, ")<<loc<<_T(", ")<< ::ConvertFromSysUnits(Capacity , unitMeasure::MPa) <<_T(", 50, ")<<gdr<<std::endl;
   }
}

Float64 pgsKdotHaulingAnalysisArtifact::GetGirderLength() const
{
   return m_GirderLength;
}

void pgsKdotHaulingAnalysisArtifact::SetGirderLength(Float64 val)
{
   m_GirderLength = val;
}

void pgsKdotHaulingAnalysisArtifact::SetOverhangs(Float64 trailing,Float64 leading)
{
   m_LeadingOverhang  = leading;
   m_TrailingOverhang = trailing;
}

void pgsKdotHaulingAnalysisArtifact::SetOverhangLimits(Float64 hardLimit, Float64 softLimit)
{
   m_HardOverhangLimit = hardLimit;
   m_SoftOverhangLimit = softLimit;
}

void pgsKdotHaulingAnalysisArtifact::GetOverhangLimits(Float64* hardLimit, Float64* softLimit) const
{
   *hardLimit = m_HardOverhangLimit;
   *softLimit = m_SoftOverhangLimit;
}

void pgsKdotHaulingAnalysisArtifact::SetGirderWeight(Float64 val)
{
   m_GirderWeight = val;
}

Float64 pgsKdotHaulingAnalysisArtifact::GetGirderWeight() const
{
   return m_GirderWeight;
}

Float64 pgsKdotHaulingAnalysisArtifact::GetAvgGirderWeightPerLength() const
{
   return m_GirderWeight/m_GirderLength;
}

void  pgsKdotHaulingAnalysisArtifact::SetGFactors(Float64 gOverhang, Float64 gInterior)
{
   m_gOverhang = gOverhang;
   m_gInterior = gInterior;
}

void  pgsKdotHaulingAnalysisArtifact::GetGFactors(Float64* gOverhang, Float64* gInterior) const
{
   *gOverhang = m_gOverhang;
   *gInterior = m_gInterior;
}

Float64 pgsKdotHaulingAnalysisArtifact::GetClearSpanBetweenSupportLocations() const
{
   return m_ClearSpanBetweenSupportLocations;
}

void pgsKdotHaulingAnalysisArtifact::SetClearSpanBetweenSupportLocations(Float64 val)
{
   m_ClearSpanBetweenSupportLocations = val;
}

Float64 pgsKdotHaulingAnalysisArtifact::GetElasticModulusOfGirderConcrete() const
{
   return m_ElasticModulusOfGirderConcrete;
}

void pgsKdotHaulingAnalysisArtifact::SetElasticModulusOfGirderConcrete(Float64 val)
{
   m_ElasticModulusOfGirderConcrete = val;
}

//======================== OPERATIONS =======================================
void pgsKdotHaulingAnalysisArtifact::MakeCopy(const pgsKdotHaulingAnalysisArtifact& rOther)
{
   m_GirderLength = rOther.m_GirderLength;
   m_GirderWeightPerLength = rOther.m_GirderWeightPerLength;
   m_GirderWeight = rOther.m_GirderWeight;

   m_gOverhang = rOther.m_gOverhang;
   m_gInterior = rOther.m_gInterior;

   m_ClearSpanBetweenSupportLocations = rOther.m_ClearSpanBetweenSupportLocations;

   m_HaulingStressAnalysisArtifacts = rOther.m_HaulingStressAnalysisArtifacts;
   m_HaulingPois = rOther.m_HaulingPois;

   m_LeadingOverhang  = rOther.m_LeadingOverhang;
   m_TrailingOverhang = rOther.m_TrailingOverhang;

   m_HardOverhangLimit = rOther.m_HardOverhangLimit;
   m_SoftOverhangLimit = rOther.m_SoftOverhangLimit;

   m_ElasticModulusOfGirderConcrete = rOther.m_ElasticModulusOfGirderConcrete;

   m_HaulingPois = rOther.m_HaulingPois;
   m_HaulingStressAnalysisArtifacts = rOther.m_HaulingStressAnalysisArtifacts;

   m_DesignOutcome = rOther.m_DesignOutcome;
   m_DesignOverhang = rOther.m_DesignOverhang;
}

void pgsKdotHaulingAnalysisArtifact::MakeAssignment(const pgsKdotHaulingAnalysisArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
void pgsKdotHaulingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsKdotHaulingAnalysisArtifact") << endl;
   os <<_T(" Stress Artifacts: ")<<endl;
   os << _T("================") <<endl;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for (iter=m_HaulingPois.begin(); iter!=m_HaulingPois.end(); iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      Float64 loc = rpoi.GetDistFromStart();
      os <<_T("--- At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");
      std::map<Float64,pgsKdotHaulingStressAnalysisArtifact,Float64_less>::const_iterator found;
      found = m_HaulingStressAnalysisArtifacts.find( loc );

      os<<endl;
      Float64 fps, ftot, ftcap, fccap;
      found->second.GetTopFiberStress(&fps, &ftot);
      os<<_T("TopStress fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, ftot=")<<::ConvertFromSysUnits(ftot,unitMeasure::KSI)<<_T("ksi")<<endl;

      found->second.GetBottomFiberStress(&fps, &ftot);
      os<<_T("BotStress fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, ftot=")<<::ConvertFromSysUnits(ftot,unitMeasure::KSI)<<_T("ksi")<<endl;

      found->second.GetConcreteTensileStress(&fps, &ftot, &ftcap);
      fccap = found->second.GetCompressiveCapacity();
      bool passed = found->second.Passed();
      os<<_T("Capacity Tens = ")<<::ConvertFromSysUnits(ftcap,unitMeasure::KSI)<<_T("ksi, Comp =")<<::ConvertFromSysUnits(fccap,unitMeasure::KSI)<<_T("ksi, Passed =")<<passed<<endl;
   }

   os <<_T(" Dump Complete")<<endl;
   os << _T("=============") <<endl;
}
#endif // _DEBUG


