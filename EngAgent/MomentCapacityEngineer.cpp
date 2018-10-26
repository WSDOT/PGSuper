///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

void pgsMomentCapacityEngineer::ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IPrestressForce, pPrestressForce);
   GET_IFACE(IBridgeMaterial,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent);

   Float64 Eps = pStrand->GetE();
   Float64 fpe = 0.0; // effective prestress after all losses
   Float64 e_initial = 0.0; // initial strain in the prestressing strands (strain at effect prestress)
   if ( bPositiveMoment )
   {
      // only for positive moment... strands are ignored for negative moment analysis
      fpe = pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterLosses);
      e_initial = fpe/Eps;
   }

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);

   pgsBondTool bondTool(m_pBroker,poi);

   ComputeMomentCapacity(stage,poi,config,fpe,e_initial,bondTool,bPositiveMoment,pmcd);
}

void pgsMomentCapacityEngineer::ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IPrestressForce, pPrestressForce);
   GET_IFACE(IBridgeMaterial,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent);

   Float64 Eps = pStrand->GetE();
   Float64 fpe = 0.0; // effective prestress after all losses
   Float64 e_initial = 0.0; // initial strain in the prestressing strands (strain at effect prestress)
   if ( bPositiveMoment )
   {
      // only for positive moment... strands are ignored for negative moment analysis
      fpe = pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,config,pgsTypes::AfterLosses);
      e_initial = fpe/Eps;
   }

   pgsBondTool bondTool(m_pBroker,poi,config);

   ComputeMomentCapacity(stage,poi,config,fpe,e_initial,bondTool,bPositiveMoment,pmcd);
}

