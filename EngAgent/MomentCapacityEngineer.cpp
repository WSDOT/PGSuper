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

#include "StdAfx.h"
#include "MomentCapacityEngineer.h"
#include "..\PGSuperException.h"

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\MomentCapacity.h>
#include <IFace\PrestressForce.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <PgsExt\statusitem.h>
#include <PgsExt\GirderLabel.h>

#include <Lrfd\ConcreteUtil.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DIAG_DEFINE_GROUP(MomCap,DIAG_GROUP_DISABLE,0);

static const Float64 ANGLE_TOL=1.0e-6;
static const Float64 D_TOL=1.0e-10;

void AddShape2Section(IGeneralSection *pSection, IShape *pShape, IStressStrain *pfgMaterial, IStressStrain *pbgMaterial, Float64 ei)
{
#if defined USE_ORIGINAL_SHAPE
   // Just add shape as is
   pSection->AddShape(pShape, pfgMaterial, pbgMaterial, ei);
#else
   // Convert shape to a fast polygon
   // get points from shape and create a faster poly
   CComPtr<IPoint2dCollection> points;
   pShape->get_PolyPoints(&points);

   CComPtr<IFasterPolyShape> poly;
   HRESULT hr = poly.CoCreateInstance(CLSID_FasterPolyShape);

   poly->AddPoints(points);

   CComQIPtr<IShape> shape(poly);

   pSection->AddShape(shape, pfgMaterial, pbgMaterial, ei);
#endif
}

