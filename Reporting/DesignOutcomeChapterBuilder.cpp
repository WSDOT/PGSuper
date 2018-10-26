///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include "StdAfx.h"

#include <Reporting\DesignOutcomeChapterBuilder.h>

#include <IFace\Project.h>
#include <IFace\DisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>
#include <IFace\AnalysisResults.h>

#include <Lrfd\RebarPool.h>

#include <PgsExt\DesignArtifact.h>
#include <PgsExt\GirderData.h>

#include <PgsExt\BridgeDescription.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDesignOutcomeChapterBuilder
****************************************************************************/

void write_artifact_data(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit,const pgsDesignArtifact* pArtifact);
void failed_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit,const pgsDesignArtifact* pArtifact);
void successful_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit,const pgsDesignArtifact* pArtifact);

CDesignOutcomeChapterBuilder::CDesignOutcomeChapterBuilder()
{
}

LPCTSTR CDesignOutcomeChapterBuilder::GetName() const
{
   return TEXT("Design Outcome");
}

rptChapter* CDesignOutcomeChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pReportSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   ATLASSERT( pReportSpec != NULL );

   CComPtr<IBroker> pBroker;
   pReportSpec->GetBroker(&pBroker);

   SpanIndexType span = pReportSpec->GetSpan();
   GirderIndexType gdr = pReportSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2( pBroker, IDisplayUnits, pDispUnit );
   GET_IFACE2( pBroker, IArtifact, pIArtifact );
   const pgsDesignArtifact* pArtifact = pIArtifact->GetDesignArtifact(span,gdr);

   if ( pArtifact == NULL )
   {
      rptParagraph* pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << "This girder has not been designed" << rptNewLine;
      return pChapter;
   }

   if ( pArtifact->GetOutcome() == pgsDesignArtifact::Success )
   {
      successful_design(pBroker,span,gdr,pChapter,pDispUnit,pArtifact);
   }
   else
   {
      failed_design(pBroker,span,gdr,pChapter,pDispUnit,pArtifact);
   }

   return pChapter;
}

CChapterBuilder* CDesignOutcomeChapterBuilder::Clone() const
{
   return new CDesignOutcomeChapterBuilder;
}