void pgsMomentCapacityEngineer::ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64 fpe,Float64 e_initial,pgsBondTool& bondTool,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
   GET_IFACE(IBridge, pBridge);
   GET_IFACE(IPrestressForce, pPrestressForce);
   GET_IFACE(ILongRebarGeometry, pLongRebarGeom);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   StrandIndexType Ns = config.PrestressConfig.GetNStrands(pgsTypes::Straight);
   StrandIndexType Nh = config.PrestressConfig.GetNStrands(pgsTypes::Harped);

   // create a problem to solve
   CComPtr<IGeneralSection> section;
   CComPtr<IPoint2d> pntCompression; // needed to figure out the result geometry
   CComPtr<ISize2d> szOffset; // distance to offset coordinates from bridge model to capacity model
   std::map<StrandIndexType,Float64> bond_factors[2];
   Float64 dt; // depth from top of section to extreme layer of tensile reinforcement
   Float64 H; // overall height of section
   BuildCapacityProblem(stage,poi,config,e_initial,bondTool,bPositiveMoment,&section,&pntCompression,&szOffset,&dt,&H,bond_factors);

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

            const unitmgtLengthData& unit = pDisplayUnits->GetSpanLengthUnit();
            CString msg;
            msg.Format(_T("An unknown error occured while computing %s moment capacity for Span %d Girder %s at %f %s from the left end of the girder (%d)"),
                        (bPositiveMoment ? _T("positive") : _T("negative")),
                        LABEL_SPAN(poi.GetSpan()),
                        LABEL_GIRDER(poi.GetGirder()),
                        ::ConvertFromSysUnits(poi.GetDistFromStart(),unit.UnitOfMeasure),
                        unit.UnitOfMeasure.UnitTag().c_str(),
                        hr);
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
      pmcd->PPR = (bPositiveMoment ? pLongRebarGeom->GetPPRBottomHalf(poi,config) : 0.0);
   }
   else
   {
      // PPR was removed from LRFD in 6th Edition, 2012. Use 1.0 for positive moments so
      // Phi = PhiRC + (PhiPS-PhiRC)*PPR = PhiRc + PhiPS - PhiRc = PhiPS and use 0.0 for negative moment so
      // Phi = PhiRC + (PhiPS-PhiRC)*0.0 = PhiRC
      //
      // See computation of Phi about 5 lines down
      pmcd->PPR = (bPositiveMoment ? 1.0 : 0.0); // PPR was removed from LRFD in 6th Edition, 2012.. Use 0.0
   }

   GET_IFACE(IBridgeMaterialEx,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetGdrConcreteType(poi.GetSpan(),poi.GetGirder());

   matRebar::Type rebarType;
   matRebar::Grade deckRebarGrade;
   pMaterial->GetDeckRebarMaterial(rebarType,deckRebarGrade);

   GET_IFACE(IResistanceFactors,pResistanceFactors);
   Float64 PhiRC,PhiPS,PhiC;
   pResistanceFactors->GetFlexureResistanceFactors(concType,&PhiPS,&PhiRC,&PhiC);
   pmcd->Phi = PhiRC + (PhiPS-PhiRC)*pmcd->PPR; // generalized form of 5.5.4.2.1-3
                                                // Removed in AASHTO LRFD 6th Edition 2012, however
                                                // PPR has been computed above to take this into account

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

   const matPsStrand* pStrand = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent);

   if ( !IsZero(Mn) )
   {
      pmcd->MomentArm = fabs(Mn/T);

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

      Float64 aps = pStrand->GetNominalArea();

      // determine average stress in strands and location of de
      // de: see PCI BDM 8.4.1.2.
      Float64 t = 0; // summ of the tension forces in the strands
      Float64 tde = 0; // summ of location of strand time tension force in strand
      // de = tde/t
      if ( bPositiveMoment && 0 <  Ns + Nh )
      {
         CComPtr<IStressStrain> ssStrand;
         ssStrand.CoCreateInstance(CLSID_PSPowerFormula);
         GET_IFACE(IStrandGeometry,pStrandGeom);
         for ( int i = 0; i < 2; i++ ) // straight and harped strands
         {
            pgsTypes::StrandType strandType = (i == 0 ? pgsTypes::Straight : pgsTypes::Harped);
            CComPtr<IPoint2dCollection> points;
            pStrandGeom->GetStrandPositionsEx(poi, config.PrestressConfig, strandType, &points);

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
               ssStrand->ComputeStress(z+e_initial,&stress);

               stress *= bond_factor;

               // this is for de shear... tension side means strands below h/2
               // if there is tensile stress in the strand,
               // sum the force in the strand and the force in the strand times it y location
               if ( y < H/2 )
               {
                  ATLASSERT( 0 <= stress );
                  Float64 _t = aps*stress;
                  t += _t;
                  tde += (_t)*(y);
               }

               fps_avg += stress;

               point.Release();
            }
         }

         fps_avg /= (Ns+Nh);

         pmcd->de_shear = IsZero(t) ? 0 : H - tde/t;
      } // if ( Ns+Nh
      else
      {
         if ( bPositiveMoment )
         {
            // this happens when Ns+Nh is zero
            pmcd->de_shear = 0; 
         }
         else
         {
            pmcd->de_shear = pmcd->de;
         }
      }
   }
   else
   {
      // dimensions have no meaning if no moment capacity
      pmcd->c          = 0.0;
      pmcd->dc         = 0.0;
      pmcd->MomentArm  = 0.0;
      pmcd->de         = 0.0;
      pmcd->de_shear   = 0.0;
      
      fps_avg          = 0.0;
   }

   WATCHX(MomCap,0, _T("X = ") << ::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft") << _T("   Mn = ") << ::ConvertFromSysUnits(Mn,unitMeasure::KipFeet) << _T(" kip-ft") << _T(" My/Mx = ") << My/Mn << _T(" fps_avg = ") << ::ConvertFromSysUnits(fps_avg,unitMeasure::KSI) << _T(" KSI"));

   pmcd->fps = fps_avg;
   pmcd->dt = dt;
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
         GET_IFACE(IBridgeMaterial,pMaterial);
         Float64 de = pmcd->de;
         Float64 c  = pmcd->c;

         Float64 hf;
         Float64 b;
         Float64 bw;
         Float64 fc;
         Float64 Beta1;
         if ( bPositiveMoment )
         {
            GET_IFACE(ISectProp2,pProps);

            GET_IFACE(IGirder, pGdr);
            bw = pGdr->GetShearWidth(poi);

            if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
            {
               b     = pGdr->GetTopFlangeWidth(poi);
               hf    = pGdr->GetMinTopFlangeThickness(poi);
               fc    = pMaterial->GetFcGdr(span,gdr);
               Beta1 = lrfdConcreteUtil::Beta1(fc);
            }
            else
            {
               b     = pProps->GetEffectiveFlangeWidth(poi);
               hf    = pBridge->GetStructuralSlabDepth(poi);
               fc    = pMaterial->GetFcSlab();
               Beta1 = lrfdConcreteUtil::Beta1(fc);
            }
         }
         else
         {
            GET_IFACE(IGirder, pGdr);
            hf = pGdr->GetMinBottomFlangeThickness(poi);
            b  = pGdr->GetBottomWidth(poi);
            bw = pGdr->GetShearWidth(poi);
            fc = pMaterial->GetFcGdr(span,gdr);
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
            pmcd->MnMin *= -1;
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
      if ( bPositiveMoment )
      {
         pResistanceFactors->GetFlexuralStrainLimits(pStrand->GetGrade(),pStrand->GetType(),&ecl,&etl);
      }
      else
      {
         pResistanceFactors->GetFlexuralStrainLimits(deckRebarGrade,&ecl,&etl);
      }
      pmcd->ecl = ecl;
      pmcd->etl = etl;

      if ( IsZero(pmcd->c) ) 
      {
         pmcd->Phi = (bPositiveMoment ? PhiPS : PhiRC); // there is no moment capacity, use PhiRC for phi instead of dividing by zero
      }
      else
      {

         pmcd->et = (pmcd->dt - pmcd->c)*0.003/(pmcd->c);
         if ( bPositiveMoment )
            pmcd->Phi = PhiC + 0.25*(pmcd->et - ecl)/(etl-ecl);
         else
            pmcd->Phi = PhiC + 0.15*(pmcd->et - ecl)/(etl-ecl);
      }

      pmcd->Phi = ForceIntoRange(PhiC,pmcd->Phi,PhiRC + (PhiPS-PhiRC)*pmcd->PPR);
   }

   pmcd->fpe = fpe;
   pmcd->e_initial = e_initial;

#if defined _DEBUG
   m_Log << _T("Dist from end ") << poi.GetDistFromStart() << endl;
   m_Log << _T("-------------------------") << endl;
   m_Log << endl;
#endif
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{
   GET_IFACE(IMomentCapacity,pMomentCapacity);

   MOMENTCAPACITYDETAILS mcd;
   pMomentCapacity->GetMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);

   CRACKINGMOMENTDETAILS cmd;
   pMomentCapacity->GetCrackingMomentDetails(stage,poi,bPositiveMoment,&cmd);

   ComputeMinMomentCapacity(stage,poi,bPositiveMoment,mcd,cmd,pmmcd);
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{
   GET_IFACE(IMomentCapacity,pMomentCapacity);

   MOMENTCAPACITYDETAILS mcd;
   pMomentCapacity->GetMomentCapacityDetails(stage,poi,config,bPositiveMoment,&mcd);

   CRACKINGMOMENTDETAILS cmd;
   pMomentCapacity->GetCrackingMomentDetails(stage,poi,config,bPositiveMoment,&cmd);

   ComputeMinMomentCapacity(stage,poi,bPositiveMoment,mcd,cmd,pmmcd);
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,const CRACKINGMOMENTDETAILS& cmd,MINMOMENTCAPDETAILS* pmmcd)
{
   Float64 Mr;     // Nominal resistance (phi*Mn)
   Float64 MrMin;  // Minimum nominal resistance - min(MrMin1,MrMin2)
   Float64 MrMin1; // 1.2Mcr
   Float64 MrMin2; // 1.33Mu
   Float64 Mu_StrengthI;
   Float64 Mu_StrengthII;
   Float64 Mu;     // Limit State Moment
   Float64 Mcr;    // Cracking moment

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(poi.GetSpan(), poi.GetGirder());

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2002  = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2003Interims ? true : false );
   bool bBefore2012 = ( pSpecEntry->GetSpecificationType() <  lrfdVersionMgr::SixthEdition2012 ? true : false );

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat;
   if (analysisType == pgsTypes::Simple)
   {
      bat = SimpleSpan;
   }
   else if (analysisType == pgsTypes::Continuous)
   {
      bat = ContinuousSpan;
   }
   else
   {
      // envelope
      if ( bPositiveMoment )
         bat = MaxSimpleContinuousEnvelope;
      else
         bat = MinSimpleContinuousEnvelope;
   }

   Mr = mcd.Phi * mcd.Mn;

   if ( bAfter2002 && bBefore2012 )
      Mcr = (bPositiveMoment ? max(cmd.Mcr,cmd.McrLimit) : min(cmd.Mcr,cmd.McrLimit));
   else
      Mcr = cmd.Mcr;


   if ( bPositiveMoment )
   {
      Float64 MuMin_StrengthI, MuMax_StrengthI;
      pLimitStateForces->GetMoment(pgsTypes::StrengthI,stage,poi,bat,&MuMin_StrengthI,&MuMax_StrengthI);
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
         pLimitStateForces->GetMoment(pgsTypes::StrengthII,stage,poi,bat,&MuMin_StrengthII,&MuMax_StrengthII);
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
      MrMin1 = Mcr;
   else
      MrMin1 = 1.20*Mcr;


   MrMin2 = 1.33*Mu;
   MrMin =  (bPositiveMoment ? min(MrMin1,MrMin2) : max(MrMin1,MrMin2));

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

void pgsMomentCapacityEngineer::ComputeCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPrestressStresses,pPrestress);
   Float64 fcpe; // Stress at bottom of non-composite girder due to prestress

   if ( bPositiveMoment )
   {
      // Get stress at bottom of girder due to prestressing
      // Using negative because we want the amount tensile stress required to overcome the
      // precompression
      Float64 P = pPrestressForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterLosses);
      Float64 ns_eff;
      Float64 e = pStrandGeom->GetEccentricity( poi, false, &ns_eff );

      fcpe = -pPrestress->GetStress(poi,pgsTypes::BottomGirder,P,e);
   }
   else
   {
      // no precompression in the slab
      fcpe = 0;
   }

   ComputeCrackingMoment(stage,poi,fcpe,bPositiveMoment,pcmd);
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPrestressStresses,pPrestress);
   Float64 fcpe; // Stress at bottom of non-composite girder due to prestress

   if ( bPositiveMoment )
   {
      // Get stress at bottom of girder due to prestressing
      // Using negative because we want the amount tensile stress required to overcome the
      // precompression
      Float64 P = pPrestressForce->GetStrandForce(poi,pgsTypes::Permanent,config,pgsTypes::AfterLosses);
      Float64 ns_eff;
      Float64 e = pStrandGeom->GetEccentricity( poi, config.PrestressConfig, false, &ns_eff);

      fcpe = -pPrestress->GetStress(poi,pgsTypes::BottomGirder,P,e);
   }
   else
   {
      // no precompression in the slab
      fcpe = 0;
   }

   ComputeCrackingMoment(stage,config,poi,fcpe,bPositiveMoment,pcmd);
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
            GET_IFACE(IBridgeMaterial,pMaterials);
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

