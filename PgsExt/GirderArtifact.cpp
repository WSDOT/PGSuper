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

void pgsGirderArtifact::SetLiftingAnalysisArtifact(pgsLiftingAnalysisArtifact* artifact)
{
   m_pLiftingAnalysisArtifact = std::auto_ptr<pgsLiftingAnalysisArtifact>(artifact);
}

const pgsLiftingAnalysisArtifact* pgsGirderArtifact::GetLiftingAnalysisArtifact() const
{
   return m_pLiftingAnalysisArtifact.get();
}

void pgsGirderArtifact::SetHaulingAnalysisArtifact(pgsHaulingAnalysisArtifact* artifact)
{
   m_pHaulingAnalysisArtifact = std::auto_ptr<pgsHaulingAnalysisArtifact>(artifact);;
}

const pgsHaulingAnalysisArtifact* pgsGirderArtifact::GetHaulingAnalysisArtifact() const
{
   return m_pHaulingAnalysisArtifact.get();
}

pgsDeflectionCheckArtifact* pgsGirderArtifact::GetDeflectionCheckArtifact()
{
   return &m_DeflectionCheckArtifact;
}

const pgsDeflectionCheckArtifact* pgsGirderArtifact::GetDeflectionCheckArtifact() const
{
   return &m_DeflectionCheckArtifact;
}


void pgsGirderArtifact::SetCastingYardCapacityWithMildRebar(Float64 fAllow)
{
    m_CastingYardAllowable = fAllow;
}

Float64 pgsGirderArtifact::GetCastingYardCapacityWithMildRebar() const
{
    return m_CastingYardAllowable;
}

void pgsGirderArtifact::SetTempStrandRemovalCapacityWithMildRebar(Float64 fAllow)
{
   m_TempStrandRemovalAllowable = fAllow;
}

Float64 pgsGirderArtifact::GetTempStrandRemovalCapacityWithMildRebar() const
{
   return m_TempStrandRemovalAllowable;
}

void pgsGirderArtifact::SetDeckCastingCapacityWithMildRebar(Float64 fAllow)
{
   m_DeckCastingAllowable = fAllow;
}

Float64 pgsGirderArtifact::GetDeckCastingCapacityWithMildRebar() const
{
   return m_DeckCastingAllowable;
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

   bPassed &= DidFlexuralStressesPass();

   bPassed &= m_StirrupCheckArtifact.Passed();

   bPassed &= m_PrecastIGirderDetailingArtifact.Passed();

   if (m_pLiftingAnalysisArtifact.get()!=NULL)
      bPassed &= m_pLiftingAnalysisArtifact->Passed();

   if (m_pHaulingAnalysisArtifact.get()!=NULL)
      bPassed &= m_pHaulingAnalysisArtifact->Passed();

   bPassed &= m_DeflectionCheckArtifact.Passed();

   for ( Uint16 i = 0; i < 3; i++ )
      bPassed &= m_DebondArtifact[i].Passed();

   return bPassed;
}

bool pgsGirderArtifact::DidFlexuralStressesPass() const
{
   bool bPassed = true;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator  i2;
   for ( i2 = m_FlexuralStressArtifacts.begin(); i2 != m_FlexuralStressArtifacts.end(); i2++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i2;
      bPassed &= artifact.second.Passed();
   }

   return bPassed;
}


Float64 pgsGirderArtifact::GetRequiredConcreteStrength(pgsTypes::Stage stage,pgsTypes::LimitState ls) const
{
   Float64 fc_reqd = 0;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;
      const pgsFlexuralStressArtifactKey& key = artifact.first;

      if ( key.GetStage() == stage && key.GetLimitState() == ls )
      {
         Float64 fc = artifact.second.GetRequiredConcreteStrength();

         if ( fc < 0 ) 
            return fc;

         if ( 0 < fc )
            fc_reqd = _cpp_max(fc,fc_reqd);
      }
   }

   return fc_reqd;
}