void write_artifact_data(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit,const pgsDesignArtifact* pArtifact)
{

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDispUnit->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDispUnit->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, distance, pDispUnit->GetXSectionDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDispUnit->GetStressUnit(),       true );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   arDesignOptions options = pArtifact->GetDesignOptions();

   if (pArtifact->GetDoDesignFlexure()!=dtNoDesign)
   {
      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;

      // see if fill order type was changed
      if (pArtifact->GetDesignOptions().doStrandFillType==ftGridOrder)
      {
         Int32 num_permanent = pArtifact->GetNumHarpedStrands() + pArtifact->GetNumStraightStrands();
         // we asked design to fill using grid, but this may be a non-standard design - let's check
         GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

         StrandIndexType ns, nh;
         if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, span, gdr, &ns, &nh))
         {
            if (ns!=pArtifact->GetNumStraightStrands() )
            {
               pParagraph = new rptParagraph();
               *pChapter << pParagraph;
               *pParagraph << color(Blue)<<"Note that strand fill order has been changed from "<<Bold("Number of Permanent")<<" to "<< Bold("Number of Straight and Number of Harped") << color(Black) << " strands.";
            }
         }
      }

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3,"");
      *pParagraph << pTable;

      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      int row=0;


      (*pTable)(row,0) << "Parameter";
      (*pTable)(row,1) << "Proposed Design";
      (*pTable)(row,2) << "Current Value";

      row++;

      GDRCONFIG config = pArtifact->GetGirderConfiguration();

      GET_IFACE2(pBroker, IGirderData, pGirderData);
      CGirderData girderData = pGirderData->GetGirderData(span,gdr);

      GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
      GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
      GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

      // current offsets, measured in absolute
      Float64 abs_offset_end, abs_offset_hp;
      pStrandGeometry->GetHarpStrandOffsets(span,gdr,&abs_offset_end,&abs_offset_hp);

      (*pTable)(row,0) << "Number of Straight Strands";
      (*pTable)(row,1) << config.Nstrands[pgsTypes::Straight];
      (*pTable)(row,2) << girderData.Nstrands[pgsTypes::Straight];

      // print straight debond information if exists
      long ddb = config.Debond[pgsTypes::Straight].size();
      long pdb = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Straight);
      if (ddb>0 || pdb>0)
      {
         (*pTable)(row,1) << " ("<<ddb<<" debonded)";
         (*pTable)(row,2) << " ("<<pdb<<" debonded)";
      }

      row++;

      (*pTable)(row,0) << "Number of Harped Strands";
      (*pTable)(row,1) << config.Nstrands[pgsTypes::Harped];
      (*pTable)(row,2) << girderData.Nstrands[pgsTypes::Harped];
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(span,gdr,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << "Number of Temporary Strands";
         (*pTable)(row,1) << config.Nstrands[pgsTypes::Temporary];
         (*pTable)(row,2) << girderData.Nstrands[pgsTypes::Temporary];
         row++;
      }

      (*pTable)(row,0) << "Straight Strand Jacking Force";
      (*pTable)(row,1) << force.SetValue(config.Pjack[pgsTypes::Straight]);
      (*pTable)(row,2) << force.SetValue(girderData.Pjack[pgsTypes::Straight]);
      row++;

      (*pTable)(row,0) << "Harped Strand Jacking Force";
      (*pTable)(row,1) << force.SetValue(config.Pjack[pgsTypes::Harped]);
      (*pTable)(row,2) << force.SetValue(girderData.Pjack[pgsTypes::Harped]);
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(span,gdr,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << "Temporary Strand Jacking Force";
         (*pTable)(row,1) << force.SetValue(config.Pjack[pgsTypes::Temporary]);
         (*pTable)(row,2) << force.SetValue(girderData.Pjack[pgsTypes::Temporary]);
         row++;
      }

      if (config.Nstrands[pgsTypes::Harped] > 0)
      {
         HarpedStrandOffsetType HsoEnd = girderData.HsoEndMeasurement;
         switch( HsoEnd )
         {
         case hsoCGFROMTOP:
            (*pTable)(row,0) << "Distance from top of girder to" << rptNewLine << "CG of harped strand group at ends of girder";
            break;

         case hsoCGFROMBOTTOM:
            (*pTable)(row,0) << "Distance from bottom of girder to" << rptNewLine << "CG of harped strand group at ends of girder";
            break;

         case hsoLEGACY:
            // convert legacy to display TOP 2 TOP

            HsoEnd = hsoTOP2TOP;

         case hsoTOP2TOP:
            (*pTable)(row,0) << "Distance from top of girder to" << rptNewLine << "top of harped strand group at ends of girder";
            break;

         case hsoTOP2BOTTOM:
            (*pTable)(row,0) << "Distance from bottom of girder" << rptNewLine << "to top of harped strand group at ends of girder";
            break;

         case hsoBOTTOM2BOTTOM:
            (*pTable)(row,0) << "Distance from bottom of girder" << rptNewLine << "to bottom of harped strand group at ends of girder";
            break;

         case hsoECCENTRICITY:
            (*pTable)(row,0) << "Eccentricity of harped strand" << rptNewLine << "group at ends of girder";
            break;

         default:
            ATLASSERT(false); // should never get here
         }

         double offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr,
                                                                             pArtifact->GetNumHarpedStrands(), 
                                                                             HsoEnd, 
                                                                             pArtifact->GetHarpStrandOffsetEnd());
         (*pTable)(row,1) << length.SetValue(offset);

         offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr, girderData.Nstrands[pgsTypes::Harped], 
                                                             HsoEnd, abs_offset_end);

         (*pTable)(row,2) << length.SetValue(offset);

         row++;

         HarpedStrandOffsetType HsoHp = girderData.HsoHpMeasurement;
         switch( HsoHp )
         {
         case hsoCGFROMTOP:
            (*pTable)(row,0) << "Distance from top of girder to" << rptNewLine << "CG of harped strand group at harping point";
            break;

         case hsoCGFROMBOTTOM:
            (*pTable)(row,0) << "Distance from bottom of girder to" << rptNewLine << "CG of harped strand group at harping point";
            break;

         case hsoTOP2TOP:
            (*pTable)(row,0) << "Distance from top of girder to" << rptNewLine << "top of harped strand group at harping point";
            break;

         case hsoTOP2BOTTOM:
            (*pTable)(row,0) << "Distance from bottom of girder to" << rptNewLine << "top of harped strand group at harping point";
            break;

         case hsoLEGACY:
            // convert legacy to display BOTTOM 2 BOTTOM
            HsoHp = hsoBOTTOM2BOTTOM;

         case hsoBOTTOM2BOTTOM:
            (*pTable)(row,0) << "Distance from bottom of girder to" << rptNewLine << "bottom of harped strand group at harping point";
            break;


         case hsoECCENTRICITY:
            (*pTable)(row,0) << "Eccentricity of harped strand" << rptNewLine << "group at harping point";
            break;

         default:
            ATLASSERT(false); // should never get here
         }


         offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr,
                                                                     pArtifact->GetNumHarpedStrands(), 
                                                                     HsoHp, 
                                                                     pArtifact->GetHarpStrandOffsetHp());

         (*pTable)(row,1) << length.SetValue(offset);

         offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr,
                                                                     girderData.Nstrands[pgsTypes::Harped], 
                                                                     HsoHp, abs_offset_hp);
         (*pTable)(row,2) << length.SetValue(offset);

         row++;
      }

      (*pTable)(row,0) << RPT_FCI;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetReleaseStrength());
      (*pTable)(row,2) << stress.SetValue( girderData.Material.Fci );
      row++;

      (*pTable)(row,0) << RPT_FC;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetConcreteStrength());
      (*pTable)(row,2) << stress.SetValue(girderData.Material.Fc);
      row++;

      if ( options.doDesignSlabOffset && (pBridge->GetDeckType()!=pgsTypes::sdtNone) )
      {
         const CGirderTypes* pGirderTypes = pIBridgeDesc->GetBridgeDescription()->GetSpan(span)->GetGirderTypes();

         // the computed slab offset will be applied according to the current slab offset mode
         if ( pIBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
         {
            // slab offset is for the entire bridge... the start value contains this parameter
            (*pTable)(row,0) << "Slab Offset (\"A\" Dimension)";
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pGirderTypes->GetSlabOffset(gdr,pgsTypes::metStart) );
            row++;
         }
         else
         {
            (*pTable)(row,0) << "Slab Offset at Start (\"A\" Dimension)";
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pGirderTypes->GetSlabOffset(gdr,pgsTypes::metStart) );
            row++;

            (*pTable)(row,0) << "Slab Offset at End (\"A\" Dimension)";
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metEnd) );
            (*pTable)(row,2) << length.SetValue( pGirderTypes->GetSlabOffset(gdr,pgsTypes::metEnd) );
            row++;
         }
      }

      if (options.doDesignLifting)
      {
         (*pTable)(row,0) << "Lifting Point Location (Left)";
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeftLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pGirderLifting->GetLeftLiftingLoopLocation(span,gdr) );
         row++;

         (*pTable)(row,0) << "Lifting Point Location (Right)";
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetRightLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pGirderLifting->GetRightLiftingLoopLocation(span,gdr) );
         row++;
      }

      if (options.doDesignHauling)
      {
         (*pTable)(row,0) << "Truck Support Location (Leading)";
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeadingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pGirderHauling->GetLeadingOverhang(span,gdr) );
         row++;

         (*pTable)(row,0) << "Truck Support Location (Trailing)";
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetTrailingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pGirderHauling->GetTrailingOverhang(span,gdr) );
         row++;
      }
   }
   else
   {
      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << "Flexure Design Not Requested"<<rptNewLine;
   }


   if (pArtifact->GetDoDesignShear())
   {

      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << "Shear Design:";

      pParagraph = new rptParagraph();

      *pChapter << pParagraph;
      *pParagraph << Bold("Proposed Design:") << rptNewLine;

     // stirrup design results
      ZoneIndexType nz = pArtifact->GetNumberOfStirrupZonesDesigned();

      if (nz>0)
      {

         rptRcTable* pTables = pgsReportStyleHolder::CreateTableNoHeading(4,"");
         *pParagraph << pTables;

         INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDispUnit->GetComponentDimUnit(), true );
         INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDispUnit->GetSpanLengthUnit(), true );

         (*pTables)(0,0) << "Zone #";
         (*pTables)(0,1) << COLHDR("Zone End", rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );
         (*pTables)(0,2) << "Bar Size";
         (*pTables)(0,3) << COLHDR("Spacing", rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );

         lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
         CHECK(pool!=0);

         Float64 zone_end = 0.0;
         for (Uint16 i=0; i<nz; i++)
         {
            Uint16 row = i+1;
            CShearZoneData szdata = pArtifact->GetShearZoneData(i);
            zone_end += szdata.ZoneLength;
            (*pTables)(row,0) << szdata.ZoneNum;

            if (i<nz-1)
               (*pTables)(row,1) << location.SetValue(zone_end);
            else
               (*pTables)(row,1) << "Mid-Girder";

            const matRebar* prb = pool->GetRebar(szdata.VertBarSize);
            if (prb!=0)
            {
               (*pTables)(row,2) << prb->GetName();
               (*pTables)(row,3) << length.SetValue(szdata.BarSpacing);
            }
            else
            {
               (*pTables)(row,2) << "none";
               (*pTables)(row,3) << "--";
            }
         }

         // confinement
         BarSizeType cbs = pArtifact->GetConfinementBarSize();
         const matRebar* pcrb = pool->GetRebar(cbs);
         *pParagraph<<"Confinement rebar size is "<<pcrb->GetName()<<rptNewLine;
         *pParagraph<<"Confinement rebar ends in zone "<<(pArtifact->GetLastConfinementZone()+1)<<rptNewLine;
      }
      else
      {
         *pParagraph << "No Zones Designed"<<rptNewLine;
      }

      // Current configuration
      *pParagraph << Bold("Current Values:") << rptNewLine;

      GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeometry);
      Uint32 ncz = pStirrupGeometry->GetNumZones(span,gdr);

      if (0 < ncz)
      {
         rptRcTable* pTables = pgsReportStyleHolder::CreateTableNoHeading(4,"");
         *pParagraph << pTables;

         INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDispUnit->GetComponentDimUnit(), true );
         INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDispUnit->GetSpanLengthUnit(), true );

         (*pTables)(0,0) << "Zone #";
         (*pTables)(0,1) << COLHDR("Zone End", rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );
         (*pTables)(0,2) << "Bar Size";
         (*pTables)(0,3) << COLHDR("Spacing", rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );

         lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
         CHECK(pool!=0);

         Uint32 nhz = (ncz+1)/2;
         Float64 zone_end = 0.0;
         for (Uint32 i=0; i<nhz; i++)
         {
            RowIndexType row = i+1;

            (*pTables)(row,0) << pStirrupGeometry->GetZoneId(span,gdr,i);

            if (i<nhz-1)
               (*pTables)(row,1) << location.SetValue(pStirrupGeometry->GetZoneEnd(span,gdr,i));
            else
               (*pTables)(row,1) << "Mid-Girder";

            BarSizeType barSize = pStirrupGeometry->GetVertStirrupBarSize(span,gdr,i);
            if ( barSize != 0 )
            {
               (*pTables)(row,2) << "#" << barSize;
               (*pTables)(row,3) << length.SetValue(pStirrupGeometry->GetS(span,gdr,i));
            }
            else
            {
               (*pTables)(row,2) << "none";
               (*pTables)(row,3) << "--";
            }
         }

         // confinement
         Uint32 lz   = pStirrupGeometry->GetNumConfinementZones(span,gdr);
         BarSizeType size = pStirrupGeometry->GetConfinementBarSize(span,gdr);
         if (lz!=0 && size!=0)
         {
            *pParagraph << "Confinement rebar size is #" << size << rptNewLine;
            *pParagraph << "Confinement rebar ends in zone " << lz << rptNewLine;
         }
         else
         {
            *pParagraph<<"Bottom flange confinement steel not present"<<rptNewLine;
         }
      }
      else
      {
         *pParagraph << "No Shear Zones in current girder"<<rptNewLine;
      }
   }

   // End up with some notes about Flexural Design
   if ( pArtifact->GetDoDesignFlexure()!=dtNoDesign && pArtifact->GetOutcome()==pgsDesignArtifact::Success)
   {

      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << "Design Notes:" << rptNewLine;

      pParagraph = new rptParagraph();

      GET_IFACE2(pBroker,ILimits,pLimits);
      double max_girder_fci = pLimits->GetMaxGirderFci();
      double max_girder_fc = pLimits->GetMaxGirderFc();
      if (pArtifact->GetReleaseStrength() > max_girder_fci)
      {
         *pParagraph <<color(Red)<< "Warning: The designed girder release strength exceeds the normal value of "<<stress.SetValue(max_girder_fci)<<color(Black)<< rptNewLine;
      }

      if (pArtifact->GetConcreteStrength() > max_girder_fc)
      {
         *pParagraph <<color(Red)<< "Warning: The designed girder final concrete strength exceeds the normal value of "<<stress.SetValue(max_girder_fc)<<color(Black)<< rptNewLine;
      }

      // Negative camber is not technically a spec check, but a warning
      GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
      std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(pgsTypes::BridgeSite3,span,gdr,POI_MIDSPAN);
      CHECK(vPoi.size()==1);
      pgsPointOfInterest poi = *vPoi.begin();

      GDRCONFIG config = pArtifact->GetGirderConfiguration();

      GET_IFACE2(pBroker,ICamber,pCamber);
      double excess_camber = pCamber->GetExcessCamber(poi,config,CREEP_MAXTIME);
      if ( excess_camber < 0 )
      {
         *pParagraph<<color(Red)<< "Warning:  Excess camber is negative, indicating a potential sag in the beam."<<color(Black)<< rptNewLine;
      }

      *pParagraph << "Concrete release strength was controlled by "<<pArtifact->GetReleaseDesignState().AsString() << rptNewLine;
      *pParagraph << "Concrete final strength was controlled by "<<pArtifact->GetFinalDesignState().AsString() << rptNewLine;
      *pParagraph << rptNewLine;

      if ( options.doDesignSlabOffset && (pBridge->GetDeckType()!=pgsTypes::sdtNone) )
      {
         if ( pIBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
         {
            *pParagraph << "Slab Offset will be applied to the bridge" << rptNewLine;
         }
         else
         {
            *pParagraph << "Slab Offset will be applied to this girder" << rptNewLine;
         }
      }

      *pChapter << pParagraph;
   }
}