void pgsMomentCapacityEngineer::ComputeCrackingMoment(pgsTypes::Stage stage,const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get dead load moment on non-composite girder
   Mdnc = GetNonCompositeDeadLoadMoment(stage,poi,config,bPositiveMoment);
   fr = GetModulusOfRupture(config,bPositiveMoment);

   GetSectionProperties(stage,poi,config,bPositiveMoment,&Sb,&Sbc);

   Float64 g1,g2,g3;
   GetCrackingMomentFactors(bPositiveMoment,&g1,&g2,&g3);

   ComputeCrackingMoment(g1,g2,g3,fr,fcpe,Mdnc,Sb,Sbc,pcmd);
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get dead load moment on non-composite girder
   Mdnc = GetNonCompositeDeadLoadMoment(stage,poi,bPositiveMoment);
   fr = GetModulusOfRupture(poi,bPositiveMoment);

   GetSectionProperties(stage,poi,bPositiveMoment,&Sb,&Sbc);

   Float64 g1,g2,g3;
   GetCrackingMomentFactors(bPositiveMoment,&g1,&g2,&g3);

   ComputeCrackingMoment(g1,g2,g3,fr,fcpe,Mdnc,Sb,Sbc,pcmd);
}

Float64 pgsMomentCapacityEngineer::GetNonCompositeDeadLoadMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment)
{
   GET_IFACE(IProductForces,pProductForces);
   Float64 Mdnc = GetNonCompositeDeadLoadMoment(stage,poi,bPositiveMoment);
   // add effect of different slab offset
   Float64 deltaSlab = pProductForces->GetDesignSlabPadMomentAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi);
   Mdnc += deltaSlab;

   return Mdnc;
}

