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
#include <PgsExt\GirderArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsGirderArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsGirderArtifact::pgsGirderArtifact(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   m_SpanIdx = spanIdx;
   m_GirderIdx  = gdrIdx;
}

pgsGirderArtifact::pgsGirderArtifact(const pgsGirderArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsGirderArtifact::~pgsGirderArtifact()
{
}

//======================== OPERATORS  =======================================
pgsGirderArtifact& pgsGirderArtifact::operator= (const pgsGirderArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsGirderArtifact::SetStrandStressArtifact(const pgsStrandStressArtifact& artifact)
{
   m_StrandStressArtifact = artifact;
}

const pgsStrandStressArtifact* pgsGirderArtifact::GetStrandStressArtifact() const
{
   return &m_StrandStressArtifact;
}

pgsStrandStressArtifact* pgsGirderArtifact::GetStrandStressArtifact()
{
   return &m_StrandStressArtifact;
}

void pgsGirderArtifact::SetStrandSlopeArtifact(const pgsStrandSlopeArtifact& artifact)
{
   m_StrandSlopeArtifact = artifact;
}

const pgsStrandSlopeArtifact* pgsGirderArtifact::GetStrandSlopeArtifact() const
{
   return &m_StrandSlopeArtifact;
}

pgsStrandSlopeArtifact* pgsGirderArtifact::GetStrandSlopeArtifact()
{
   return &m_StrandSlopeArtifact;
}

void pgsGirderArtifact::SetHoldDownForceArtifact(const pgsHoldDownForceArtifact& artifact)
{
   m_HoldDownForceArtifact = artifact;
}

const pgsHoldDownForceArtifact* pgsGirderArtifact::GetHoldDownForceArtifact() const
{
   return &m_HoldDownForceArtifact;
}

pgsHoldDownForceArtifact* pgsGirderArtifact::GetHoldDownForceArtifact()
{
   return &m_HoldDownForceArtifact;
}

void pgsGirderArtifact::AddFlexuralStressArtifact(const pgsFlexuralStressArtifactKey& key,
                                                  const pgsFlexuralStressArtifact& artifact)
{
   m_FlexuralStressArtifacts.insert(std::make_pair(key,artifact));
}

const pgsFlexuralStressArtifact* pgsGirderArtifact::GetFlexuralStressArtifact(const pgsFlexuralStressArtifactKey& key) const
{
   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator found;
   found = m_FlexuralStressArtifacts.find( key );
   if ( found == m_FlexuralStressArtifacts.end() )
      return 0;

   return &(*found).second;
}

pgsFlexuralStressArtifact* pgsGirderArtifact::GetFlexuralStressArtifact(const pgsFlexuralStressArtifactKey& key)
{
   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::iterator found;
   found = m_FlexuralStressArtifacts.find( key );
   if ( found == m_FlexuralStressArtifacts.end() )
      return 0;

   return &(*found).second;
}

std::vector<pgsFlexuralStressArtifactKey> pgsGirderArtifact::GetFlexuralStressArtifactKeys() const
{
   std::vector<pgsFlexuralStressArtifactKey> keys;
   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator iter;
   for ( iter = m_FlexuralStressArtifacts.begin(); iter != m_FlexuralStressArtifacts.end(); iter++ )
   {
      keys.push_back(iter->first);
   }

   return keys;
}

void pgsGirderArtifact::AddFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifactKey& key,
                                                  const pgsFlexuralCapacityArtifact& pmartifact,
                                                  const pgsFlexuralCapacityArtifact& nmartifact)
{
   m_PositiveMomentFlexuralCapacityArtifacts.insert(std::make_pair(key,pmartifact));
   m_NegativeMomentFlexuralCapacityArtifacts.insert(std::make_pair(key,nmartifact));
}

std::vector<pgsFlexuralCapacityArtifactKey> pgsGirderArtifact::GetFlexuralCapacityArtifactKeys() const
{
   std::vector<pgsFlexuralCapacityArtifactKey> keys;
   std::map<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact>::const_iterator iter;
   for ( iter = m_PositiveMomentFlexuralCapacityArtifacts.begin(); iter != m_PositiveMomentFlexuralCapacityArtifacts.end(); iter++ )
   {
      keys.push_back(iter->first);
   }

   return keys;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetPositiveMomentFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifactKey& key) const
{
   std::map<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact>::const_iterator found;
   found = m_PositiveMomentFlexuralCapacityArtifacts.find( key );
   if ( found == m_PositiveMomentFlexuralCapacityArtifacts.end() )
      return 0;

   return &(*found).second;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetNegativeMomentFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifactKey& key) const
{
   std::map<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact>::const_iterator found;
   found = m_NegativeMomentFlexuralCapacityArtifacts.find( key );
   if ( found == m_NegativeMomentFlexuralCapacityArtifacts.end() )
      return 0;

   return &(*found).second;
}

const pgsStirrupCheckArtifact* pgsGirderArtifact::GetStirrupCheckArtifact() const
{
   return &m_StirrupCheckArtifact;
}

pgsStirrupCheckArtifact* pgsGirderArtifact::GetStirrupCheckArtifact()
{
   return &m_StirrupCheckArtifact;
}

const pgsPrecastIGirderDetailingArtifact* pgsGirderArtifact::GetPrecastIGirderDetailingArtifact() const
{
   return &m_PrecastIGirderDetailingArtifact;
}

pgsPrecastIGirderDetailingArtifact* pgsGirderArtifact::GetPrecastIGirderDetailingArtifact()
{
   return &m_PrecastIGirderDetailingArtifact;
}

void pgsGirderArtifact::SetConstructabilityArtifact(const pgsConstructabilityArtifact& artifact)
{
   m_ConstructabilityArtifact = artifact;
}

const pgsConstructabilityArtifact* pgsGirderArtifact::GetConstructabilityArtifact() const
{
   return &m_ConstructabilityArtifact;
}

pgsConstructabilityArtifact* pgsGirderArtifact::GetConstructabilityArtifact()
{
   return &m_ConstructabilityArtifact;
}

void pgsGirderArtifact::SetLiftingCheckArtifact(pgsLiftingCheckArtifact* artifact)
{
   m_pLiftingCheckArtifact = std::auto_ptr<pgsLiftingCheckArtifact>(artifact);
}

const pgsLiftingCheckArtifact* pgsGirderArtifact::GetLiftingCheckArtifact() const
{
   return m_pLiftingCheckArtifact.get();
}

void pgsGirderArtifact::SetHaulingCheckArtifact(pgsHaulingCheckArtifact* artifact)
{
   m_pHaulingCheckArtifact = std::auto_ptr<pgsHaulingCheckArtifact>(artifact);;
}

const pgsHaulingCheckArtifact* pgsGirderArtifact::GetHaulingCheckArtifact() const
{
   return m_pHaulingCheckArtifact.get();
}

pgsDeflectionCheckArtifact* pgsGirderArtifact::GetDeflectionCheckArtifact()
{
   return &m_DeflectionCheckArtifact;
}

const pgsDeflectionCheckArtifact* pgsGirderArtifact::GetDeflectionCheckArtifact() const
{
   return &m_DeflectionCheckArtifact;
}

void pgsGirderArtifact::SetCastingYardMildRebarRequirement(Float64 As)
{
    m_CastingYardAs = As;
}

Float64 pgsGirderArtifact::GetCastingYardMildRebarRequirement() const
{
    return m_CastingYardAs;
}

void pgsGirderArtifact::SetCastingYardCapacityWithMildRebar(Float64 fAllow)
{
    m_CastingYardAllowable = fAllow;
}

Float64 pgsGirderArtifact::GetCastingYardCapacityWithMildRebar() const
{
    return m_CastingYardAllowable;
}

pgsDebondArtifact* pgsGirderArtifact::GetDebondArtifact(pgsTypes::StrandType strandType)
{
   return &m_DebondArtifact[strandType];
}

const pgsDebondArtifact* pgsGirderArtifact::GetDebondArtifact(pgsTypes::StrandType strandType) const
{
   return &m_DebondArtifact[strandType];
}

bool pgsGirderArtifact::Passed() const
{
   bool bPassed = true;

   bPassed &= m_ConstructabilityArtifact.Pass();
   bPassed &= m_HoldDownForceArtifact.Passed();
   bPassed &= m_StrandSlopeArtifact.Passed();
   bPassed &= m_StrandStressArtifact.Passed();

   std::map<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact>::const_iterator i1;
   for ( i1 = m_PositiveMomentFlexuralCapacityArtifacts.begin(); i1 != m_PositiveMomentFlexuralCapacityArtifacts.end(); i1++ )
   {
      const std::pair<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact>& artifact = *i1;
      bPassed &= artifact.second.Passed();
   }

   for ( i1 = m_NegativeMomentFlexuralCapacityArtifacts.begin(); i1 != m_NegativeMomentFlexuralCapacityArtifacts.end(); i1++ )
   {
      const std::pair<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact>& artifact = *i1;
      bPassed &= artifact.second.Passed();
   }

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator  i2;
   for ( i2 = m_FlexuralStressArtifacts.begin(); i2 != m_FlexuralStressArtifacts.end(); i2++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i2;
      bPassed &= artifact.second.Passed(pgsFlexuralStressArtifact::WithRebar);
      bPassed &= artifact.second.Passed(pgsFlexuralStressArtifact::WithoutRebar);
   }

   bPassed &= m_StirrupCheckArtifact.Passed();

   bPassed &= m_PrecastIGirderDetailingArtifact.Passed();

   if (m_pLiftingCheckArtifact.get()!=NULL)
      bPassed &= m_pLiftingCheckArtifact->Passed();

   if (m_pHaulingCheckArtifact.get()!=NULL)
      bPassed &= m_pHaulingCheckArtifact->Passed();

   bPassed &= m_DeflectionCheckArtifact.Passed();

   for ( Uint16 i = 0; i < 3; i++ )
      bPassed &= m_DebondArtifact[i].Passed();

   return bPassed;
}

double pgsGirderArtifact::GetRequiredConcreteStrength(pgsTypes::Stage stage,pgsTypes::LimitState ls) const
{
   double fc_reqd = 0;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;
      const pgsFlexuralStressArtifactKey& key = artifact.first;

      if ( key.GetStage() == stage && key.GetLimitState() == ls )
      {
         double fc = artifact.second.GetRequiredConcreteStrength();

         if ( fc < 0 ) 
            return fc;

         if ( 0 < fc )
            fc_reqd = _cpp_max(fc,fc_reqd);
      }
   }

   return fc_reqd;
}