Float64 pgsGirderArtifact::GetRequiredConcreteStrength() const
{
   Float64 fc_reqd = 0;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;

      if ( artifact.first.GetStage() == pgsTypes::CastingYard )
         continue;

      Float64 fc = artifact.second.GetRequiredConcreteStrength();

      if ( fc < 0 ) // there is no concrete strength that will work
         return fc;

      if ( 0 < fc )
         fc_reqd = _cpp_max(fc,fc_reqd);
   }

   if (m_pLiftingAnalysisArtifact.get()!=NULL)
   {
      Float64 fc_reqd_Lifting_comp, fc_reqd_Lifting_tens, fc_reqd_Lifting_tens_wbar;
      m_pLiftingAnalysisArtifact->GetRequiredConcreteStrength(&fc_reqd_Lifting_comp,&fc_reqd_Lifting_tens, &fc_reqd_Lifting_tens_wbar);

      Float64 fc_reqd_Lifting = max(fc_reqd_Lifting_tens_wbar,fc_reqd_Lifting_comp);

      if ( fc_reqd_Lifting < 0 ) // there is no concrete strength that will work
         return fc_reqd_Lifting;

      fc_reqd = _cpp_max(fc_reqd,fc_reqd_Lifting);
   }

   if (m_pHaulingAnalysisArtifact.get()!=NULL)
   {
      Float64 fc_reqd_hauling_comp, fc_reqd_hauling_tens, fc_reqd_hauling_tens_wbar;
      m_pHaulingAnalysisArtifact->GetRequiredConcreteStrength(&fc_reqd_hauling_comp,&fc_reqd_hauling_tens, &fc_reqd_hauling_tens_wbar);

      Float64 fc_reqd_hauling = max(fc_reqd_hauling_tens_wbar,fc_reqd_hauling_comp);

      if ( fc_reqd_hauling < 0 ) // there is no concrete strength that will work
         return fc_reqd_hauling;

      fc_reqd = _cpp_max(fc_reqd,fc_reqd_hauling);
   }

   return fc_reqd;
}

Float64 pgsGirderArtifact::GetRequiredReleaseStrength() const
{
   Float64 fc_reqd = 0;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   {
      const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;

      if ( artifact.first.GetStage() != pgsTypes::CastingYard )
         continue;

      Float64 fc = artifact.second.GetRequiredConcreteStrength();

      if ( fc < 0 ) // there is no concrete strength that will work
         return fc;

      if ( 0 < fc )
         fc_reqd = _cpp_max(fc,fc_reqd);
   }

   if (m_pLiftingAnalysisArtifact.get()!=NULL)
   {
      Float64 fc_reqd_lifting_comp,fc_reqd_lifting_tens_norebar,fc_reqd_lifting_tens_withrebar;
      m_pLiftingAnalysisArtifact->GetRequiredConcreteStrength(&fc_reqd_lifting_comp,&fc_reqd_lifting_tens_norebar,&fc_reqd_lifting_tens_withrebar);

      Float64 fc_reqd_lifting = Max3(fc_reqd_lifting_comp,fc_reqd_lifting_tens_norebar,fc_reqd_lifting_tens_withrebar);

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

   if(rOther.m_pLiftingAnalysisArtifact.get()!=NULL)
   {
      m_pLiftingAnalysisArtifact = std::auto_ptr<pgsLiftingAnalysisArtifact>(new pgsLiftingAnalysisArtifact);
      *m_pLiftingAnalysisArtifact            = *rOther.m_pLiftingAnalysisArtifact;
   }

   if(rOther.m_pHaulingAnalysisArtifact.get()!=NULL)
   {
      m_pHaulingAnalysisArtifact = std::auto_ptr<pgsHaulingAnalysisArtifact>(rOther.m_pHaulingAnalysisArtifact->Clone());
   }


   m_DeflectionCheckArtifact         = rOther.m_DeflectionCheckArtifact;
   m_CastingYardAllowable            = rOther.m_CastingYardAllowable;
   m_TempStrandRemovalAllowable      = rOther.m_TempStrandRemovalAllowable;
   m_DeckCastingAllowable            = rOther.m_DeckCastingAllowable;

   for ( Uint16 i = 0; i < 3; i++ )
      m_DebondArtifact[i] = rOther.m_DebondArtifact[i];
}

void pgsGirderArtifact::MakeAssignment(const pgsGirderArtifact& rOther)
{
   MakeCopy( rOther );
}