Float64 pgsMomentCapacityEngineer::GetNonCompositeDeadLoadMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   GET_IFACE(IProductLoads,pProductLoads);
   GET_IFACE(IProductForces,pProductForces);

   Float64 Mdnc = 0; // Dead load moment on non-composite girder

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   pgsTypes::Stage girderLoadStage = pProductLoads->GetGirderDeadLoadStage(span,gdr);

   if ( bPositiveMoment )
   {
      // Girder moment
      Mdnc += pProductForces->GetMoment(girderLoadStage,pftGirder,poi, SimpleSpan);

      // Slab moment
      Mdnc += pProductForces->GetMoment(pgsTypes::BridgeSite1,pftSlab,poi, SimpleSpan);

      // Slab pad moment
      Mdnc += pProductForces->GetMoment(pgsTypes::BridgeSite1,pftSlabPad,poi, SimpleSpan);

      // Diaphragm moment
      Mdnc += pProductForces->GetMoment(pgsTypes::BridgeSite1,pftDiaphragm,poi, SimpleSpan);

      // Shear Key moment
      Mdnc += pProductForces->GetMoment(pgsTypes::BridgeSite1,pftShearKey,poi, SimpleSpan);

      // User DC and User DW
      Mdnc += pProductForces->GetMoment(pgsTypes::BridgeSite1,pftUserDC,poi, SimpleSpan);
      Mdnc += pProductForces->GetMoment(pgsTypes::BridgeSite1,pftUserDW,poi, SimpleSpan);
   }

   return Mdnc;
}