double pgsGirderArtifact::GetRequiredConcreteStrength() const
{
   double fc_reqd = 0;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;

      if ( artifact.first.GetStage() == pgsTypes::CastingYard )
         continue;

      double fc = artifact.second.GetRequiredConcreteStrength();

      if ( fc < 0 ) // there is no concrete strength that will work
         return fc;

      if ( 0 < fc )
         fc_reqd = _cpp_max(fc,fc_reqd);
   }

   if (m_pHaulingCheckArtifact.get()!=NULL)
   {
      double fc_reqd_hauling_tens,fc_reqd_hauling_comp;
      double fcMax = lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? ::ConvertToSysUnits(105,unitMeasure::MPa) : ::ConvertToSysUnits(15.0,unitMeasure::KSI);
      bool min_rebar_reqd;
      m_pHaulingCheckArtifact->GetRequiredConcreteStrength(&fc_reqd_hauling_comp,&fc_reqd_hauling_tens,&min_rebar_reqd,fcMax,false);

      double fc_reqd_hauling = max(fc_reqd_hauling_tens,fc_reqd_hauling_comp);

      if ( fc_reqd_hauling < 0 ) // there is no concrete strength that will work
         return fc_reqd_hauling;

      fc_reqd = _cpp_max(fc_reqd,fc_reqd_hauling);
   }

   return fc_reqd;
}

