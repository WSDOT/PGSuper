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
#include <WBFLCore.h>
#include <IFace\Tools.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\DesignArtifact.h>
#include <DesignConfigUtil.h>
#include <EAF\EAFUtilities.h>
#include <PgsExt\BridgeDescription.h>

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
   Init(true);
}

pgsDesignArtifact::pgsDesignArtifact(SpanIndexType span,GirderIndexType gdr)
{
   Init(true);

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

IndexType pgsDesignArtifact::DoPreviouslyFailedDesignsExist() const
{
   return !m_PreviouslyFailedDesigns.empty();
}

void pgsDesignArtifact::AddFailedDesign(const arDesignOptions& options)
{
   m_PreviouslyFailedDesigns.push_back(options.doDesignForFlexure);
}

std::vector<arFlexuralDesignType> pgsDesignArtifact::GetPreviouslyFailedFlexuralDesigns() const
{
   return m_PreviouslyFailedDesigns;
}

SpanIndexType pgsDesignArtifact::GetSpan() const
{
   return m_Span;
}

GirderIndexType pgsDesignArtifact::GetGirder() const
{
   return m_Gdr;
}

void pgsDesignArtifact::InitializeDesign(const arDesignOptions& options)
{
   // Reset the artifact data
   Init(false);

   SetDesignOptions(options);
}

void pgsDesignArtifact::SetDesignOptions(const arDesignOptions& options)
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

pgsTypes::AdjustableStrandType pgsDesignArtifact::GetAdjustableStrandType() const
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
      ATLASSERT(0);
      return pgsTypes::asStraight;
   }
}

void pgsDesignArtifact::SetRaisedAdjustableStrands(const ConfigStrandFillVector& strandFill)
{
   ATLASSERT(dtDesignFullyBondedRaised==m_DesignOptions.doDesignForFlexure || 
             dtDesignForDebondingRaised==m_DesignOptions.doDesignForFlexure);

   m_RaisedAdjustableStrandFill = strandFill;
}

ConfigStrandFillVector pgsDesignArtifact::GetRaisedAdjustableStrands() const
{
   ATLASSERT(dtDesignFullyBondedRaised==m_DesignOptions.doDesignForFlexure ||
             dtDesignForDebondingRaised==m_DesignOptions.doDesignForFlexure);

   return m_RaisedAdjustableStrandFill;
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

DebondConfigCollection pgsDesignArtifact::GetStraightStrandDebondInfo() const
{
   return m_SsDebondInfo;
}

void pgsDesignArtifact::SetStraightStrandDebondInfo(const DebondConfigCollection& dbinfo)
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
   m_Concrete.SetFc(fc);

   // update Ec if not input by user
   if (!m_IsUserEc)
   {
      Float64 density = m_Concrete.GetDensity();
      Float64 Ec = lrfdConcreteUtil::ModE(fc,density,false);
      m_Concrete.SetE(Ec);
   }
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
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   GDRCONFIG config;
   PRESTRESSCONFIG& rpsconfig(config.PrestressConfig); // use reference as a shortcut

   rpsconfig.SetStrandFill(pgsTypes::Straight,  pStrandGeometry->ComputeStrandFill(GetSpan(), GetGirder(),pgsTypes::Straight,  GetNumStraightStrands()));

   // Raised designs store strand fill directly
   if(dtDesignFullyBondedRaised==m_DesignOptions.doDesignForFlexure || 
      dtDesignForDebondingRaised==m_DesignOptions.doDesignForFlexure)
   {
      rpsconfig.SetStrandFill(pgsTypes::Harped, m_RaisedAdjustableStrandFill );
   }
   else
   {
      rpsconfig.SetStrandFill(pgsTypes::Harped, pStrandGeometry->ComputeStrandFill(GetSpan(), GetGirder(), pgsTypes::Harped, GetNumHarpedStrands()));
   }

   rpsconfig.SetStrandFill(pgsTypes::Temporary, pStrandGeometry->ComputeStrandFill(GetSpan(), GetGirder(),pgsTypes::Temporary, GetNumTempStrands()));

   rpsconfig.Pjack[pgsTypes::Straight]  = GetPjackStraightStrands();
   rpsconfig.Pjack[pgsTypes::Harped]    = GetPjackHarpedStrands();
   rpsconfig.Pjack[pgsTypes::Temporary] = GetPjackTempStrands();

   rpsconfig.EndOffset = GetHarpStrandOffsetEnd();
   rpsconfig.HpOffset  = GetHarpStrandOffsetHp();

   rpsconfig.AdjustableStrandType = GetAdjustableStrandType();
   
   rpsconfig.Debond[pgsTypes::Straight] = m_SsDebondInfo; // we only design debond for straight strands

   config.Fci       = GetReleaseStrength();
   config.Fc        = GetConcreteStrength();
   config.ConcType  = (pgsTypes::ConcreteType)m_Concrete.GetType();
   config.bHasFct   = m_Concrete.HasAggSplittingStrength();
   config.Fct       = m_Concrete.GetAggSplittingStrength();

   rpsconfig.TempStrandUsage = GetTemporaryStrandUsage();

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

   WriteShearDataToStirrupConfig(m_ShearData, config.StirrupConfig);

//   WriteLongitudinalRebarDataToConfig(m_LongitudinalRebarData, config.LongitudinalRebarConfig);

   return config;
}