void successful_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit,const pgsDesignArtifact* pArtifact)
{
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pParagraph;

   *pParagraph << color(Green)
               << "The design for Span " << LABEL_SPAN(pArtifact->GetSpan())
               << " Girder " << LABEL_GIRDER(pArtifact->GetGirder())
               << " was successful." 
               << color(Black)
               << rptNewLine;

   write_artifact_data(pBroker,span,gdr,pChapter,pDispUnit,pArtifact);
}

void failed_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit,const pgsDesignArtifact* pArtifact)
{
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pParagraph;

   *pParagraph << color(Red)
               << "The design attempt for Span " << LABEL_SPAN(pArtifact->GetSpan())
               << " Girder " << LABEL_GIRDER(pArtifact->GetGirder())
               << " failed." 
               << color(Black)
               << rptNewLine;

   pParagraph = new rptParagraph( pgsReportStyleHolder::GetSubheadingStyle() );
   *pChapter << pParagraph;
   switch( pArtifact->GetOutcome() )
   {
      case pgsDesignArtifact::Success:
         CHECK(false); // Should never get here
         break;

      case pgsDesignArtifact::NoDesignRequested:
         *pParagraph << "No Design was requested." << rptNewLine;
         break;

      case pgsDesignArtifact::TooManyStrandsReqd:
         *pParagraph << "Too many strands are required to satisfy the stress criteria." << rptNewLine;
         break;

      case pgsDesignArtifact::OverReinforced:
         *pParagraph << "The section is over reinforced and the number of strands cannot be reduced." << rptNewLine;
         break;

      case pgsDesignArtifact::UnderReinforced:
         *pParagraph << "The section is under reinforced and the number of strands cannot be increased." << rptNewLine;
         *pParagraph << "Increasing the compressive strength of the deck will improve section capacity." << rptNewLine;
         break;

      case pgsDesignArtifact::UltimateMomentCapacity:
         *pParagraph << "Too many strands are required to satisfy ultimate moment capacity criteria." << rptNewLine;
         break;

      case pgsDesignArtifact::MaxIterExceeded:
         *pParagraph << "After several iterations, a successful design could not be found." << rptNewLine;
         break;

      case pgsDesignArtifact::ReleaseStrength:
         *pParagraph << "An acceptable concrete release strength could not be found." << rptNewLine;
         break;

      case pgsDesignArtifact::ExceededMaxHoldDownForce:
         *pParagraph << "Design is such that maximum allowable hold down force in casting yard is exceeded."<<rptNewLine;
         break;

      case pgsDesignArtifact::StrandSlopeOutOfRange:
         *pParagraph << "Design is such that maximum strand slope is exceeded."<<rptNewLine;
         break;

      case pgsDesignArtifact::ShearExceedsMaxConcreteStrength:
         *pParagraph << "Section is too small to carry ultimate shear. Crushing capacity was exceeded" << rptNewLine;
         break;

      case pgsDesignArtifact::TooManyStirrupsReqd:
         *pParagraph << "Could not design stirrups - Minimum spacing requirements were violated" << rptNewLine;
         break;

      case pgsDesignArtifact::GirderLiftingStability:
         *pParagraph << "Could not satisfy stability requirements for lifting" << rptNewLine;
         break;

      case pgsDesignArtifact::GirderLiftingConcreteStrength:
         *pParagraph << "Could not find a concrete strength to satisfy stress limits for lifting" << rptNewLine;
         break;

      case pgsDesignArtifact::GirderShippingStability:
         *pParagraph << "Could not satisfy stability requirements for shipping" << rptNewLine;
         break;

      case pgsDesignArtifact::GirderShippingConfiguration:
         *pParagraph << "Could not satisfy trucking configuration requirements for shipping" << rptNewLine;
         break;

      case pgsDesignArtifact::GirderShippingConcreteStrength:
         *pParagraph << "Could not find a concrete strength to satisfy stress limits for shipping" << rptNewLine;
         break;

      case pgsDesignArtifact::StressExceedsConcreteStrength:
         *pParagraph << "Could not find a concrete strength to satisfy stress limits" << rptNewLine;
         break;
      
      case pgsDesignArtifact::DebondDesignFailed:
         *pParagraph << "Unable to find an adequate debond design" << rptNewLine;
         break;

      default:
         CHECK(false); // Should never get here
   }

   pParagraph = new rptParagraph();
   *pParagraph << Bold("Results from last trial:") << rptNewLine;
   *pChapter << pParagraph;
   write_artifact_data(pBroker,span,gdr,pChapter,pDispUnit,pArtifact);
}