double pgsGirderArtifact::GetRequiredReleaseStrength() const
{
   double fc_reqd = 0;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;

      if ( artifact.first.GetStage() != pgsTypes::CastingYard )
         continue;

      double fc = artifact.second.GetRequiredConcreteStrength();

      if ( fc < 0 ) // there is no concrete strength that will work
         return fc;

      if ( 0 < fc )
         fc_reqd = _cpp_max(fc,fc_reqd);
   }

   if (m_pLiftingCheckArtifact.get()!=NULL)
   {
      double fc_reqd_lifting_tens,fc_reqd_lifting_comp;
      bool min_rebar_reqd;
      m_pLiftingCheckArtifact->GetRequiredConcreteStrength(&fc_reqd_lifting_comp,&fc_reqd_lifting_tens,&min_rebar_reqd);
      
      double fc_reqd_lifting = max(fc_reqd_lifting_tens,fc_reqd_lifting_comp);

      if ( fc_reqd_lifting < 0 ) // there is no concrete strength that will work
         return fc_reqd_lifting;

      fc_reqd = _cpp_max(fc_reqd,fc_reqd_lifting);
   }

   return fc_reqd;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsGirderArtifact::MakeCopy(const pgsGirderArtifact& rOther)
{
   m_SpanIdx = rOther.m_SpanIdx;
   m_GirderIdx = rOther.m_GirderIdx;

   m_StrandStressArtifact            = rOther.m_StrandStressArtifact;
   m_FlexuralStressArtifacts         = rOther.m_FlexuralStressArtifacts;
   m_PositiveMomentFlexuralCapacityArtifacts       = rOther.m_PositiveMomentFlexuralCapacityArtifacts;
   m_NegativeMomentFlexuralCapacityArtifacts       = rOther.m_NegativeMomentFlexuralCapacityArtifacts;
   m_StirrupCheckArtifact            = rOther.m_StirrupCheckArtifact;
   m_PrecastIGirderDetailingArtifact = rOther.m_PrecastIGirderDetailingArtifact;
   m_StrandSlopeArtifact             = rOther.m_StrandSlopeArtifact;
   m_HoldDownForceArtifact           = rOther.m_HoldDownForceArtifact;
   m_ConstructabilityArtifact        = rOther.m_ConstructabilityArtifact;

   if(rOther.m_pLiftingCheckArtifact.get()!=NULL)
   {
      m_pLiftingCheckArtifact = std::auto_ptr<pgsLiftingCheckArtifact>(new pgsLiftingCheckArtifact);
      *m_pLiftingCheckArtifact            = *rOther.m_pLiftingCheckArtifact;
   }

   if(rOther.m_pHaulingCheckArtifact.get()!=NULL)
   {
      m_pHaulingCheckArtifact = std::auto_ptr<pgsHaulingCheckArtifact>(new pgsHaulingCheckArtifact);
      *m_pHaulingCheckArtifact            = *rOther.m_pHaulingCheckArtifact;
   }


   m_DeflectionCheckArtifact         = rOther.m_DeflectionCheckArtifact;
   m_CastingYardAs                   = rOther.m_CastingYardAs;
   m_CastingYardAllowable            = rOther.m_CastingYardAllowable;

   for ( Uint16 i = 0; i < 3; i++ )
      m_DebondArtifact[i] = rOther.m_DebondArtifact[i];
}

void pgsGirderArtifact::MakeAssignment(const pgsGirderArtifact& rOther)
{
   MakeCopy( rOther );
}