CGirderData pgsDesignArtifact::GetGirderData() const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   // Start with girder data in current project and tweak for design
   CGirderData data = *(pGirderData->GetGirderData(m_Span, m_Gdr));

   if (this->GetDesignOptions().doDesignForFlexure != dtNoDesign)
   {
      ModGirderDataForFlexureDesign(pBroker,data);
   }

   if (this->GetDesignOptions().doDesignForShear)
   {
      ModGirderDataForShearDesign(pBroker,data);
   }

   return data;
}

void pgsDesignArtifact::ModGirderDataForFlexureDesign(IBroker* pBroker, CGirderData& rdata) const
{

   SpanIndexType   span  = this->GetSpan();
   GirderIndexType gdr   = this->GetGirder();

   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderTypes* pGirderTypes = pBridgeDesc->GetSpan(span)->GetGirderTypes();

   std::_tstring gdrName = pGirderTypes->GetGirderName(gdr);

   arDesignOptions design_options = this->GetDesignOptions();

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

      rdata.PrestressData.SetAdjustableStrandType(adjType);

      ConfigStrandFillVector harpfillvec = pStrandGeometry->ComputeStrandFill(span, gdr, pgsTypes::Harped, this->GetNumHarpedStrands());

      // Convert Adjustable strand offset data
      // offsets are absolute measure in the design artifact
      // Convert them to the measurement basis that the CGirderData object is using, unless it's the default,
      // then let's use a favorite
      if (hsoLEGACY == rdata.PrestressData.HsoEndMeasurement)
      {
         rdata.PrestressData.HsoEndMeasurement = hsoBOTTOM2BOTTOM;
      }

      rdata.PrestressData.HpOffsetAtEnd = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(gdrName.c_str(), adjType,
                                                                                          harpfillvec, 
                                                                                          rdata.PrestressData.HsoEndMeasurement, 
                                                                                          this->GetHarpStrandOffsetEnd());

      if (hsoLEGACY == rdata.PrestressData.HsoHpMeasurement)
      {
         rdata.PrestressData.HsoHpMeasurement = hsoBOTTOM2BOTTOM;
      }

      rdata.PrestressData.HpOffsetAtHp = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), adjType,
                                                                                        harpfillvec, 
                                                                                        rdata.PrestressData.HsoHpMeasurement, 
                                                                                        this->GetHarpStrandOffsetHp());

      // See if strand design data fits in grid
      bool fills_grid=false;
      StrandIndexType num_permanent = this->GetNumHarpedStrands() + this->GetNumStraightStrands();
      StrandIndexType ns(0), nh(0);
      if (design_options.doStrandFillType == ftGridOrder)
      {
         // we asked design to fill using grid, but this may be a non-standard design - let's check
         if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, span, gdr, &ns, &nh))
         {
            if (ns == this->GetNumStraightStrands() && nh ==  this->GetNumHarpedStrands() )
            {
               fills_grid = true;
            }
         }
      }

      if ( fills_grid )
      {
         ATLASSERT(num_permanent==ns+nh);
         rdata.PrestressData.SetTotalPermanentNstrands(num_permanent, ns, nh);
         rdata.PrestressData.Pjack[pgsTypes::Permanent]               = this->GetPjackStraightStrands() + this->GetPjackHarpedStrands();
         rdata.PrestressData.bPjackCalculated[pgsTypes::Permanent]    = this->GetUsedMaxPjackStraightStrands();
      }
      else
      {
         rdata.PrestressData.SetHarpedStraightNstrands(this->GetNumStraightStrands(), this->GetNumHarpedStrands());
      }

      rdata.PrestressData.SetTemporaryNstrands(this->GetNumTempStrands());

   }
   else
   {
      // Raised straight design
      rdata.PrestressData.SetAdjustableStrandType(pgsTypes::asStraight);

      // Raised straight adjustable strands are filled directly, but others use fill order.
      // must convert all to DirectStrandFillCollection
      ConfigStrandFillVector strvec = pStrandGeometry->ComputeStrandFill(span, gdr, pgsTypes::Straight, this->GetNumStraightStrands());
      DirectStrandFillCollection strfill =  ConvertConfigToDirectStrandFill(strvec);
      rdata.PrestressData.SetDirectStrandFillStraight(strfill);

      DirectStrandFillCollection harpfill =  ConvertConfigToDirectStrandFill(this->GetRaisedAdjustableStrands());
      rdata.PrestressData.SetDirectStrandFillHarped(harpfill);

      ConfigStrandFillVector tempvec = pStrandGeometry->ComputeStrandFill(span, gdr, pgsTypes::Temporary, this->GetNumTempStrands());
      DirectStrandFillCollection tempfill =  ConvertConfigToDirectStrandFill(tempvec);
      rdata.PrestressData.SetDirectStrandFillTemporary(tempfill);

      // Convert Adjustable strand offset data. This is typically zero from library, but must be converted to input datum
      // offsets are absolute measure in the design artifact
      // convert them to the measurement basis that the CGirderData object is using
      ConfigStrandFillVector harpfillvec = this->GetRaisedAdjustableStrands();
         
      rdata.PrestressData.HpOffsetAtEnd = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(gdrName.c_str(), pgsTypes::asStraight,
                                                                                          harpfillvec, 
                                                                                          rdata.PrestressData.HsoEndMeasurement, 
                                                                                          this->GetHarpStrandOffsetEnd());

      rdata.PrestressData.HpOffsetAtHp = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), pgsTypes::asStraight,
                                                                                        harpfillvec, 
                                                                                        rdata.PrestressData.HsoHpMeasurement, 
                                                                                        this->GetHarpStrandOffsetHp());
   }

   rdata.PrestressData.Pjack[pgsTypes::Harped]               = this->GetPjackHarpedStrands();
   rdata.PrestressData.Pjack[pgsTypes::Straight]             = this->GetPjackStraightStrands();
   rdata.PrestressData.Pjack[pgsTypes::Temporary]            = this->GetPjackTempStrands();
   rdata.PrestressData.bPjackCalculated[pgsTypes::Harped]    = this->GetUsedMaxPjackHarpedStrands();
   rdata.PrestressData.bPjackCalculated[pgsTypes::Straight]  = this->GetUsedMaxPjackStraightStrands();
   rdata.PrestressData.bPjackCalculated[pgsTypes::Temporary] = this->GetUsedMaxPjackTempStrands();
   rdata.PrestressData.LastUserPjack[pgsTypes::Harped]       = this->GetPjackHarpedStrands();
   rdata.PrestressData.LastUserPjack[pgsTypes::Straight]     = this->GetPjackStraightStrands();
   rdata.PrestressData.LastUserPjack[pgsTypes::Temporary]    = this->GetPjackTempStrands();

   rdata.PrestressData.TempStrandUsage = this->GetTemporaryStrandUsage();

   // Designer doesn't do extended strands
   rdata.PrestressData.ClearExtendedStrands(pgsTypes::Straight, pgsTypes::metStart);
   rdata.PrestressData.ClearExtendedStrands(pgsTypes::Straight, pgsTypes::metEnd);

   // Get debond information from design artifact
   rdata.PrestressData.ClearDebondData();
   rdata.PrestressData.bSymmetricDebond = true;  // design is always symmetric

   // TRICKY: Mapping from DEBONDCONFIG to CDebondInfo is tricky because
   //         former designates individual strands and latter stores strands
   //         in grid order.
   // Use utility tool to make the strand indexing conversion
   ConfigStrandFillVector strtfillvec = pStrandGeometry->ComputeStrandFill(span, gdr, pgsTypes::Straight, this->GetNumStraightStrands());
   ConfigStrandFillTool fillTool( strtfillvec );

   DebondConfigCollection dbcoll = this->GetStraightStrandDebondInfo();
   // sort this collection by strand idices to ensure we get it right
   std::sort( dbcoll.begin(), dbcoll.end() ); // default < operator is by index

   for (DebondConfigConstIterator dbit = dbcoll.begin(); dbit!=dbcoll.end(); dbit++)
   {
      const DEBONDCONFIG& rdbrinfo = *dbit;

      CDebondInfo cdbi;

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

      cdbi.Length1    = rdbrinfo.LeftDebondLength;
      cdbi.Length2    = rdbrinfo.RightDebondLength;

      rdata.PrestressData.Debond[pgsTypes::Straight].push_back(cdbi);
   }
   
   // concrete
   rdata.Material.Fci = this->GetReleaseStrength();
   if (!rdata.Material.bUserEci)
   {
      rdata.Material.Eci = lrfdConcreteUtil::ModE( rdata.Material.Fci, 
                                                             rdata.Material.StrengthDensity, 
                                                             false  ); // ignore LRFD range checks 
      rdata.Material.Eci *= (rdata.Material.EcK1*rdata.Material.EcK2);
   }

   rdata.Material.Fc  = this->GetConcreteStrength();
   if (!rdata.Material.bUserEc)
   {
      rdata.Material.Ec = lrfdConcreteUtil::ModE( rdata.Material.Fc, 
                                                            rdata.Material.StrengthDensity, 
                                                            false );// ignore LRFD range checks 
      rdata.Material.Ec *= (rdata.Material.EcK1*rdata.Material.EcK2);
   }

   // lifting
   if ( design_options.doDesignLifting )
   {
      rdata.HandlingData.LeftLiftPoint  = this->GetLeftLiftingLocation();
      rdata.HandlingData.RightLiftPoint = this->GetRightLiftingLocation();
   }

   // shipping
   if ( design_options.doDesignHauling )
   {
      rdata.HandlingData.LeadingSupportPoint  = this->GetLeadingOverhang();
      rdata.HandlingData.TrailingSupportPoint = this->GetTrailingOverhang();
   }
}