Float64 pgsMomentCapacityEngineer::GetModulusOfRupture(const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   GET_IFACE(IBridgeMaterial,pMaterial);

   Float64 fr;   // Rupture stress
   // Compute modulus of rupture
   if ( bPositiveMoment )
   {
      fr = pMaterial->GetFlexureFrGdr(poi.GetSpan(),poi.GetGirder());
   }
   else
   {
      GET_IFACE(IBridge,pBridge);
      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
         fr = pMaterial->GetFlexureFrGdr(poi.GetSpan(),poi.GetGirder());
      else
         fr = pMaterial->GetFlexureFrSlab();
   }

   return fr;
}

Float64 pgsMomentCapacityEngineer::GetModulusOfRupture(const GDRCONFIG& config,bool bPositiveMoment)
{
   GET_IFACE(IBridgeMaterialEx,pMaterial);

   Float64 fr;   // Rupture stress
   // Compute modulus of rupture
   if ( bPositiveMoment )
   {
      fr = pMaterial->GetFlexureModRupture(config.Fc,config.ConcType);
   }
   else
   {
      GET_IFACE(IBridge,pBridge);
      if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
         fr = pMaterial->GetFlexureModRupture(config.Fc,config.ConcType);
      else
         fr = pMaterial->GetFlexureFrSlab();
   }

   return fr;
}

void pgsMomentCapacityEngineer::GetSectionProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,Float64* pSb,Float64* pSbc)
{
   GET_IFACE(ISectProp2,pSectProp2);

   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get the section moduli
   if ( bPositiveMoment )
   {
      Sb  = pSectProp2->GetSb(pgsTypes::BridgeSite1,poi);
      Sbc = pSectProp2->GetSb(stage,poi);
   }
   else
   {
      Sbc = pSectProp2->GetSt(stage,poi);
      Sb  = Sbc;
   }

   *pSb  = Sb;
   *pSbc = Sbc;
}