/****************************************************************************
CLASS
   pgsMomentCapacityEngineer
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsMomentCapacityEngineer::pgsMomentCapacityEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;
   CREATE_LOGFILE("MomentCapacity");

   // create solvers
   HRESULT hr = m_MomentCapacitySolver.CoCreateInstance(CLSID_MomentCapacitySolver);

   if ( FAILED(hr) )
   {
      THROW_SHUTDOWN(_T("Installation Problem - Unable to create Moment Capacity Solver"),XREASON_COMCREATE_ERROR,true);
   }


   hr = m_CrackedSectionSolver.CoCreateInstance(CLSID_CrackedSectionSolver);

   if ( FAILED(hr) )
   {
      THROW_SHUTDOWN(_T("Installation Problem - Unable to create Cracked Section Solver"),XREASON_COMCREATE_ERROR,true);
   }
}

pgsMomentCapacityEngineer::pgsMomentCapacityEngineer(const pgsMomentCapacityEngineer& rOther)
{
   MakeCopy(rOther);
}

pgsMomentCapacityEngineer::~pgsMomentCapacityEngineer()
{
   CLOSE_LOGFILE;
}

//======================== OPERATORS  =======================================
pgsMomentCapacityEngineer& pgsMomentCapacityEngineer::operator= (const pgsMomentCapacityEngineer& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsMomentCapacityEngineer::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsMomentCapacityEngineer::SetStatusGroupID(StatusGroupIDType statusGroupID)
{
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidUnknown = pStatusCenter->RegisterCallback( new pgsUnknownErrorStatusCallback() );
}

void pgsMomentCapacityEngineer::ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);

   Float64 Eps = pStrand->GetE();
   Float64 fpe_ps = 0.0; // effective prestress after all losses
   Float64 eps_initial = 0.0; // initial strain in the prestressing strands (strain at effect prestress)
   if ( bPositiveMoment )
   {
      // only for positive moment... strands are ignored for negative moment analysis
      GET_IFACE(IPretensionForce, pPrestressForce);
      fpe_ps = pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End);
      eps_initial = fpe_ps/Eps;
   }

   std::vector<Float64> fpe_pt;
   std::vector<Float64> ept_initial;
   const matPsStrand* pTendon = pMaterial->GetTendonMaterial(segmentKey);
   Float64 Ept = pTendon->GetE();
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE_NOCHECK(IPosttensionForce,pPTForce); // only used if 0 < nDucts
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      Float64 fpe = pPTForce->GetTendonStress(poi,intervalIdx,pgsTypes::End,ductIdx);
      Float64 e = fpe/Ept;
      fpe_pt.push_back(fpe);
      ept_initial.push_back(e);
   }

   pgsBondTool bondTool(m_pBroker,poi);

   ComputeMomentCapacity(intervalIdx,poi,NULL,fpe_ps,eps_initial,fpe_pt,ept_initial,bondTool,bPositiveMoment,pmcd);
}

void pgsMomentCapacityEngineer::ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);

   Float64 Eps = pStrand->GetE();
   Float64 fpe_ps = 0.0; // effective prestress after all losses
   Float64 eps_initial = 0.0; // initial strain in the prestressing strands (strain at effect prestress)
   if ( bPositiveMoment )
   {
      // only for positive moment... strands are ignored for negative moment analysis
      GET_IFACE(IPretensionForce, pPrestressForce);
      if ( pConfig )
      {
         fpe_ps = pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End,*pConfig);
      }
      else
      {
         fpe_ps = pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End);
      }

      eps_initial = fpe_ps/Eps;
   }

   std::vector<Float64> fpe_pt;
   std::vector<Float64> ept_initial;
   const matPsStrand* pTendon = pMaterial->GetTendonMaterial(segmentKey);
   Float64 Ept = pTendon->GetE();
   GET_IFACE_NOCHECK(IPosttensionForce,pPTForce); // not used if there aren't any tendons
   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      Float64 fpe = pPTForce->GetTendonStress(poi,intervalIdx,pgsTypes::End,ductIdx);
      Float64 e = fpe/Ept;
      fpe_pt.push_back(fpe);
      ept_initial.push_back(e);
   }

   if ( pConfig )
   {
      pgsBondTool bondTool(m_pBroker,poi,*pConfig);
      ComputeMomentCapacity(intervalIdx,poi,pConfig,fpe_ps,eps_initial,fpe_pt,ept_initial,bondTool,bPositiveMoment,pmcd);
   }
   else
   {
      pgsBondTool bondTool(m_pBroker,poi);
      ComputeMomentCapacity(intervalIdx,poi,NULL,fpe_ps,eps_initial,fpe_pt,ept_initial,bondTool,bPositiveMoment,pmcd);
   }
}

void pgsMomentCapacityEngineer::ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64 fpe_ps,Float64 eps_initial,const std::vector<Float64>& fpe_pt,const std::vector<Float64>& ept_initial,pgsBondTool& bondTool,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
   GET_IFACE(ITendonGeometry,pTendonGeom);

   GET_IFACE_NOCHECK(IStrandGeometry,pStrandGeom); // only used for positive moment

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   StrandIndexType Ns = 0;
   StrandIndexType Nh = 0;
   if ( bPositiveMoment )
   {
      // Strands only modeled for positive moment calculations
      if ( pConfig )
      {
         Ns = pConfig->PrestressConfig.GetStrandCount(pgsTypes::Straight);
         Nh = pConfig->PrestressConfig.GetStrandCount(pgsTypes::Harped);
      }
      else
      {
         Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
         Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
      }
   }

   DuctIndexType Npt = 0;
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      Npt += pTendonGeom->GetTendonStrandCount(segmentKey,ductIdx);
   }

   bool bIsSplicedGirder = (0 < nDucts ? true : false);

   // create a problem to solve
   CComPtr<IGeneralSection> section;
   CComPtr<IPoint2d> pntCompression; // location of the extreme compression faoce
   CComPtr<ISize2d> szOffset; // distance to offset coordinates from bridge model to capacity model
   std::map<StrandIndexType,Float64> bond_factors[2];
   Float64 dt; // depth from top of section to extreme layer of tensile reinforcement
   Float64 H; // overall height of section
   Float64 Haunch; // haunch build up that is modeled
   BuildCapacityProblem(intervalIdx,poi,pConfig,eps_initial,ept_initial,bondTool,bPositiveMoment,&section,&pntCompression,&szOffset,&dt,&H,&Haunch,bond_factors);

#if defined _DEBUG_SECTION_DUMP
   DumpSection(poi,section,bond_factors[0],bond_factors[1],bPositiveMoment);
#endif // _DEBUG_SECTION_DUMP

   m_MomentCapacitySolver->putref_Section(section);
   m_MomentCapacitySolver->put_Slices(10);
   m_MomentCapacitySolver->put_SliceGrowthFactor(3);
   m_MomentCapacitySolver->put_MaxIterations(50);

   // Set the convergence tolerance to 0.1N. This is more than accurate enough for the
   // output display. Output accurace for SI = 0.01kN = 10N, for US = 0.01kip = 45N
   m_MomentCapacitySolver->put_AxialTolerance(0.1);

   // determine neutral axis angle
   // compression is on the left side of the neutral axis
   Float64 na_angle = (bPositiveMoment ? 0.00 : M_PI);

   // compressive strain limit
   Float64 ec = -0.003; 

   CComPtr<IMomentCapacitySolution> solution;

#if defined _DEBUG
   CTime startTime = CTime::GetCurrentTime();
#endif // _DEBUG

   HRESULT hr = m_MomentCapacitySolver->Solve(0.00,na_angle,ec,smFixedCompressiveStrain,&solution);

   if ( hr == RC_E_MATERIALFAILURE )
   {
      WATCHX(MomCap,0,_T("Exceeded material strain limit"));
      hr = S_OK;
   }
   
   // It is ok if this assert fires... All it means is that the solver didn't find a solution
   // on its first try. The purpose of this assert is to help gauge how often this happens.
   // Second and third attempts are made below
#if defined _DEBUG
   ATLASSERT(SUCCEEDED(hr));
   if ( hr == RC_E_INITCONCRETE )       ATLASSERT(SUCCEEDED(hr));
   if ( hr == RC_E_SOLUTIONNOTFOUND )   ATLASSERT(SUCCEEDED(hr));
   if ( hr == RC_E_BEAMNOTSYMMETRIC )   ATLASSERT(SUCCEEDED(hr));
   if ( hr == RC_E_MATERIALFAILURE )    ATLASSERT(SUCCEEDED(hr));
#endif // _DEBUG

   if (FAILED(hr))
   {
      // Try again with more slices
      m_MomentCapacitySolver->put_Slices(20);
      m_MomentCapacitySolver->put_SliceGrowthFactor(2);
      m_MomentCapacitySolver->put_AxialTolerance(1.0);
      hr = m_MomentCapacitySolver->Solve(0.00,na_angle,ec,smFixedCompressiveStrain,&solution);

      if ( hr == RC_E_MATERIALFAILURE )
      {
         WATCHX(MomCap,0,_T("Exceeded material strain limit"));
         hr = S_OK;
      }

      if ( FAILED(hr) )
      {
         // Try again with more slices
         m_MomentCapacitySolver->put_Slices(50);
         m_MomentCapacitySolver->put_SliceGrowthFactor(2);
         m_MomentCapacitySolver->put_AxialTolerance(10.0);
         hr = m_MomentCapacitySolver->Solve(0.00,na_angle,ec,smFixedCompressiveStrain,&solution);

         if ( hr == RC_E_MATERIALFAILURE )
         {
            WATCHX(MomCap,0,_T("Exceeded material strain limit"));
            hr = S_OK;
         }

         if ( FAILED(hr) )
         {
            GET_IFACE(IEAFStatusCenter,pStatusCenter);
            GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
            GET_IFACE(IDocumentType,pDocType);

            const unitmgtLengthData& unit = pDisplayUnits->GetSpanLengthUnit();
            CString msg;
            if ( pDocType->IsPGSuperDocument() )
            {
               msg.Format(_T("An unknown error occured while computing %s moment capacity for Span %d Girder %s at %f %s from the left end of the girder (%d)"),
                           (bPositiveMoment ? _T("positive") : _T("negative")),
                           LABEL_SPAN(segmentKey.groupIndex),
                           LABEL_GIRDER(segmentKey.girderIndex),
                           ::ConvertFromSysUnits(poi.GetDistFromStart(),unit.UnitOfMeasure),
                           unit.UnitOfMeasure.UnitTag().c_str(),
                           hr);
            }
            else
            {
               msg.Format(_T("An unknown error occured while computing %s moment capacity for Group %d Girder %s Segment %d at %f %s from the left end of the segment (%d)"),
                           (bPositiveMoment ? _T("positive") : _T("negative")),
                           LABEL_GROUP(segmentKey.groupIndex),
                           LABEL_GIRDER(segmentKey.girderIndex),
                           LABEL_SEGMENT(segmentKey.segmentIndex),
                           ::ConvertFromSysUnits(poi.GetDistFromStart(),unit.UnitOfMeasure),
                           unit.UnitOfMeasure.UnitTag().c_str(),
                           hr);
            }
            pgsUnknownErrorStatusItem* pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg);
            pStatusCenter->Add(pStatusItem);
            THROW_UNWIND(msg,-1);
         }
      }
   }


#if defined _DEBUG
   CTime endTime = CTime::GetCurrentTime();
   CTimeSpan duration = endTime - startTime;
   WATCHX(MomCap,0,_T("Duration = ") << duration.GetTotalSeconds() << _T(" seconds"));
#endif // _DEBUG

   pmcd->CapacitySolution = solution;

   Float64 Fz,Mx,My;
   CComPtr<IPlane3d> strains;
   solution->get_Fz(&Fz);
   solution->get_Mx(&Mx);
   solution->get_My(&My);
   solution->get_StrainPlane(&strains);

   ATLASSERT( IsZero(Fz,0.1) );
   ATLASSERT( Mx != 0.0 ? IsZero(My/Mx,0.05) : true );  // when there is an odd number of harped strands, the strands aren't always symmetrical
                                     // this will cause a small amount of off axis bending.
                                     // Only assert if the ratio of My/Mx is larger that the tolerance for zero

   Float64 Mn = -Mx;

   pmcd->Mn  = Mn;

   if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
   {
      GET_IFACE(ILongRebarGeometry, pLongRebarGeom);

      if ( pConfig )
      {
         pmcd->PPR = (bPositiveMoment ? pLongRebarGeom->GetPPRBottomHalf(poi,*pConfig) : 0.0);
      }
      else
      {
         pmcd->PPR = (bPositiveMoment ? pLongRebarGeom->GetPPRBottomHalf(poi) : 0.0);
      }
   }
   else
   {
      // PPR was removed from LRFD in 6th Edition, 2012. Use 1.0 for positive moments so
      // Phi = PhiRC + (PhiPS-PhiRC)*PPR = PhiRC + PhiPS - PhiRC = PhiPS and use 0.0 for negative moment so
      // Phi = PhiRC + (PhiPS-PhiRC)*0.0 = PhiRC
      //
      // See computation of Phi about 5 lines down
      if ( bIsSplicedGirder )
      {
         pmcd->PPR = 1.0;
      }
      else
      {
         pmcd->PPR = (bPositiveMoment ? 1.0 : 0.0);
      }
   }

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   matRebar::Type rebarType;
   matRebar::Grade deckRebarGrade;
   pMaterial->GetDeckRebarMaterial(&rebarType,&deckRebarGrade);

   GET_IFACE(IResistanceFactors,pResistanceFactors);
   GET_IFACE(IPointOfInterest,pPoi);
   Float64 PhiRC,PhiPS,PhiSP,PhiC;
   CClosureKey closureKey;
   if ( pPoi->IsInClosureJoint(poi,&closureKey) )
   {
      pmcd->Phi = pResistanceFactors->GetClosureJointFlexureResistanceFactor(concType);
   }
   else
   {
      pResistanceFactors->GetFlexureResistanceFactors(concType,&PhiPS,&PhiRC,&PhiSP,&PhiC);
      if ( bIsSplicedGirder )
      {
         pmcd->Phi = PhiSP;
      }
      else
      {
         pmcd->Phi = PhiRC + (PhiPS-PhiRC)*pmcd->PPR; // generalized form of 5.5.4.2.1-3
                                                      // Removed in AASHTO LRFD 6th Edition 2012, however
                                                      // PPR has been computed above to take this into account
      }
   }

   Float64 C,T;
   solution->get_CompressionResultant(&C);
   solution->get_TensionResultant(&T);
   ATLASSERT(IsZero(C+T,0.5)); // equilibrium within 0.5 Newtons
   
   pmcd->C = C;
   pmcd->T = T;

   CComPtr<IPoint2d> cgC, cgT;
   solution->get_CompressionResultantLocation(&cgC);
   solution->get_TensionResultantLocation(&cgT);

   Float64 fps_avg = 0;

   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   const matPsStrand* pTendon = pMaterial->GetTendonMaterial(segmentKey);

   if ( IsZero(Mn) )
   {
      // dimensions have no meaning if no moment capacity
      pmcd->c          = 0.0;
      pmcd->dc         = 0.0;
      pmcd->MomentArm  = 0.0;
      pmcd->de         = 0.0;
      pmcd->de_shear   = 0.0;
      
      fps_avg          = 0.0;
   }
   else
   {
      GET_IFACE(IBridge, pBridge);

      pmcd->MomentArm = fabs(Mn/T);

      Float64 tSlab = pBridge->GetStructuralSlabDepth(poi);

      Float64 x1,y1, x2,y2;
      pntCompression->get_X(&x1);
      pntCompression->get_Y(&y1);

      cgC->get_X(&x2);
      cgC->get_Y(&y2);

      pmcd->dc = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
      pmcd->de = pmcd->dc + pmcd->MomentArm;

      CComPtr<IPlane3d> strainPlane;
      solution->get_StrainPlane(&strainPlane);
      Float64 x,y,z;
      x = 0;
      z = 0;
      strainPlane->GetY(x,z,&y);

      pmcd->c = sqrt((x1-x)*(x1-x) + (y1-y)*(y1-y));

      Float64 dx,dy;
      szOffset->get_Dx(&dx);
      szOffset->get_Dy(&dy);

      // calculate average resultant stress in strands/tendonds at ultimate

      if ( 0 < Ns+Nh+Npt )
      {
         Float64 aps = pStrand->GetNominalArea();

         Float64 Aps[2] = {0,0}; // total area of straight/harped
         Float64 Apt = 0; // total area of tendon

         CComPtr<IStressStrain> ssStrand;
         CreateStrandMaterial(segmentKey,&ssStrand);

         CComPtr<IStressStrain> ssTendon;
         CreateTendonMaterial(segmentKey,&ssTendon);

         if ( bPositiveMoment )
         {
            for ( int i = 0; i < 2; i++ ) // straight and harped strands
            {
               pgsTypes::StrandType strandType = (i == 0 ? pgsTypes::Straight : pgsTypes::Harped);
               CComPtr<IPoint2dCollection> points;
               if ( pConfig )
               {
                  pStrandGeom->GetStrandPositionsEx(poi, pConfig->PrestressConfig, strandType, &points);
               }
               else
               {
                  pStrandGeom->GetStrandPositions(poi, strandType, &points);
               }

               long strandPos = 0;
               CComPtr<IEnumPoint2d> enum_points;
               points->get__Enum(&enum_points);
               CComPtr<IPoint2d> point;
               while ( enum_points->Next(1,&point,NULL) != S_FALSE )
               {
                  Float64 bond_factor = bond_factors[i][strandPos++];

                  point->get_X(&x);
                  point->get_Y(&y);

                  strainPlane->GetZ(x-dx,y-dy,&z);
                  Float64 stress;
                  ssStrand->ComputeStress(z+eps_initial,&stress);

                  stress *= bond_factor;

                  Aps[i] += aps;
                  fps_avg += aps*stress;

                  point.Release();
               }
            }
         }

         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            CComPtr<IPoint2d> point;
            pTendonGeom->GetDuctPoint(poi,ductIdx,&point);

            Float64 apt = pTendonGeom->GetTendonArea(segmentKey,intervalIdx,ductIdx);
            Apt += apt;

            point->get_X(&x);
            point->get_Y(&y);

            strainPlane->GetZ(x-dx,y-dy,&z);
            Float64 stress;
            ssTendon->ComputeStress(z+ept_initial[ductIdx],&stress);

            //stress *= bond_factor;

            fps_avg += apt*stress;

            point.Release();
         }

         fps_avg /= (Aps[pgsTypes::Straight]+Aps[pgsTypes::Harped]+Apt);
      }

      // determine de (see PCI BDM 8.4.1.2).
      // de = resultant of tension force due to reinforcement on the tension half of the beam
      Float64 t = 0; // summ of the tension forces 
      Float64 tde = 0; // summ of the product of the location of tension force and the tension force
      // de = tde/t
      CComPtr<IGeneralSectionSolution> general_solution;
      solution->get_GeneralSectionSolution(&general_solution);
      CollectionIndexType nSlices;
      general_solution->get_SliceCount(&nSlices);

      for ( CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++ )
      {
         CComPtr<IGeneralSectionSlice> slice;
         general_solution->get_Slice(sliceIdx,&slice);
      
         Float64 slice_area;
         slice->get_Area(&slice_area);

         CComPtr<IPoint2d> pntCG;
         slice->get_CG(&pntCG);

         Float64 cgY;
         pntCG->get_Y(&cgY);

         Float64 fgStress,bgStress,netStress;
         slice->get_ForegroundStress(&fgStress);
         slice->get_BackgroundStress(&bgStress);

         netStress = fgStress - bgStress;

         Float64 F = slice_area * netStress;
         Float64 d = cgY + dy - (Haunch+tSlab); // adding dy moves cgY to the Girder Section Coordinate (0,0 at top of bare girder)
                                                // subtracting (Haunch+tSlab) makes d measured from the top of composite girder section
         if ( 0 < F &&  // tension
             ( 
               ( bPositiveMoment && d < -H/2) || // on bottom half
               (!bPositiveMoment && -H/2 < d)    // on top half
              )
            ) 
         {
            t += F;
            tde += F*d;
         }
      }

      pmcd->de_shear = (IsZero(t) ? 0 : -tde/t);

      if ( !bPositiveMoment )
      {
         // de_shear is measured from the top of the girder
         // for negative moment, we want it to be measured from the bottom
         pmcd->de_shear = H - pmcd->de_shear;
      }
   }


   WATCHX(MomCap,0, _T("X = ") << ::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft") << _T("   Mn = ") << ::ConvertFromSysUnits(Mn,unitMeasure::KipFeet) << _T(" kip-ft") << _T(" My/Mx = ") << My/Mn << _T(" fps_avg = ") << ::ConvertFromSysUnits(fps_avg,unitMeasure::KSI) << _T(" KSI"));

   pmcd->fps = fps_avg;
   pmcd->dt  = dt;
   pmcd->bOverReinforced = false;

   GET_IFACE(ISpecification, pSpec);
   pmcd->Method = pSpec->GetMomentCapacityMethod();

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pmcd->Method == LRFD_METHOD && pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2006Interims)
   {
      pmcd->bOverReinforced = (pmcd->c / pmcd->de > 0.42) ? true : false;
      if ( pmcd->bOverReinforced )
      {
         GET_IFACE(IMaterials,pMaterial);
         Float64 de = pmcd->de;
         Float64 c  = pmcd->c;

         Float64 hf;
         Float64 b;
         Float64 bw;
         Float64 fc;
         Float64 Beta1;
         if ( bPositiveMoment )
         {
            GET_IFACE(ISectionProperties,pProps);
            GET_IFACE(IBridge, pBridge);
            GET_IFACE(IGirder, pGdr);

            bw = pGdr->GetWebWidth(poi);

            if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
            {
               b     = pGdr->GetTopFlangeWidth(poi);
               hf    = pGdr->GetMinTopFlangeThickness(poi);
               fc    = pMaterial->GetSegmentFc(segmentKey,intervalIdx);
               Beta1 = lrfdConcreteUtil::Beta1(fc);
            }
            else
            {
               b     = pProps->GetEffectiveFlangeWidth(poi);
               hf    = pBridge->GetStructuralSlabDepth(poi);
               fc    = pMaterial->GetDeckFc(segmentKey,intervalIdx);
               Beta1 = lrfdConcreteUtil::Beta1(fc);
            }
         }
         else
         {
            GET_IFACE(IGirder, pGdr);
            hf = pGdr->GetMinBottomFlangeThickness(poi);
            b  = pGdr->GetBottomWidth(poi);
            bw = pGdr->GetWebWidth(poi);
            fc = pMaterial->GetSegmentFc(segmentKey,intervalIdx);
            Beta1 = lrfdConcreteUtil::Beta1(fc);
         }

         pmcd->FcSlab = fc;
         pmcd->b = b;
         pmcd->bw = bw;
         pmcd->hf = hf;
         pmcd->Beta1Slab = Beta1;

         if ( c <= hf )
         {
            pmcd->bRectSection = true;
            pmcd->MnMin = (0.36*Beta1 - 0.08*Beta1*Beta1)*fc*b*de*de;
         }
         else
         {
            // T-section behavior
            pmcd->bRectSection = false;
            pmcd->MnMin = (0.36*Beta1 - 0.08*Beta1*Beta1)*fc*bw*de*de 
                        + 0.85*Beta1*fc*(b - bw)*hf*(de - 0.5*hf);
         }

         if ( !bPositiveMoment )
         {
            pmcd->MnMin *= -1;
         }
      }
      else
      {
         // Dummy values
         pmcd->bRectSection = true;
         pmcd->MnMin = 0;
      }
   }
   else
   {
      // WSDOT method 2005... LRFD 2006 and later

      // the method of compute phi based on strains was introduced in WSDOT 2005/LRFD 2006, however it only included
      // prestressing strand and grade 60 rebar. PGSuper can model grade 40-80 rebar. Use the strain based method
      // for computing Phi. This method is in WSDOT BDM 2012 and will be in LRFD 2013
      Float64 ecl, etl;
      if ( bIsSplicedGirder )
      {
         pResistanceFactors->GetFlexuralStrainLimits(pTendon->GetGrade(),pTendon->GetType(),&ecl,&etl);
      }
      else
      {
         if ( bPositiveMoment )
         {
            pResistanceFactors->GetFlexuralStrainLimits(pStrand->GetGrade(),pStrand->GetType(),&ecl,&etl);
         }
         else
         {
            pResistanceFactors->GetFlexuralStrainLimits(deckRebarGrade,&ecl,&etl);
         }
      }
      pmcd->ecl = ecl;
      pmcd->etl = etl;

      // Compute Phi based on the net tensile strain....
      // This not applicable at closure joints
      CClosureKey closureKey;
      if ( !pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         if ( IsZero(pmcd->c) ) 
         {
            if ( bIsSplicedGirder )
            {
               pmcd->Phi = PhiSP;
            }
            else
            {
               pmcd->Phi = (bPositiveMoment ? PhiPS : PhiRC); // there is no moment capacity, use PhiRC for phi instead of dividing by zero
            }
         }
         else
         {
            pmcd->et = (pmcd->dt - pmcd->c)*0.003/(pmcd->c);
            if ( bIsSplicedGirder )
            {
               pmcd->Phi = PhiC + 0.20*(pmcd->et - ecl)/(etl-ecl);
            }
            else
            {
               if ( bPositiveMoment )
               {
                  pmcd->Phi = PhiC + 0.25*(pmcd->et - ecl)/(etl-ecl);
               }
               else
               {
                  pmcd->Phi = PhiC + 0.15*(pmcd->et - ecl)/(etl-ecl);
               }
            }
         }

         if ( bIsSplicedGirder )
         {
            pmcd->Phi = ForceIntoRange(PhiC,pmcd->Phi,PhiSP);
         }
         else
         {
            pmcd->Phi = ForceIntoRange(PhiC,pmcd->Phi,PhiRC + (PhiPS-PhiRC)*pmcd->PPR);
         }
      }
   }

   pmcd->fpe_ps      = fpe_ps;
   pmcd->eps_initial = eps_initial;

   pmcd->fpe_pt      = fpe_pt;
   pmcd->ept_initial = ept_initial;

#if defined _DEBUG
   m_Log << _T("Dist from end ") << poi.GetDistFromStart() << endl;
   m_Log << _T("-------------------------") << endl;
   m_Log << endl;
#endif
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{
   GET_IFACE(IMomentCapacity,pMomentCapacity);

   MOMENTCAPACITYDETAILS mcd;
   pMomentCapacity->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,&mcd);

   CRACKINGMOMENTDETAILS cmd;
   pMomentCapacity->GetCrackingMomentDetails(intervalIdx,poi,bPositiveMoment,&cmd);

   ComputeMinMomentCapacity(intervalIdx,poi,bPositiveMoment,mcd,cmd,pmmcd);
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{
   GET_IFACE(IMomentCapacity,pMomentCapacity);

   MOMENTCAPACITYDETAILS mcd;
   pMomentCapacity->GetMomentCapacityDetails(intervalIdx,poi,config,bPositiveMoment,&mcd);

   CRACKINGMOMENTDETAILS cmd;
   pMomentCapacity->GetCrackingMomentDetails(intervalIdx,poi,config,bPositiveMoment,&cmd);

   ComputeMinMomentCapacity(intervalIdx,poi,bPositiveMoment,mcd,cmd,pmmcd);
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,const CRACKINGMOMENTDETAILS& cmd,MINMOMENTCAPDETAILS* pmmcd)
{
   Float64 Mr;     // Nominal resistance (phi*Mn)
   Float64 MrMin;  // Minimum nominal resistance - Min(MrMin1,MrMin2)
   Float64 MrMin1; // 1.2Mcr
   Float64 MrMin2; // 1.33Mu
   Float64 Mu_StrengthI;
   Float64 Mu_StrengthII;
   Float64 Mu;     // Limit State Moment
   Float64 Mcr;    // Cracking moment

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable( poi.GetSegmentKey() );

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2002  = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2003Interims ? true : false );
   bool bBefore2012 = ( pSpecEntry->GetSpecificationType() <  lrfdVersionMgr::SixthEdition2012 ? true : false );

   GET_IFACE(IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(bPositiveMoment ? pgsTypes::Maximize : pgsTypes::Minimize);

   Mr = mcd.Phi * mcd.Mn;

   if ( bAfter2002 && bBefore2012 )
   {
      Mcr = (bPositiveMoment ? Max(cmd.Mcr,cmd.McrLimit) : Min(cmd.Mcr,cmd.McrLimit));
   }
   else
   {
      Mcr = cmd.Mcr;
   }


   if ( bPositiveMoment )
   {
      Float64 MuMin_StrengthI, MuMax_StrengthI;
      pLimitStateForces->GetMoment(intervalIdx,pgsTypes::StrengthI,poi,bat,&MuMin_StrengthI,&MuMax_StrengthI);
      Mu_StrengthI = MuMax_StrengthI;
   }
   else
   {
      Mu_StrengthI = pLimitStateForces->GetSlabDesignMoment(pgsTypes::StrengthI,poi,bat);
   }
      
   if ( bPermit )
   {
      if ( bPositiveMoment )
      {
         Float64 MuMin_StrengthII, MuMax_StrengthII;
         pLimitStateForces->GetMoment(intervalIdx,pgsTypes::StrengthII,poi,bat,&MuMin_StrengthII,&MuMax_StrengthII);
         Mu_StrengthII = MuMax_StrengthII;
      }
      else
      {
         Mu_StrengthII = pLimitStateForces->GetSlabDesignMoment(pgsTypes::StrengthII,poi,bat);
      }
   }
   else
   {
      Mu_StrengthII = (bPositiveMoment ? DBL_MAX : -DBL_MAX);
   }

   pgsTypes::LimitState ls; // limit state of controlling Mu (greatest magnitude)
   if ( bPermit )
   {
      if ( fabs(Mu_StrengthII) < fabs(Mu_StrengthI) )
      {
         Mu = Mu_StrengthI;
         ls = pgsTypes::StrengthI;
      }
      else
      {
         Mu = Mu_StrengthII;
         ls = pgsTypes::StrengthII;
      }
   }
   else
   {
      Mu = Mu_StrengthI;
      ls = pgsTypes::StrengthI;
   }

   if ( lrfdVersionMgr::SixthEdition2012 <= pSpecEntry->GetSpecificationType() )
   {
      MrMin1 = Mcr;
   }
   else
   {
      MrMin1 = 1.20*Mcr;
   }


   MrMin2 = 1.33*Mu;
   MrMin =  (bPositiveMoment ? Min(MrMin1,MrMin2) : Max(MrMin1,MrMin2));

   if (ls==pgsTypes::StrengthI)
   {
      pmmcd->LimitState = _T("Strength I");
   }
   else 
   {
      ATLASSERT(ls==pgsTypes::StrengthII);
      pmmcd->LimitState = _T("Strength II");
   }

   pmmcd->Mr     = Mr;
   pmmcd->MrMin  = MrMin;
   pmmcd->MrMin1 = MrMin1;
   pmmcd->MrMin2 = MrMin2;
   pmmcd->Mu     = Mu;
   pmmcd->Mcr    = Mcr;
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPretensionStresses,pPrestress);

   GET_IFACE(IPosttensionStresses,pPosttension);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 fcpe = 0; // compressive stress in concrete due to effective prestress forces only
                     // (after allowance for all prestress losses) at extreme fiber of 
                     // section where tensile stress is caused by externally applied loads

   pgsTypes::StressLocation stressLocation = (bPositiveMoment ? pgsTypes::BottomGirder : pgsTypes::TopDeck);

   // Compute stress due to prestressing
   Float64 Pps = pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End);
   Float64 ns_eff;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   Float64 eps = pStrandGeom->GetEccentricity( releaseIntervalIdx, poi, pgsTypes::Permanent, &ns_eff ); // eccentricity of non-composite section

   Float64 dfcpe = pPrestress->GetStress(releaseIntervalIdx, poi,stressLocation,Pps,eps);
   if ( dfcpe < 0 )
   {
      fcpe += dfcpe; // only want compression stress
   }

   // Compute stress due to tendons
   dfcpe = pPosttension->GetStress(intervalIdx,poi,stressLocation,ALL_DUCTS);
   if ( dfcpe < 0 )
   {
      fcpe += dfcpe;
   }

   // We have computed a compression stress so its sign is < 0. However, in the context of computing
   // the cracking moment, we want the equal and opposite tension stress that must overcome this compression
   // to induce cracking. Change sign of stress for use in LRFD Eq. 5.7.3.3.2-1
   ATLASSERT(fcpe <= 0);
   fcpe *= -1;

   ComputeCrackingMoment(intervalIdx,poi,fcpe,bPositiveMoment,pcmd);
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   Float64 fcpe; // Stress at bottom of non-composite girder due to prestress

#if defined _DEBUG
   // version of this method with GDRCONFIG does not apply to spliced girder bridges
   GET_IFACE(ITendonGeometry,pTendonGeom);
   ATLASSERT(pTendonGeom->GetDuctCount(poi.GetSegmentKey()) == 0);
#endif

   if ( bPositiveMoment )
   {
      // Get stress at bottom of girder due to prestressing
      // Using negative because we want the amount tensile stress required to overcome the
      // precompression
      GET_IFACE(IPretensionForce,pPrestressForce);
      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(IPretensionStresses,pPrestress);

      Float64 P = pPrestressForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent,pgsTypes::ServiceI,config);
      Float64 ns_eff;
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
      Float64 e = pStrandGeom->GetEccentricity( releaseIntervalIdx, poi, config, pgsTypes::Permanent, &ns_eff ); // eccentricity of non-composite section

      fcpe = -pPrestress->GetStress(releaseIntervalIdx,poi,pgsTypes::BottomGirder,P,e);
   }
   else
   {
      // no precompression in the slab
      fcpe = 0;
   }

   ComputeCrackingMoment(intervalIdx,config,poi,fcpe,bPositiveMoment,pcmd);
}

void pgsMomentCapacityEngineer::GetCrackingMomentFactors(bool bPositiveMoment,Float64* pG1,Float64* pG2,Float64* pG3)
{
   if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
   {
      *pG1 = 1.6; // all other concrete structures (not-segmental)
      *pG2 = 1.1; // bonded strand/tendon

      if ( bPositiveMoment )
      {
         *pG3 = 1.0; // prestressed concrete
      }
      else
      {
         GET_IFACE(IBridge,pBridge);
         if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
         {
            *pG3 = 1.0; // prestress concrete (no deck, so all we have is the beam)
         }
         else
         {
            GET_IFACE(IMaterials,pMaterials);
            Float64 E,fy,fu;
            pMaterials->GetDeckRebarProperties(&E,&fy,&fu);

            *pG3 = fy/fu;
         }
      }
   }
   else
   {
      *pG1 = 1.0;
      *pG2 = 1.0;
      *pG3 = 1.0;
   }
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get dead load moment on non-composite girder
   Mdnc = GetNonCompositeDeadLoadMoment(intervalIdx,poi,config,bPositiveMoment);
   fr = GetModulusOfRupture(intervalIdx,config,bPositiveMoment);

   GetSectionProperties(intervalIdx,poi,config,bPositiveMoment,&Sb,&Sbc);

   Float64 g1,g2,g3;
   GetCrackingMomentFactors(bPositiveMoment,&g1,&g2,&g3);

   ComputeCrackingMoment(g1,g2,g3,fr,fcpe,Mdnc,Sb,Sbc,pcmd);
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get dead load moment on non-composite girder
   Mdnc = GetNonCompositeDeadLoadMoment(intervalIdx,poi,bPositiveMoment);
   fr = GetModulusOfRupture(intervalIdx,poi,bPositiveMoment);

   GetSectionProperties(intervalIdx,poi,bPositiveMoment,&Sb,&Sbc);

   Float64 g1,g2,g3;
   GetCrackingMomentFactors(bPositiveMoment,&g1,&g2,&g3);

   ComputeCrackingMoment(g1,g2,g3,fr,fcpe,Mdnc,Sb,Sbc,pcmd);
}

Float64 pgsMomentCapacityEngineer::GetNonCompositeDeadLoadMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment)
{
   GET_IFACE(IProductForces,pProductForces);
   Float64 Mdnc = GetNonCompositeDeadLoadMoment(intervalIdx,poi,bPositiveMoment);

   // add effect of different slab offset
   Float64 deltaSlab = pProductForces->GetDesignSlabMomentAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi);
   Mdnc += deltaSlab;

   Float64 deltaSlabPad = pProductForces->GetDesignSlabPadMomentAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi);
   Mdnc += deltaSlabPad;

   return Mdnc;
}

Float64 pgsMomentCapacityEngineer::GetNonCompositeDeadLoadMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   GET_IFACE(IProductForces,pProductForces);

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(bPositiveMoment ? pgsTypes::Maximize : pgsTypes::Minimize);

   Float64 Mdnc = 0; // Dead load moment on non-composite girder

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if ( bPositiveMoment )
   {
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType intervalIdx;
      if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
      {
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
         intervalIdx = compositeDeckIntervalIdx - 1;
      }
      else
      {
         intervalIdx = pIntervals->GetIntervalCount(segmentKey) - 1;
      }

      // Girder moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pftGirder,poi, bat, rtCumulative);

      // Slab moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pftSlab,   poi, bat, rtCumulative);
      Mdnc += pProductForces->GetMoment(intervalIdx,pftSlabPad,poi, bat, rtCumulative);

      // Diaphragm moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pftDiaphragm,poi, bat, rtCumulative);

      // Shear Key moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pftShearKey,poi, bat, rtCumulative);

      // User DC and User DW
      Mdnc += pProductForces->GetMoment(intervalIdx,pftUserDC,poi, bat, rtCumulative);
      Mdnc += pProductForces->GetMoment(intervalIdx,pftUserDW,poi, bat, rtCumulative);
   }

   return Mdnc;
}

Float64 pgsMomentCapacityEngineer::GetModulusOfRupture(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   GET_IFACE(IMaterials,pMaterial);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 fr;   // Rupture stress
   // Compute modulus of rupture
   if ( bPositiveMoment )
   {
      fr = pMaterial->GetSegmentFlexureFr(segmentKey,intervalIdx);
   }
   else
   {
      GET_IFACE(IBridge,pBridge);
      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      {
         fr = pMaterial->GetSegmentFlexureFr(segmentKey,intervalIdx);
      }
      else
      {
         fr = pMaterial->GetDeckFlexureFr(segmentKey,intervalIdx);
      }
   }

   return fr;
}

Float64 pgsMomentCapacityEngineer::GetModulusOfRupture(IntervalIndexType intervalIdx,const GDRCONFIG& config,bool bPositiveMoment)
{
   GET_IFACE(IMaterials,pMaterial);

   Float64 fr;   // Rupture stress
   // Compute modulus of rupture
   if ( bPositiveMoment )
   {
#pragma Reminder("BUG: need modulus of rupture for fc and the specified interval")
      fr = pMaterial->GetFlexureModRupture(config.Fc,config.ConcType);
   }
   else
   {
      GET_IFACE(IBridge,pBridge);
      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      {
#pragma Reminder("BUG: need modulus of rupture for fc and the specified interval")
         fr = pMaterial->GetFlexureModRupture(config.Fc,config.ConcType);
      }
      else
      {
         fr = pMaterial->GetDeckFlexureFr(config.SegmentKey,intervalIdx);
      }
   }

   return fr;
}

void pgsMomentCapacityEngineer::GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,Float64* pSb,Float64* pSbc)
{
   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get the section moduli
   if ( bPositiveMoment )
   {
      GET_IFACE(IPointOfInterest,pPoi);

      Sbc = pSectProp->GetS(intervalIdx,poi,pgsTypes::BottomGirder);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         Sb = Sbc;
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(poi.GetSegmentKey());
         Sb  = pSectProp->GetS(castDeckIntervalIdx,poi,pgsTypes::BottomGirder);
      }
   }
   else
   {
      Sbc = pSectProp->GetS(intervalIdx,poi,pgsTypes::TopDeck);
      Sb  = Sbc;
   }

   *pSb  = Sb;
   *pSbc = Sbc;
}

void pgsMomentCapacityEngineer::GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,Float64* pSb,Float64* pSbc)
{
   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get the section moduli
   if ( bPositiveMoment )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(poi.GetSegmentKey());
      Sb  = pSectProp->GetS(castDeckIntervalIdx,poi,pgsTypes::BottomGirder,config.Fc);
      Sbc = pSectProp->GetS(intervalIdx,        poi,pgsTypes::BottomGirder,config.Fc);
   }
   else
   {
      Sbc = pSectProp->GetS(intervalIdx,poi,pgsTypes::TopDeck,config.Fc);
      Sb  = Sbc;
   }

   *pSb  = Sb;
   *pSbc = Sbc;
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(Float64 g1,Float64 g2,Float64 g3,Float64 fr,Float64 fcpe,Float64 Mdnc,Float64 Sb,Float64 Sbc,CRACKINGMOMENTDETAILS* pcmd)
{
   Float64 Mcr = g3*((g1*fr + g2*fcpe)*Sbc - Mdnc*(Sbc/Sb - 1));

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2002 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2003Interims ? true : false );
   if ( bAfter2002 )
   {
      Float64 McrLimit = Sbc*fr;
      pcmd->McrLimit = McrLimit;
   }

   pcmd->Mcr  = Mcr;
   pcmd->Mdnc = Mdnc;
   pcmd->fr   = fr;
   pcmd->fcpe = fcpe;
   pcmd->Sb   = Sb;
   pcmd->Sbc  = Sbc;
   pcmd->g1   = g1;
   pcmd->g2   = g2;
   pcmd->g3   = g3;
}

void pgsMomentCapacityEngineer::AnalyzeCrackedSection(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   pgsBondTool bondTool(m_pBroker,poi);

   // create a problem to solve
   // the cracked section analysis tool uses the same model as the moment capacity tool
   CComPtr<IGeneralSection> beam_section;
   CComPtr<IPoint2d> pntCompression; // needed to figure out the result geometry
   CComPtr<ISize2d> szOffset; // distance to offset coordinates from bridge model to capacity model
   std::map<StrandIndexType,Float64> bond_factors[2];
   Float64 dt; // depth from top of section to extreme layer of tensile reinforcement
   Float64 H; // overall height of section
   Float64 Haunch; // haunch build up that is modeled

   Float64 e_initial_strands = 0;
   std::vector<Float64> e_initial_tendons;
   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   e_initial_tendons.insert(e_initial_tendons.begin(),nDucts,0);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
   BuildCapacityProblem(liveLoadIntervalIdx,poi,NULL,e_initial_strands,e_initial_tendons,bondTool,bPositiveMoment,&beam_section,&pntCompression,&szOffset,&dt,&H,&Haunch,bond_factors);

   // determine neutral axis angle
   // compression is on the left side of the neutral axis
   Float64 na_angle = (bPositiveMoment ? 0.00 : M_PI);

   CComPtr<ICrackedSectionSolution> solution;
   m_CrackedSectionSolver->putref_Section(beam_section);
   m_CrackedSectionSolver->put_Slices(20);
   m_CrackedSectionSolver->put_SliceGrowthFactor(2);
   m_CrackedSectionSolver->put_CGTolerance(0.001);
   HRESULT hr = m_CrackedSectionSolver->Solve(na_angle,&solution);
   ATLASSERT(SUCCEEDED(hr));

   pCSD->CrackedSectionSolution = solution;
  

   ///////////////////////////////////////////
   // Compute I-crack
   ///////////////////////////////////////////

   // use the WBFL Sections library
   CComPtr<ICompositeSection> composite_section;
   composite_section.CoCreateInstance(CLSID_CompositeSection);

   // add each slice into a composite section object
   CollectionIndexType nSlices;
   solution->get_SliceCount(&nSlices);
   for ( CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++ )
   {
      CComPtr<ICrackedSectionSlice> slice;
      solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      Float64 Efg, Ebg;
      slice->get_Efg(&Efg);
      slice->get_Ebg(&Ebg);

      if ( !IsZero(Efg) )
      {
         // only add slices that aren't cracked
         composite_section->AddSection(shape,Efg,1,VARIANT_FALSE,VARIANT_TRUE);

         if ( !IsZero(Ebg) )
         {
            // add the void
            composite_section->AddSection(shape,Ebg,1,VARIANT_TRUE,VARIANT_TRUE);
         }
      }
   }

   // get the elastic properties
   CComQIPtr<ISection> section(composite_section);
   CComPtr<IElasticProperties> elastic_properties;
   section->get_ElasticProperties(&elastic_properties);

   // transform properties into girder matieral
   GET_IFACE(IMaterials,pMaterials);
   Float64 Eg = pMaterials->GetSegmentEc(segmentKey,liveLoadIntervalIdx);

   CComPtr<IShapeProperties> shape_properties;
   elastic_properties->TransformProperties(Eg,&shape_properties);

   // Icrack for girder
   Float64 Icr;
   shape_properties->get_Ixx(&Icr);
   pCSD->Icr = Icr;

   // distance from top of section to the cracked centroid
   Float64 c;
   shape_properties->get_Ytop(&c);
   pCSD->c = c;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsMomentCapacityEngineer::MakeCopy(const pgsMomentCapacityEngineer& rOther)
{
   // Add copy code here...
   m_pBroker = rOther.m_pBroker;
   m_CrackedSectionSolver = rOther.m_CrackedSectionSolver;
   m_MomentCapacitySolver = rOther.m_MomentCapacitySolver;
}

void pgsMomentCapacityEngineer::MakeAssignment(const pgsMomentCapacityEngineer& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsMomentCapacityEngineer::CreateStrandMaterial(const CSegmentKey& segmentKey,IStressStrain** ppSS)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);

   StrandGradeType grade = pStrand->GetGrade() == matPsStrand::Gr1725 ? sgtGrade250 : sgtGrade270;
   ProductionMethodType type = pStrand->GetType() == matPsStrand::LowRelaxation ? pmtLowRelaxation : pmtStressRelieved;

   CComPtr<IPowerFormula> powerFormula;
   powerFormula.CoCreateInstance(CLSID_PSPowerFormula);
   powerFormula->put_Grade(grade);
   powerFormula->put_ProductionMethod(type);

   CComQIPtr<IStressStrain> ssStrand(powerFormula);
   (*ppSS) = ssStrand;
   (*ppSS)->AddRef();
}

void pgsMomentCapacityEngineer::CreateTendonMaterial(const CGirderKey& girderKey,IStressStrain** ppSS)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pTendon = pMaterial->GetTendonMaterial(girderKey);

   StrandGradeType grade = pTendon->GetGrade() == matPsStrand::Gr1725 ? sgtGrade250 : sgtGrade270;
   ProductionMethodType type = pTendon->GetType() == matPsStrand::LowRelaxation ? pmtLowRelaxation : pmtStressRelieved;

   CComPtr<IPowerFormula> powerFormula;
   powerFormula.CoCreateInstance(CLSID_PSPowerFormula);
   powerFormula->put_Grade(grade);
   powerFormula->put_ProductionMethod(type);

   CComQIPtr<IStressStrain> ssStrand(powerFormula);
   (*ppSS) = ssStrand;
   (*ppSS)->AddRef();
}

void pgsMomentCapacityEngineer::BuildCapacityProblem(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64 eps_initial,const std::vector<Float64>& ept_initial,pgsBondTool& bondTool,bool bPositiveMoment,IGeneralSection** ppProblem,IPoint2d** pntCompression,ISize2d** szOffset,Float64* pdt,Float64* pH,Float64* pHaunch,std::map<StrandIndexType,Float64>* pBondFactors)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IShapes, pShapes);
   GET_IFACE(IPointOfInterest,pPoi);

   CComPtr<IPoint2d> pntTension; // location of the extreme tension face

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CClosureKey closureKey;
   bool bIsInClosure = pPoi->IsInClosureJoint(poi,&closureKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

   Float64 dt = 0; // depth from compression face to extreme layer of tensile reinforcement

   StrandIndexType Ns(0), Nh(0);
   if ( pConfig )
   {
      Ns = pConfig->PrestressConfig.GetStrandCount(pgsTypes::Straight);
      Nh = pConfig->PrestressConfig.GetStrandCount(pgsTypes::Harped);
   }
   else
   {
      GET_IFACE(IStrandGeometry, pStrandGeom);
      Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
      Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   }

   //
   // Create Materials
   //

   // strand
   CComPtr<IStressStrain> ssStrand;
   CreateStrandMaterial(segmentKey,&ssStrand);

   // tendon
   CComPtr<IStressStrain> ssTendon;
   CreateTendonMaterial(segmentKey,&ssTendon);

   // girder concrete
   CComPtr<IUnconfinedConcrete> matGirder;
   matGirder.CoCreateInstance(CLSID_UnconfinedConcrete);
   if ( pConfig )
   {
      matGirder->put_fc( pConfig->Fc );
   }
   else
   {
      if ( bIsInClosure )
      {
         // poi is in a closure joint
         matGirder->put_fc( pMaterial->GetClosureJointFc(closureKey,intervalIdx) );
      }
      else if ( pPoi->IsOffSegment(poi) )
      {
         // poi is not in the segment and isn't in a closure joint
         // this means the POI is in a cast-in-place diaphragm between girder groups
         // assume cast in place diaphragms between groups is the same material as the deck
         // because they are typically cast together
         ATLASSERT(poi.HasAttribute(POI_BOUNDARY_PIER) || poi.IsTenthPoint(POI_SPAN) == 1);
         matGirder->put_fc( pMaterial->GetDeckFc(segmentKey,intervalIdx) );
      }
      else
      {
         matGirder->put_fc( pMaterial->GetSegmentFc(segmentKey,intervalIdx) );
      }
   }
   CComQIPtr<IStressStrain> ssGirder(matGirder);

   // slab concrete
   CComPtr<IUnconfinedConcrete> matSlab;
   matSlab.CoCreateInstance(CLSID_UnconfinedConcrete);
   matSlab->put_fc( pMaterial->GetDeckFc(segmentKey,intervalIdx) );
   CComQIPtr<IStressStrain> ssSlab(matSlab);

   // girder rebar
   CComPtr<IRebarModel> matGirderRebar;
   matGirderRebar.CoCreateInstance(CLSID_RebarModel);
   Float64 E, Fy, Fu;
   if ( bIsInClosure )
   {
      pMaterial->GetClosureJointLongitudinalRebarProperties(closureKey,&E,&Fy,&Fu);
   }
   else
   {
      pMaterial->GetSegmentLongitudinalRebarProperties(segmentKey,&E,&Fy,&Fu);
   }
   matGirderRebar->Init( Fy, E, 1.00 );
   CComQIPtr<IStressStrain> ssGirderRebar(matGirderRebar);

   // slab rebar
   CComPtr<IRebarModel> matSlabRebar;
   matSlabRebar.CoCreateInstance(CLSID_RebarModel);
   pMaterial->GetDeckRebarProperties(&E,&Fy,&Fu);
   matSlabRebar->Init( Fy, E, 1.00 );
   CComQIPtr<IStressStrain> ssSlabRebar(matSlabRebar);

   //
   // Build the section
   //
   CComPtr<IGeneralSection> section;
   section.CoCreateInstance(CLSID_GeneralSection);

   
   // beam shape
   CComPtr<IShape> shapeBeam;
   pShapes->GetSegmentShape(intervalIdx,poi,false,pgsTypes::scGirder,&shapeBeam);
   CComQIPtr<ICompositeShape> compBeam(shapeBeam);

   if ( compBeam && // beam is a composite (made of many shapes) 
        pBridge->GetDeckType() != pgsTypes::sdtNone && // bridge has a deck
        compositeDeckIntervalIdx <= intervalIdx // deck is become composite with the girder
      )
   {
      // The last item is the bridge deck and we don't want it...
      // We will be adding the deck below
      IndexType nShapes;
      compBeam->get_Count(&nShapes);
      compBeam->Remove(nShapes-1);
   }

   // offset each shape so that the origin of the composite (if it is composite)
   // is located at the centroid of the bare girder (this keeps the moment capacity solver happy)
   // Use the same offset to position the rebar
   CComPtr<IShapeProperties> props;
   shapeBeam->get_ShapeProperties(&props);
   CComPtr<IPoint2d> cgBeam;
   props->get_Centroid(&cgBeam);
   Float64 dx,dy;
   cgBeam->get_X(&dx);
   cgBeam->get_Y(&dy);

   // need to return the offset for use later
   CComPtr<ISize2d> size;
   size.CoCreateInstance(CLSID_Size2d);
   size->put_Dx(dx);
   size->put_Dy(dy);
   *szOffset = size;
   (*szOffset)->AddRef();

   // start building up the capacity model
   CComQIPtr<IXYPosition> posBeam(shapeBeam);
   if ( compBeam )
   {
      CollectionIndexType shapeCount;
      compBeam->get_Count(&shapeCount);

      for ( CollectionIndexType idx = 0; idx < shapeCount; idx++ )
      {
         CComPtr<ICompositeShapeItem> csItem;
         compBeam->get_Item(idx,&csItem);

         CComPtr<IShape> shape;
         csItem->get_Shape(&shape);

         CComQIPtr<IXYPosition> position(shape);
         position->Offset(-dx,-dy);

         VARIANT_BOOL bVoid;
         csItem->get_Void(&bVoid);

         if ( bVoid == VARIANT_FALSE )
         {
            AddShape2Section(section,shape,ssGirder,NULL,0.00);
         }
         else
         {
            // void shape... use only a background material (backgrounds are subtracted)
            AddShape2Section(section,shape,NULL,ssGirder,0.00);
         }
      } // next shape
   }
   else
   {
      // beam shape isn't composite so just add it
      posBeam->Offset(-dx,-dy);
      AddShape2Section(section,shapeBeam,ssGirder,NULL,0.00);
   }

   // so far there is no deck in the model.... 
   // if this is for positive moment the compression point is top center, otherwise bottom center
   if ( bPositiveMoment )
   {
      posBeam->get_LocatorPoint(lpTopCenter,pntCompression);
      posBeam->get_LocatorPoint(lpBottomCenter,&pntTension);
   }
   else
   {
      posBeam->get_LocatorPoint(lpBottomCenter,pntCompression);
      posBeam->get_LocatorPoint(lpTopCenter,&pntTension);
   }

   Float64 Yc; // elevation of the extreme compression fiber
   (*pntCompression)->get_Y(&Yc);

   if ( bPositiveMoment ) // only model strands for positive moment
   {
      // strands
      const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
      Float64 aps = pStrand->GetNominalArea();
      Float64 dps = pStrand->GetNominalDiameter();
      GET_IFACE(IStrandGeometry, pStrandGeom);
      for ( int i = 0; i < 2; i++ ) // straight and harped strands
      {
         StrandIndexType nStrands = (i == 0 ? Ns : Nh);
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)(i);
         CComPtr<IPoint2dCollection> points; // strand points are in Girder Section Coordinates
         if ( pConfig )
         {
            pStrandGeom->GetStrandPositionsEx(poi, pConfig->PrestressConfig, strandType, &points);
         }
         else
         {
            pStrandGeom->GetStrandPositions(poi, strandType, &points);
         }

         /////////////////////////////////////////////
         // We know that we have symmetric section and that strands are generally in rows.
         // Create a single "lump of strand" for each row instead of modeling each strand 
         // individually. This will speed up the solver by quite a bit

         RowIndexType nStrandRows = pStrandGeom->GetNumRowsWithStrand(segmentKey,nStrands,strandType);
         for ( RowIndexType rowIdx = 0; rowIdx < nStrandRows; rowIdx++ )
         {
            Float64 rowArea = 0;
            std::vector<StrandIndexType> strandIdxs = pStrandGeom->GetStrandsInRow(segmentKey,nStrands,rowIdx,strandType);

#if defined _DEBUG
            StrandIndexType nStrandsInRow = pStrandGeom->GetNumStrandInRow(segmentKey,nStrands,rowIdx,strandType);
            ATLASSERT( nStrandsInRow == strandIdxs.size() );
#endif

            ATLASSERT( 0 < strandIdxs.size() );
            BOOST_FOREACH(StrandIndexType strandIdx,strandIdxs)
            {
               ATLASSERT( strandIdx < nStrands );

               bool bDebonded = bondTool.IsDebonded(strandIdx,strandType);
               if ( bDebonded )
               {
                  // strand is debonded... don't add it... go to the next strand
                  continue;
               }
   
               // get the bond factor (this will reduce the effective area of the strand if it isn't fully developed)
               Float64 bond_factor = bondTool.GetBondFactor(strandIdx,strandType);

               pBondFactors[i].insert( std::make_pair(strandIdx,bond_factor) );

               rowArea += bond_factor*aps;
            }

            // create a single equivalent rectangle for the area of reinforcement in this row
            Float64 h = dps; // height is diamter of strand
            Float64 w = rowArea/dps;

            CComPtr<IRectangle> bar_shape;
            bar_shape.CoCreateInstance(CLSID_Rect);
            bar_shape->put_Width(w);
            bar_shape->put_Height(h);

            // get one strand from the row and get it's Y value
            CComPtr<IPoint2d> point;
            points->get_Item(strandIdxs[0],&point);
            Float64 rowY;
            point->get_Y(&rowY);
            point.Release();

            // position the "strand" rectangle
            CComQIPtr<IXYPosition> position(bar_shape);
            CComPtr<IPoint2d> hp;
            position->get_LocatorPoint(lpHookPoint,&hp);
            hp->Move(0,rowY);
            hp->Offset(-dx,-dy);

            // determine depth to lowest layer of strand
            Float64 cy;
            hp->get_Y(&cy);
            dt = Max(dt,fabs(Yc-cy));

            CComQIPtr<IShape> shape(bar_shape);
            AddShape2Section(section,shape,ssStrand,ssGirder,eps_initial);

#if defined _DEBUG
            CComPtr<IShapeProperties> props;
            shape->get_ShapeProperties(&props);
            Float64 area;
            props->get_Area(&area);
            ATLASSERT( IsEqual(area,rowArea) );
#endif // _DEBUG
         }

      } // next strand type
   }

   // PT Tendons
   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CComPtr<IPoint2d> point;
      pTendonGeom->GetDuctPoint(poi,ductIdx,&point);

      Float64 area = pTendonGeom->GetTendonArea(segmentKey,intervalIdx,ductIdx);

      Float64 s = sqrt(area); // side of equivalent square (area = s*s)

      CComPtr<IRectangle> tendon_shape;
      tendon_shape.CoCreateInstance(CLSID_Rect);
      tendon_shape->put_Width(s);
      tendon_shape->put_Height(s);

      CComQIPtr<IXYPosition> position(tendon_shape);
      CComPtr<IPoint2d> hp;
      position->get_LocatorPoint(lpHookPoint,&hp);
      hp->MoveEx(point);
      hp->Offset(-dx,-dy);

      // determine depth to lowest layer of strand
      Float64 cy;
      hp->get_Y(&cy);
      dt = Max(dt,fabs(Yc-cy));

      Float64 epti = ept_initial[ductIdx];

      CComQIPtr<IShape> shape(tendon_shape);
      AddShape2Section(section,shape,ssTendon,ssGirder,epti);
   }

   // girder rebar
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->IncludeRebarForMoment() )
   {
      GET_IFACE(ILongRebarGeometry, pRebarGeom);

      CComPtr<IRebarSection> rebar_section;
      pRebarGeom->GetRebars(poi,&rebar_section);
      
      CComPtr<IEnumRebarSectionItem> enumItems;
      rebar_section->get__EnumRebarSectionItem(&enumItems);

      CComPtr<IRebarSectionItem> item;
      while ( enumItems->Next(1,&item,NULL) != S_FALSE )
      {
         CComPtr<IPoint2d> location;
         item->get_Location(&location);

         Float64 x,y;
         location->get_X(&x);
         location->get_Y(&y);

         CComPtr<IRebar> rebar;
         item->get_Rebar(&rebar);
         Float64 as;
         rebar->get_NominalArea(&as);

         Float64 dev_length_factor = pRebarGeom->GetDevLengthFactor(poi,item);

         // create an "area perfect" square
         // (clips are lot faster than a circle)
         Float64 s = sqrt(dev_length_factor*as);

         CComPtr<IRectangle> square;
         square.CoCreateInstance(CLSID_Rect);
         square->put_Width(s);
         square->put_Height(s);
         
         CComPtr<IPoint2d> hp;
         square->get_HookPoint(&hp);
         hp->MoveEx(location);
         hp->Offset(-dx,-dy);

         Float64 cy;
         hp->get_Y(&cy);
         dt = Max(dt,fabs(Yc-cy));

         CComQIPtr<IShape> shape(square);
         AddShape2Section(section,shape,ssGirderRebar,ssGirder,0);

         item.Release();
      }
   }

   if ( (pBridge->GetDeckType() != pgsTypes::sdtNone && // if there is a deck
         pBridge->IsCompositeDeck() && // the deck is composite
         compositeDeckIntervalIdx <= intervalIdx) // interval at or after deck is composite
      )
   {
      // add the deck to the model
      GET_IFACE(ISectionProperties, pSectProp);
      Float64 Weff = pSectProp->GetEffectiveFlangeWidth(poi);
      Float64 Dslab = pBridge->GetStructuralSlabDepth(poi);

      // so far, dt is measured from top of girder (if positive moment)
      // since we have a deck, add Dslab so that dt is measured from top of slab
      // If dt is zero, there wasn't any reinforcement so don't add Dslab
      if ( bPositiveMoment && !IsZero(dt) )
      {
         dt += Dslab;
      }

      CComPtr<IRectangle> rect;
      rect.CoCreateInstance(CLSID_Rect);
      rect->put_Height(Dslab);
      rect->put_Width(Weff);

      CComQIPtr<IXYPosition> posDeck(rect);

      if ( bPositiveMoment )
      {
         // put the bottom center of the deck rectangle right on the top center of the beam
         CComPtr<IPoint2d> pntCommon;
         posBeam->get_LocatorPoint(lpTopCenter,&pntCommon);
         posDeck->put_LocatorPoint(lpBottomCenter,pntCommon);

         *pHaunch = 0; // not modeling haunch buildup for positive moment
      }
      else
      {
        // put slab in correct location to account for additional moment arm due to "A" dimension
        Float64 top_girder_to_top_slab = pSectProp->GetDistTopSlabToTopGirder(poi); // does not account for camber
        Float64 excess_camber;
        GET_IFACE(ICamber,pCamber);
        if ( pConfig )
        {
           excess_camber = pCamber->GetExcessCamber(poi,*pConfig,CREEP_MAXTIME);
        }
        else
        {
           excess_camber = pCamber->GetExcessCamber(poi,CREEP_MAXTIME);
        }

        Float64 top_girder_to_bottom_slab = top_girder_to_top_slab - Dslab - excess_camber;
        if ( top_girder_to_bottom_slab < 0 )
        {
           top_girder_to_bottom_slab = 0;
        }

        ATLASSERT(0 <= top_girder_to_bottom_slab);

        CComPtr<IPoint2d> pntCommon;
        posBeam->get_LocatorPoint(lpTopCenter,&pntCommon);
        pntCommon->Offset(0,top_girder_to_bottom_slab);

        posDeck->put_LocatorPoint(lpBottomCenter,pntCommon);

        *pHaunch = top_girder_to_bottom_slab;

        if ( !IsZero(top_girder_to_bottom_slab) )
        {
           Float64 x_centerline;
           pntCommon->get_X(&x_centerline);
           GET_IFACE(IGirder,pGirder);
           FlangeIndexType nTopFlanges = pGirder->GetNumberOfTopFlanges(segmentKey);
           for ( FlangeIndexType flangeIdx = 0; flangeIdx < nTopFlanges; flangeIdx++ )
           {
              Float64 offset = pGirder->GetTopFlangeLocation(poi,flangeIdx);
              Float64 top_flange_width = pGirder->GetTopFlangeWidth(poi,flangeIdx);

              CComPtr<IRectangle> haunch;
              haunch.CoCreateInstance(CLSID_Rect);
              haunch->put_Height(top_girder_to_bottom_slab);
              haunch->put_Width(top_flange_width);

              Float64 x = x_centerline + offset;
              pntCommon->put_X(x);
              CComQIPtr<IXYPosition> posHaunch(haunch);
              posHaunch->put_LocatorPoint(lpTopCenter,pntCommon);

              CComQIPtr<IShape> shapeHaunch(haunch);
              AddShape2Section(section,shapeHaunch,ssSlab,NULL,0.00);
           }
         }
      }


      // if this is positive moment and we have a deck, the extreme compression point is top center
      if (bPositiveMoment)
      {
         (*pntCompression)->Release();
         posDeck->get_LocatorPoint(lpTopCenter,pntCompression);
      }
      else
      {
         pntTension.Release();
         posDeck->get_LocatorPoint(lpTopCenter,&pntTension);
      }

      CComQIPtr<IShape> shapeDeck(posDeck);

      AddShape2Section(section,shapeDeck,ssSlab,NULL,0.00);


      // deck rebar if this is for negative moment
      if ( !bPositiveMoment )
      {
#pragma Reminder("UPDATE: use the correct location of deck mat rebar")
         // NOTE: The value return from GetCoverTop/BottomMat() is the raw input. The raw input
         // is the cover to the center of the lump sum input and the CLEAR COVER to the individual bar input.
         // The CG of the individual and lump sum input are not at the same location.
         // Use the GetTop/BottomMatLocation() function to get the CG location measured from the bottom of the deck.
         // 
         // NOTE: NOTE: NOTE *****!!!!!!******!!!!
         // 
         // Don't make this change until all of the regression tests from the 2.x PGSuper branch are validated
         // This change will slightly alter the negative moment capacity results. We don't want to be dealing with
         // changed results while doing the initial validating.
         //
         // THIS MUST BE ONE OF THE LAST CHANGES TO MAKE BEFORE MOVING EVERYTHING TO THE 3.0 BRANCH
         GET_IFACE(ILongRebarGeometry, pRebarGeom);
         Float64 AsTop = pRebarGeom->GetAsTopMat(poi,ILongRebarGeometry::All);

         if ( !IsZero(AsTop) )
         {
            Float64 coverTop = pRebarGeom->GetCoverTopMat();
            Float64 equiv_height = AsTop / Weff; // model deck rebar as rectangles of equivalent area
            Float64 equiv_width = Weff;
            if ( equiv_height < Dslab/16. )
            {
               // of the equivalent height is too sort, it doesn't model well
               equiv_height = Dslab/16.;
               equiv_width = AsTop/equiv_height;
            }
            CComPtr<IRectangle> topRect;
            topRect.CoCreateInstance(CLSID_Rect);
            topRect->put_Height(equiv_height);
            topRect->put_Width(equiv_width);

            // move the center of the rebar rectangle below the top of the deck rectangle by the cover amount.
            // center it horizontally
            CComQIPtr<IXYPosition> posTop(topRect);
            CComPtr<IPoint2d> pntDeck;
            posDeck->get_LocatorPoint(lpTopCenter,&pntDeck);
            pntDeck->Offset(0,-coverTop);
            posTop->put_LocatorPoint(lpCenterCenter,pntDeck);

            Float64 cy;
            pntDeck->get_Y(&cy);
            dt = Max(dt,fabs(Yc-cy));

            CComQIPtr<IShape> shapeTop(posTop);
            AddShape2Section(section,shapeTop,ssSlabRebar,ssSlab,0.00);
         }


         Float64 AsBottom = pRebarGeom->GetAsBottomMat(poi,ILongRebarGeometry::All);
         if ( !IsZero(AsBottom) )
         {
            Float64 coverBottom = pRebarGeom->GetCoverBottomMat();
            Float64 equiv_height = AsBottom / Weff;
            Float64 equiv_width = Weff;
            if ( equiv_height < Dslab/16. )
            {
               // of the equivalent height is too sort, it doesn't model well
               equiv_height = Dslab/16.;
               equiv_width = AsBottom/equiv_height;
            }
            CComPtr<IRectangle> botRect;
            botRect.CoCreateInstance(CLSID_Rect);
            botRect->put_Height(equiv_height);
            botRect->put_Width(equiv_width);

            // move the center of the rebar rectangle above the bottom of the deck rectangle by the cover amount.
            // center it horizontally
            CComQIPtr<IXYPosition> posBottom(botRect);
            CComPtr<IPoint2d> pntDeck;
            posDeck->get_LocatorPoint(lpBottomCenter,&pntDeck);
            pntDeck->Offset(0,coverBottom);
            posBottom->put_LocatorPoint(lpCenterCenter,pntDeck);

            Float64 cy;
            pntDeck->get_Y(&cy);
            dt = Max(dt,fabs(Yc-cy));

            CComQIPtr<IShape> shapeBottom(posBottom);
            AddShape2Section(section,shapeBottom,ssSlabRebar,ssSlab,0.00);
         }
      }
   }


   // measure from bottom of beam to top of deck to get height
   pntTension->DistanceEx(*pntCompression,pH);

   *pdt = dt;

   (*ppProblem) = section;
   (*ppProblem)->AddRef();
}

//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsMomentCapacityEngineer::AssertValid() const
{
   return true;
}

void pgsMomentCapacityEngineer::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsMomentCapacityEngineer") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsMomentCapacityEngineer::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsMomentCapacityEngineer");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsMomentCapacityEngineer");

   TESTME_EPILOG("MomentCapacityEngineer");
}
#endif // _UNITTEST

#if defined _DEBUG_SECTION_DUMP
void pgsMomentCapacityEngineer::DumpSection(const pgsPointOfInterest& poi,IGeneralSection* section, std::map<long,Float64> ssBondFactors,std::map<long,Float64> hsBondFactors,bool bPositiveMoment)
{
   std::_tostringstream os;
   std::_tstring strMn(bPositiveMoment ? "+M" : "-M"); 
   os << "GeneralSection_" << strMn << "_Span_" << LABEL_SPAN(poi.GetSpan()) << "_Girder_" << LABEL_GIRDER(poi.GetGirder()) << "_" << ::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << ".txt";
   std::_tofstream file(os.str().c_str());

   long shape_count;
   section->get_ShapeCount(&shape_count);
   for ( long idx = 0; idx < shape_count; idx++ )
   {
      file << (idx+1) << std::endl;

      CComPtr<IShape> shape;
      section->get_Shape(idx,&shape);

      CComPtr<IPoint2dCollection> points;
      shape->get_PolyPoints(&points);

      CComPtr<IPoint2d> point;
      CComPtr<IEnumPoint2d> enum_points;
      points->get__Enum(&enum_points);
      while ( enum_points->Next(1,&point,0) == S_OK )
      {
         Float64 x,y;
         point->get_X(&x);
         point->get_Y(&y);

         file << ::ConvertFromSysUnits(x,unitMeasure::Inch) << "," << ::ConvertFromSysUnits(y,unitMeasure::Inch) << std::endl;

         point.Release();
      }
   }

   file << "done" << std::endl;

   file << "Straight Strand Bond Factors" << std::endl;
   std::map<long,Float64>::iterator iter;
   for ( iter = ssBondFactors.begin(); iter != ssBondFactors.end(); iter++ )
   {
      file << iter->first << ", " << iter->second << std::endl;
   }

   file << "Harped Strand Bond Factors" << std::endl;
   for ( iter = hsBondFactors.begin(); iter != hsBondFactors.end(); iter++ )
   {
      file << iter->first << ", " << iter->second << std::endl;
   }

   file.close();
}
#endif // _DEBUG_SECTION_DUMP

pgsMomentCapacityEngineer::pgsBondTool::pgsBondTool(IBroker* pBroker,const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   m_pBroker    = pBroker;
   m_Poi        = poi;
   m_Config     = config;
   m_bUseConfig = true;
   Init();
}

pgsMomentCapacityEngineer::pgsBondTool::pgsBondTool(IBroker* pBroker,const pgsPointOfInterest& poi)
{
   m_pBroker    = pBroker;
   m_Poi        = poi;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   m_CurrentConfig = pBridge->GetSegmentConfiguration(segmentKey);
   m_bUseConfig = false;
   Init();
}

void pgsMomentCapacityEngineer::pgsBondTool::Init()
{
   m_pBroker->GetInterface(IID_IPretensionForce,(IUnknown**)&m_pPrestressForce);

   const CSegmentKey& segmentKey = m_Poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   m_GirderLength = pBridge->GetSegmentLength(segmentKey);

   m_DistFromStart = m_Poi.GetDistFromStart();

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L) );
   ASSERT( vPOI.size() == 1 );
   m_PoiMidSpan = vPOI[0];

   /////// -- NOTE -- //////
   // Development length, and hence the development length adjustment factor, require the result
   // of a moment capacity analysis. fps is needed to compute development length, yet, development
   // length is needed to adjust the effectiveness of the strands for moment capacity analysis.
   //
   // This causes a circular dependency. However, the development length calculation only needs
   // fps for the capacity at the mid-span section. Unless the bridge is extremely short, the
   // strands will be fully developed at mid-span.
   //
   // If the poi is around mid-span, assume a development length factor of 1.0 otherwise compute it.
   //
   Float64 fra = 0.25; // 25% either side of centerline
   m_bNearMidSpan = false;
   if ( InRange(fra*m_GirderLength,m_DistFromStart,(1-fra)*m_GirderLength))
   {
      m_bNearMidSpan = true;
   }
}

Float64 pgsMomentCapacityEngineer::pgsBondTool::GetBondFactor(StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
   const CSegmentKey& segmentKey = m_Poi.GetSegmentKey();

   // NOTE: More tricky code here (see note above)
   //
   // If we have a section that isn't near mid-span, we have to compute a bond factor. The bond factor is
   // a function of development length and development length needs fps and fpe which are computed in the
   // moment capacity analysis... again, circular dependency here.
   //
   // To work around this, the bond factors for moment capacity analysis are computed based on fps and fpe
   // at mid span.
   Float64 bond_factor = 1;
   if ( !m_bNearMidSpan )
   {
      if ( m_bUseConfig )
      {
         GET_IFACE(IStrandGeometry,pStrandGeom);
         Float64 bond_start, bond_end;
         bool bDebonded = pStrandGeom->IsStrandDebonded(segmentKey,strandIdx,strandType,m_Config,&bond_start,&bond_end);
         STRANDDEVLENGTHDETAILS dev_length = m_pPrestressForce->GetDevLengthDetails(m_PoiMidSpan,m_Config,bDebonded);

         bond_factor = m_pPrestressForce->GetStrandBondFactor(m_Poi,m_Config,strandIdx,strandType,dev_length.fps,dev_length.fpe);
      }
      else
      {
         GET_IFACE(IStrandGeometry,pStrandGeom);
         Float64 bond_start, bond_end;
         bool bDebonded = pStrandGeom->IsStrandDebonded(segmentKey,strandIdx,strandType,&bond_start,&bond_end);
         STRANDDEVLENGTHDETAILS dev_length = m_pPrestressForce->GetDevLengthDetails(m_PoiMidSpan,bDebonded);

         bond_factor = m_pPrestressForce->GetStrandBondFactor(m_Poi,strandIdx,strandType,dev_length.fps,dev_length.fpe);
      }
   }

   return bond_factor;
}

bool pgsMomentCapacityEngineer::pgsBondTool::IsDebonded(StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
   bool bDebonded = false;

   GDRCONFIG& config = (m_bUseConfig ? m_Config : m_CurrentConfig);

   BOOST_FOREACH(const DEBONDCONFIG& debondConfig,config.PrestressConfig.Debond[strandType])
   {
      if ( debondConfig.strandIdx == strandIdx &&
          ((m_DistFromStart < debondConfig.DebondLength[pgsTypes::metStart]) || ((m_GirderLength - debondConfig.DebondLength[pgsTypes::metEnd]) < m_DistFromStart)) )
      {
         // this strand is debonded at this POI... next strand
         bDebonded = true;
         break;
      }
   }

   return bDebonded;
}