void pgsDesignArtifact::ModGirderDataForShearDesign(IBroker* pBroker, CGirderData& rdata) const
{
   GET_IFACE2(pBroker,IShear,pShear);

   // get the design data
   rdata.ShearData.ShearZones.clear();

   ZoneIndexType nShearZones = this->GetNumberOfStirrupZonesDesigned();
   if (0 < nShearZones)
   {
      rdata.ShearData =  this->GetShearData();
   }
   else
   {
      // if no shear zones were designed, we had a design failure.
      // create a single zone with no stirrups in it.
      CShearZoneData dat;
      rdata.ShearData.ShearZones.push_back(dat);
   }

   if(this->GetWasLongitudinalRebarForShearDesigned())
   {
      // Rebar data was changed during shear design
      rdata.LongitudinalRebarData  = this->GetLongitudinalRebarData();
   }

   // It is possible for shear stress to control final concrete strength
   // Make sure it is updated if no flexural design was requested
   if (this->GetDesignOptions().doDesignForFlexure == dtNoDesign)
   {
      rdata.Material.Fc  = this->GetConcreteStrength();
      if (!rdata.Material.bUserEc)
      {
         rdata.Material.Ec = lrfdConcreteUtil::ModE( rdata.Material.Fc, 
                                                               rdata.Material.StrengthDensity, 
                                                               false );// ignore LRFD range checks 
         rdata.Material.Ec *= (rdata.Material.EcK1*rdata.Material.EcK2);
      }
   }
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


void pgsDesignArtifact::ConcreteStrengthDesignState::SetStressState(bool controlledByMin, pgsTypes::Stage stage, pgsTypes::StressType stressType, 
              pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
{
   m_Action = actStress;
   m_MinimumControls = controlledByMin;
   m_Stage = stage;
   m_StressType = stressType;
   m_LimitState = limitState;
   m_StressLocation = stressLocation;
}

void pgsDesignArtifact::ConcreteStrengthDesignState::SetShearState(pgsTypes::Stage stage, pgsTypes::LimitState limitState)
{
   m_Action = actShear;
   m_MinimumControls = false;
   m_Stage = stage;
   m_LimitState = limitState;
}

bool pgsDesignArtifact::ConcreteStrengthDesignState::WasControlledByMinimum() const
{
   return m_MinimumControls;
}

pgsDesignArtifact::ConcreteStrengthDesignState::Action pgsDesignArtifact::ConcreteStrengthDesignState::GetAction() const
{
   return m_Action;
}

pgsTypes::Stage pgsDesignArtifact::ConcreteStrengthDesignState::Stage() const
{
   PRECONDITION(m_MinimumControls==false);
   return m_Stage;
}

pgsTypes::StressType pgsDesignArtifact::ConcreteStrengthDesignState::StressType() const
{
   PRECONDITION(m_MinimumControls==false && m_Action==actStress);
   return m_StressType;
}

pgsTypes::LimitState pgsDesignArtifact::ConcreteStrengthDesignState::LimitState() const
{
   PRECONDITION(m_MinimumControls==false);
   return m_LimitState;
}

pgsTypes::StressLocation pgsDesignArtifact::ConcreteStrengthDesignState::StressLocation() const
{
   PRECONDITION(m_MinimumControls==false && m_Action==actStress);
   return m_StressLocation;
}

void pgsDesignArtifact::ConcreteStrengthDesignState::SetRequiredAdditionalRebar(bool wasReqd)
{
   m_RequiredAdditionalRebar = wasReqd;
}

bool pgsDesignArtifact::ConcreteStrengthDesignState::GetRequiredAdditionalRebar() const
{
   ATLASSERT(m_Stage==pgsTypes::CastingYard || m_Stage==pgsTypes::Lifting || m_Stage==pgsTypes::Hauling);
   return m_RequiredAdditionalRebar;
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
   else if (m_Action==actStress)
   {
      std::_tostringstream sstr;
      sstr<< _T("flexural stress in ") << StageString(m_Stage)<<_T(", ")<<LimitStateString(m_LimitState)<<_T(", ")<<StressTypeString(m_StressType)<<_T(", at ")<<StressLocationString(m_StressLocation);
      return sstr.str();
   }
   else if (m_Action==actShear)
   {
      std::_tostringstream sstr;
      sstr<< _T("ultimate shear stress in ") << StageString(m_Stage)<<_T(", ")<<LimitStateString(m_LimitState);
      return sstr.str();
   }
   else
   {
      ATLASSERT(0);
      return std::_tstring(_T("unknown design state. "));
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
   else if (m_Action != rOther.m_Action)
   {
      return false;
   }
   else
   {
      if (m_Action==actStress)
      {
         return m_Stage==rOther.m_Stage && m_StressType==rOther.m_StressType &&
                m_LimitState==rOther.m_LimitState && m_StressLocation==rOther.m_StressLocation &&
                m_RequiredAdditionalRebar==rOther.m_RequiredAdditionalRebar;
      }
      else
      {
         ATLASSERT(rOther.m_Action==actShear);
         return m_Stage==rOther.m_Stage && m_LimitState==rOther.m_LimitState;
      }
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
void pgsDesignArtifact::Init(bool fromBirth)
{
   if (fromBirth)
   {
      m_Span = 0;
      m_Gdr  = 0;

      // want to keep track of previous design options
      m_PreviouslyFailedDesigns.clear();
   }

   m_Outcome = Success;

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
}
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