void pgsMomentCapacityEngineer::GetSectionProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,Float64* pSb,Float64* pSbc)
{
   GET_IFACE(ISectProp2,pSectProp2);

   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get the section moduli
   if ( bPositiveMoment )
   {
      Sb  = pSectProp2->GetSb(pgsTypes::BridgeSite1,poi,config.Fc);
      Sbc = pSectProp2->GetSb(stage,poi,config.Fc);
   }
   else
   {
      Sbc = pSectProp2->GetSt(stage,poi,config.Fc);
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
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(spanIdx,gdrIdx);

   pgsBondTool bondTool(m_pBroker,poi);

   // create a problem to solve
   // the cracked section analysis tool uses the same model as the moment capacity tool
   CComPtr<IGeneralSection> beam_section;
   CComPtr<IPoint2d> pntCompression; // needed to figure out the result geometry
   CComPtr<ISize2d> szOffset; // distance to offset coordinates from bridge model to capacity model
   std::map<StrandIndexType,Float64> bond_factors[2];
   Float64 dt; // depth from top of section to extreme layer of tensile reinforcement
   Float64 H;
   BuildCapacityProblem(pgsTypes::BridgeSite3,poi,config,0,bondTool,bPositiveMoment,&beam_section,&pntCompression,&szOffset,&dt,&H,bond_factors);

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
   GET_IFACE(IBridgeMaterial,pMaterials);
   Float64 Eg = pMaterials->GetEcGdr(spanIdx,gdrIdx);

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
void pgsMomentCapacityEngineer::CreateStrandMaterial(SpanIndexType span,GirderIndexType gdr,IStressStrain** ppSS)
{
   GET_IFACE(IBridgeMaterial,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent);

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

void pgsMomentCapacityEngineer::BuildCapacityProblem(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64 e_initial,pgsBondTool& bondTool,bool bPositiveMoment,IGeneralSection** ppProblem,IPoint2d** pntCompression,ISize2d** szOffset,Float64* pdt,Float64* pH,std::map<StrandIndexType,Float64>* pBondFactors)
{
   ATLASSERT( stage == pgsTypes::BridgeSite3 );

   GET_IFACE(IPrestressForce,pPrestressForce);

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterial,pMaterial);
   GET_IFACE(ISectProp2, pSectProp);
   GET_IFACE(IStrandGeometry, pStrandGeom );
   GET_IFACE(ILongRebarGeometry, pRebarGeom);
   GET_IFACE(ICamber,pCamber);
   GET_IFACE(IGirder,pGirder);

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr  = poi.GetGirder();
   Float64 dist_from_start = poi.GetDistFromStart();

   Float64 gdr_length = pBridge->GetGirderLength(span,gdr);

   Float64 dt = 0; // depth from compression face to extreme layer of tensile reinforcement

   StrandIndexType Ns = config.PrestressConfig.GetNStrands(pgsTypes::Straight);
   StrandIndexType Nh = config.PrestressConfig.GetNStrands(pgsTypes::Harped);

   //
   // Create Materials
   //

   // strand
   CComPtr<IStressStrain> ssStrand;
   CreateStrandMaterial(span,gdr,&ssStrand);

   // girder concrete
   CComPtr<IUnconfinedConcrete> matGirder;
   matGirder.CoCreateInstance(CLSID_UnconfinedConcrete);
   matGirder->put_fc( config.Fc );
   CComQIPtr<IStressStrain> ssGirder(matGirder);

   // slab concrete
   CComPtr<IUnconfinedConcrete> matSlab;
   matSlab.CoCreateInstance(CLSID_UnconfinedConcrete);
   matSlab->put_fc( pMaterial->GetFcSlab() );
   CComQIPtr<IStressStrain> ssSlab(matSlab);

   // girder rebar
   CComPtr<IRebarModel> matGirderRebar;
   matGirderRebar.CoCreateInstance(CLSID_RebarModel);
   Float64 E, Fy, Fu;
   pMaterial->GetLongitudinalRebarProperties(span,gdr,&E,&Fy,&Fu);
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
   // if there is no deck, get the final shape of the girder
   // if there is a deck, get the basic shape and we'll add the deck below
   CComPtr<IShape> shapeBeam;
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      pSectProp->GetGirderShape(poi,pgsTypes::BridgeSite3,false,&shapeBeam);
   else
      pSectProp->GetGirderShape(poi,pgsTypes::CastingYard,false,&shapeBeam);
   
   // the shape is positioned relative to the bridge cross section
   // move the beam such that it's bottom center is at (0,0)
   // this will give us a nice coordinate system to work with
   // NOTE: Later we will move the origin to the CG of the section
   CComQIPtr<IXYPosition> posBeam(shapeBeam);
   CComPtr<IPoint2d> origin;
   origin.CoCreateInstance(CLSID_Point2d);
   origin->Move(0,0);
   posBeam->put_LocatorPoint(lpBottomCenter,origin);


   // offset each shape so that the origin of the composite (if it is composite)
   // is located at the origin (this keeps the moment capacity solver happy)
   // Use the same offset to position the rebar
   CComPtr<IShapeProperties> props;
   shapeBeam->get_ShapeProperties(&props);
   CComPtr<IPoint2d> cgBeam;
   props->get_Centroid(&cgBeam);
   Float64 dx,dy;
   cgBeam->get_X(&dx);
   cgBeam->get_Y(&dy);

   CComPtr<ISize2d> size;
   size.CoCreateInstance(CLSID_Size2d);
   size->put_Dx(dx);
   size->put_Dy(dy);
   *szOffset = size;
   (*szOffset)->AddRef();

   CComQIPtr<ICompositeShape> compBeam(shapeBeam);
   if ( compBeam )
   {
      // beam shape is composite

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
      }
   }
   else
   {
      // beam shape isn't composite so just add it
      posBeam->Offset(-dx,-dy);
      AddShape2Section(section,shapeBeam,ssGirder,NULL,0.00);
   }

   // so far there is no deck in the model.... if this is positive moment the compression point is top center, otherwise bottom center
   if ( bPositiveMoment )
      posBeam->get_LocatorPoint(lpTopCenter,pntCompression);
   else
      posBeam->get_LocatorPoint(lpBottomCenter,pntCompression);

   Float64 Yc;
   (*pntCompression)->get_Y(&Yc);

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
   bool bNearMidSpan = false;
   if ( InRange(fra*gdr_length,dist_from_start,(1-fra)*gdr_length))
      bNearMidSpan = true;


   if ( bPositiveMoment ) // only model strands for positive moment
   {
      // strands
      const matPsStrand* pStrand = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent);
      Float64 aps = pStrand->GetNominalArea();
      Float64 dps = pStrand->GetNominalDiameter();
      for ( int i = 0; i < 2; i++ ) // straight and harped strands
      {
         StrandIndexType nStrands = (i == 0 ? Ns : Nh);
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)(i);

         CComPtr<IPoint2dCollection> points;
         pStrandGeom->GetStrandPositionsEx(poi, config.PrestressConfig, strandType, &points);

         /////////////////////////////////////////////
         // We know that we have symmetric section and that strands are generally in rows.
         // Create a single "lump of strand" for each row instead of modeling each strand 
         // individually. This will spead up the solver by quite a bit

         RowIndexType nStrandRows = pStrandGeom->GetNumRowsWithStrand(span,gdr,config.PrestressConfig,strandType);
         for ( RowIndexType rowIdx = 0; rowIdx < nStrandRows; rowIdx++ )
         {
            Float64 rowArea = 0;
            std::vector<StrandIndexType> strandIdxs = pStrandGeom->GetStrandsInRow(span,gdr,config.PrestressConfig,rowIdx,strandType);

#if defined _DEBUG
            StrandIndexType nStrandsInRow = pStrandGeom->GetNumStrandInRow(span,gdr,config.PrestressConfig,rowIdx,strandType);
            ATLASSERT( nStrandsInRow == strandIdxs.size() );
#endif

            ATLASSERT( 0 < strandIdxs.size() );
            std::vector<StrandIndexType>::iterator iter;
            for ( iter = strandIdxs.begin(); iter != strandIdxs.end(); iter++ )
            {
               StrandIndexType strandIdx = *iter;
               ATLASSERT( strandIdx < nStrands );

               bool bDebonded = bondTool.IsDebonded(strandIdx,strandType);
               if ( bDebonded )
               {
                  // strand is debonded... don't add it... go to the next strand
                  continue;
               }
   
               // get the bond factor (this will reduce the effective area of the strand if it isn't fully developed)
               Float64 bond_factor = bondTool.GetBondFactor(strandIdx,strandType);

               // for negative moment, assume fully bonded
               if ( !bPositiveMoment )
                  bond_factor = 1.0;

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
            dt = _cpp_max(dt,fabs(Yc-cy));

            CComQIPtr<IShape> shape(bar_shape);
            AddShape2Section(section,shape,ssStrand,ssGirder,e_initial);

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

   // girder rebar
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( bPositiveMoment && pSpecEntry->IncludeRebarForMoment() )
   {
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

         Float64 dev_length_factor = pRebarGeom->GetDevLengthFactor(span, gdr, item);

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
         dt = _cpp_max(dt,fabs(Yc-cy));

         CComQIPtr<IShape> shape(square);
         AddShape2Section(section,shape,ssGirderRebar,ssGirder,0);

         item.Release();
      }
   }

   if ( (stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3) &&
         pBridge->GetDeckType() != pgsTypes::sdtNone && pBridge->IsCompositeDeck() )
   {
      // deck
      Float64 Weff = pSectProp->GetEffectiveFlangeWidth(poi);
      Float64 Dslab = pBridge->GetStructuralSlabDepth(poi);

      // so far, dt is measured from top of girder (if positive moment)
      // since we have a deck, add Dslab so that dt is measured from top of slab
      if ( bPositiveMoment )
         dt += Dslab;

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
      }
      else
      {
        // put slab in correct location to account for additional moment arm due to "A" dimension
        Float64 top_girder_to_top_slab = pSectProp->GetDistTopSlabToTopGirder(poi); // does not account for camber
        Float64 excess_camber = pCamber->GetExcessCamber(poi,config,CREEP_MAXTIME);
        Float64 top_girder_to_bottom_slab = top_girder_to_top_slab - Dslab - excess_camber;
        if ( top_girder_to_bottom_slab < 0 )
           top_girder_to_bottom_slab = 0;

        ATLASSERT(0 <= top_girder_to_bottom_slab);

         CComPtr<IPoint2d> pntCommon;
         posBeam->get_LocatorPoint(lpTopCenter,&pntCommon);
         pntCommon->Offset(0,top_girder_to_bottom_slab);

         posDeck->put_LocatorPoint(lpBottomCenter,pntCommon);

         if ( !IsZero(top_girder_to_bottom_slab) )
         {
            Float64 x_centerline;
            pntCommon->get_X(&x_centerline);
            FlangeIndexType nTopFlanges = pGirder->GetNumberOfTopFlanges(span,gdr);
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
         posDeck->get_LocatorPoint(lpTopCenter,pntCompression);

      CComQIPtr<IShape> shapeDeck(posDeck);

      AddShape2Section(section,shapeDeck,ssSlab,NULL,0.00);


      // deck rebar if this is for negative moment
      if ( !bPositiveMoment )
      {
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
            dt = _cpp_max(dt,fabs(Yc-cy));

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
            dt = _cpp_max(dt,fabs(Yc-cy));

            CComQIPtr<IShape> shapeBottom(posBottom);
            AddShape2Section(section,shapeBottom,ssSlabRebar,ssSlab,0.00);
         }
      }
   }


   // measure from bottom of beam to top of deck to get height
   CComPtr<IPoint2d> pntBottom;
   posBeam->get_LocatorPoint(lpBottomCenter,&pntBottom);
   pntBottom->DistanceEx(*pntCompression,pH);

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

   GET_IFACE(IBridge,pBridge);
   m_CurrentConfig = pBridge->GetGirderConfiguration(m_Poi.GetSpan(),m_Poi.GetGirder());
   m_bUseConfig = false;
   Init();
}

void pgsMomentCapacityEngineer::pgsBondTool::Init()
{
   GET_IFACE(IPrestressForce,pPrestressForce);
   m_pPrestressForce = pPrestressForce;

   GET_IFACE(IBridge,pBridge);
   m_GirderLength = pBridge->GetGirderLength(m_Poi.GetSpan(),m_Poi.GetGirder());

   m_DistFromStart = m_Poi.GetDistFromStart();

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(m_Poi.GetSpan(),m_Poi.GetGirder(),pgsTypes::BridgeSite3,POI_MIDSPAN) );
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
      m_bNearMidSpan = true;
}

Float64 pgsMomentCapacityEngineer::pgsBondTool::GetBondFactor(StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
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
         bool bDebonded = pStrandGeom->IsStrandDebonded(m_Poi.GetSpan(),m_Poi.GetGirder(),strandIdx,strandType,m_Config.PrestressConfig,&bond_start,&bond_end);
         STRANDDEVLENGTHDETAILS dev_length = m_pPrestressForce->GetDevLengthDetails(m_PoiMidSpan,m_Config,bDebonded);

         bond_factor = m_pPrestressForce->GetStrandBondFactor(m_Poi,m_Config,strandIdx,strandType,dev_length.fps,dev_length.fpe);
      }
      else
      {
         GET_IFACE(IStrandGeometry,pStrandGeom);
         Float64 bond_start, bond_end;
         bool bDebonded = pStrandGeom->IsStrandDebonded(m_Poi.GetSpan(),m_Poi.GetGirder(),strandIdx,strandType,&bond_start,&bond_end);
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

   DebondConfigConstIterator iter;
   for ( iter = config.PrestressConfig.Debond[strandType].begin(); iter != config.PrestressConfig.Debond[strandType].end(); iter++ )
   {
      const DEBONDCONFIG& di = *iter;

      if ( di.strandIdx == strandIdx &&
          ((m_DistFromStart < di.LeftDebondLength) || ((m_GirderLength - di.RightDebondLength) < m_DistFromStart)) )
      {
         // this strand is debonded at this POI... next strand
         bDebonded = true;
         break;
      }
   }

   return bDebonded;
}