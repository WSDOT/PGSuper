///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "TxDOTOptionalDesignDocProxyAgent.h"
#include "TxDOTOptionalDesignDoc.h"
#include "TxDOTAgent_i.h"
#include "TxDOTAppPlugin.h"
#include "TxDOTOptionalDesignUtilities.h"

#include "resource.h"

#include <MFCTools\VersionInfo.h>

#include <IFace\Artifact.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Allowables.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTxDOTOptionalDesignDocProxyAgent
****************************************************************************/

CTxDOTOptionalDesignDocProxyAgent::CTxDOTOptionalDesignDocProxyAgent():
m_NeedValidate(true),
m_GirderArtifact(TOGA_SPAN, TOGA_FABR_GDR)
{
   m_pTxDOTOptionalDesignDoc = NULL;
}

CTxDOTOptionalDesignDocProxyAgent::~CTxDOTOptionalDesignDocProxyAgent()
{
}

void CTxDOTOptionalDesignDocProxyAgent::SetDocument(CTxDOTOptionalDesignDoc* pDoc)
{
   m_pTxDOTOptionalDesignDoc = pDoc;

   pDoc->m_ProjectData.Attach(this);
}

//////////////////////////////////////////////////////////
// IAgentEx
STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2> pBrokerInit(m_pBroker);
   pBrokerInit->RegInterface( IID_IUpdateTemplates, this );
   pBrokerInit->RegInterface( IID_ISelection,       this );
   pBrokerInit->RegInterface( IID_IVersionInfo,     this );
   pBrokerInit->RegInterface( IID_IGetTogaData,     this );
   pBrokerInit->RegInterface( IID_IGetTogaResults,  this );
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::Init()
{
//   AGENT_INIT;

   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::Init2()
{
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::Reset()
{
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::ShutDown()
{
//   CLOSE_LOGFILE;

   AGENT_CLEAR_INTERFACE_CACHE;

   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_TxDOTOptionalDesignDocProxyAgent;
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// IUpdateTemplates
bool CTxDOTOptionalDesignDocProxyAgent::UpdatingTemplates()
{
   // By returning true here we are forcing local library entries to be overwritten by
   // master library entries if there is a conflict. 
   // This behavior is by design so if a library update occurs, new data will be used.
   return true;
}

////////////////////////////////////////////////////////////////////
// ISelection

// NOTE: Not really utilizing this interface - just appealing functions that need it.

PierIndexType CTxDOTOptionalDesignDocProxyAgent::GetPierIdx()
{
   return TOGA_SPAN;
}

SpanIndexType CTxDOTOptionalDesignDocProxyAgent::GetSpanIdx()
{
   return TOGA_SPAN;
}

GirderIndexType CTxDOTOptionalDesignDocProxyAgent::GetGirderIdx()
{
   return TOGA_FABR_GDR;
}

void CTxDOTOptionalDesignDocProxyAgent::SelectPier(PierIndexType pierIdx)
{
   ASSERT(0);
}

void CTxDOTOptionalDesignDocProxyAgent::SelectSpan(SpanIndexType spanIdx)
{
   ASSERT(0);
}

void CTxDOTOptionalDesignDocProxyAgent::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   ASSERT(0);
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetSectionCutStation()
{
   ASSERT(false); // should never get here
   return -999999;
}

////////////////////////////////////////////////////////////////////
// IVersionInfo
CString CTxDOTOptionalDesignDocProxyAgent::GetVersionString(bool bIncludeBuildNumber)
{
   CString str("Version ");
   str += GetVersion(bIncludeBuildNumber);
#if defined _BETA_VERSION
   str += CString(" BETA");
#endif

   str += CString(" - Built on ");
   str += CString(__DATE__);
   return str;
}

CString CTxDOTOptionalDesignDocProxyAgent::GetVersion(bool bIncludeBuildNumber)
{
   // This plug-in is like an application... we want the version information for this DLL
   // set the module state to this DLL
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CWinApp* pApp = AfxGetApp();
   CString strExe( pApp->m_pszExeName );
   strExe += ".dll";

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   
   CString strVersion = verInfo.GetProductVersionAsString();

#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif

   if (!bIncludeBuildNumber)
   {
      // remove the build number
      int pos = strVersion.ReverseFind('.'); // find the last '.'
      strVersion = strVersion.Left(pos);
   }

   return strVersion;
}

///////////////////////////////////////////////////////////////////
// IGetTogaData
const CTxDOTOptionalDesignData* CTxDOTOptionalDesignDocProxyAgent::GetTogaData()
{
   return &(this->m_pTxDOTOptionalDesignDoc->m_ProjectData);
}

///////////////////////////////////////////////////////////////////
// IGetTogaResults
void CTxDOTOptionalDesignDocProxyAgent::GetControllingTensileStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart)
{
   Validate();

   *pStress        = m_CtrlTensileStress;
   *pDistFromStart = m_CtrlTensileStressLocation;
   *pStressFactor  = m_CtrlTensileStressFactor;
}

void CTxDOTOptionalDesignDocProxyAgent::GetControllingCompressiveStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart)
{
   Validate();

   *pStress        = m_CtrlCompressiveStress;
   *pDistFromStart = m_CtrlCompressiveStressLocation;
   *pStressFactor  = m_CtrlCompressiveStressFactor;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetRequiredUltimateMoment()
{
   Validate();

   return m_RequiredUltimateMoment;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetUltimateMomentCapacity()
{
   Validate();

   return m_UltimateMomentCapacity;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetMaximumCamber()
{
   Validate();
   
   return m_MaximumCamber;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetFabricatorMaximumCamber()
{
   Validate();
   
   return m_FabricatorMaximumCamber;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetRequiredFc()
{
   Validate();

   return m_RequiredFc;
}

bool CTxDOTOptionalDesignDocProxyAgent::ShearPassed()
{
   Validate();

   return m_ShearPassed;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetRequiredFci()
{
   Validate();

   return m_RequiredFci;
}

const pgsGirderArtifact* CTxDOTOptionalDesignDocProxyAgent::GetFabricatorDesignArtifact()
{
   Validate();

   return &m_GirderArtifact;
}

void CTxDOTOptionalDesignDocProxyAgent::OnTxDotDataChanged(int change)
{
   // any change type affects us
   m_NeedValidate = true;
}

void CTxDOTOptionalDesignDocProxyAgent::Validate()
{
   if ( m_NeedValidate )
   {
      // build model
      IBroker* pBroker = this->m_pTxDOTOptionalDesignDoc->GetUpdatedBroker();

      GET_IFACE2(pBroker,IAllowableConcreteStress, pAllowable );

      // Get responses from design based on original data
      GET_IFACE2(pBroker,IArtifact,pIArtifact);
      const pgsGirderArtifact* pOriginalGdrArtifact = pIArtifact->GetArtifact(TOGA_SPAN, TOGA_ORIG_GDR);

      const CTxDOTOptionalDesignData* pDesignData = GetTogaData();

      // Our model is always prismatic - max's will occur at mid-span poi
      GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
      std::vector<pgsPointOfInterest> vPOI = pIPoi->GetPointsOfInterest(TOGA_SPAN, TOGA_ORIG_GDR, pgsTypes::BridgeSite3,POI_MIDSPAN);
      ATLASSERT( vPOI.size() == 1 );
      const pgsPointOfInterest orig_ms_poi = vPOI.front();

      // Find max compressive stress and compute stress factor from original model
      const pgsFlexuralStressArtifact* pOriginalStressArtifact;
      pOriginalStressArtifact = pOriginalGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceI,pgsTypes::Compression,orig_ms_poi.GetDistFromStart()));

      Float64 fTop, fBot;
      pOriginalStressArtifact->GetExternalEffects( &fTop, &fBot );

      m_CtrlCompressiveStress = fTop;
      m_CtrlCompressiveStressLocation = orig_ms_poi.GetDistFromStart();

      Float64 ft_des = -1.0 * pDesignData->GetFt(); // opposite sign convention
      m_CtrlCompressiveStressFactor = ft_des/m_CtrlCompressiveStress;

      // Tensile stress factor
      pOriginalStressArtifact = pOriginalGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,orig_ms_poi.GetDistFromStart()));
      pOriginalStressArtifact->GetExternalEffects( &fTop, &fBot );

      m_CtrlTensileStress = fBot;
      m_CtrlTensileStressLocation = orig_ms_poi.GetDistFromStart();

      Float64 fb_des = -1.0 * pDesignData->GetFb(); // opposite sign convention
      m_CtrlTensileStressFactor = fb_des/m_CtrlTensileStress;

      // Camber from original model
      GET_IFACE2(pBroker,ICamber,pCamber);
      m_MaximumCamber = pCamber->GetDCamberForGirderSchedule(orig_ms_poi,CREEP_MAXTIME);

      // Now we need results from fabricator model
      // =========================================
      const pgsGirderArtifact* pFabricatorGdrArtifact = pIArtifact->GetArtifact(TOGA_SPAN, TOGA_FABR_GDR);

      // Create a copy of PGSuper's artifact and factor stresses in our new copy
      m_GirderArtifact = *pFabricatorGdrArtifact;

      // List of possible stress checks
      const int num_cases = 7;
      pgsTypes::Stage       stages[num_cases] = {pgsTypes::BridgeSite1,pgsTypes::BridgeSite1,pgsTypes::BridgeSite2, pgsTypes::BridgeSite3, pgsTypes::BridgeSite3, pgsTypes::BridgeSite3, pgsTypes::BridgeSite3};
      pgsTypes::LimitState lstates[num_cases] = {pgsTypes::ServiceI,   pgsTypes::ServiceI,   pgsTypes::ServiceI,    pgsTypes::ServiceI   , pgsTypes::ServiceIII,  pgsTypes::FatigueI,    pgsTypes::ServiceIA};
      pgsTypes::StressType ststype[num_cases] = {pgsTypes::Tension,    pgsTypes::Compression,pgsTypes::Compression, pgsTypes::Compression, pgsTypes::Tension,     pgsTypes::Compression, pgsTypes::Compression};

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         lstates[num_cases-1] = pgsTypes::FatigueI;

      // Loop over all pois and limit states and factor results in our copy
      pgsFlexuralStressArtifact* pFabrStressArtifact;

      for (int icase=0; icase<num_cases; icase++)
      {
         vPOI = pIPoi->GetPointsOfInterest( TOGA_SPAN, TOGA_FABR_GDR, stages[icase], 
                                            POI_FLEXURESTRESS | POI_TABULAR );
         CHECK(vPOI.size()>0);

         if ( (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims && lstates[icase] == pgsTypes::FatigueI) || 
              (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion()&& lstates[icase] == pgsTypes::ServiceIA)
              )
         {
            // if before LRFD 2009 and Fatigue I 
            // - OR -
            // LRFD 2009 and later and Service IA
            //
            // ... don't evaluate this case
            continue;
         }

         Float64 k;
         if (lstates[icase] == pgsTypes::ServiceIA || lstates[icase] == pgsTypes::FatigueI )
            k = 0.5; // Use half prestress stress if service IA (See Tbl 5.9.4.2.1-1)
         else
            k = 1.0;

         std::vector<pgsPointOfInterest>::const_iterator iter;
         for ( iter = vPOI.begin(); iter != vPOI.end(); iter++ )
         {
            const pgsPointOfInterest& poi = *iter;

            pFabrStressArtifact = m_GirderArtifact.GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stages[icase],lstates[icase],ststype[icase],poi.GetDistFromStart()));

            // factor external stresses
            Float64 fTopExt, fBotExt;
            pFabrStressArtifact->GetExternalEffects( &fTopExt, &fBotExt );

            fTopExt *= m_CtrlCompressiveStressFactor;
            fBotExt *= m_CtrlTensileStressFactor;

            pFabrStressArtifact->SetExternalEffects( fTopExt, fBotExt );

            // recompute demand
            Float64 fTopPs, fBotPs;
            pFabrStressArtifact->GetPrestressEffects( &fTopPs, &fBotPs );

            fTop = k*fTopPs + fTopExt;
            fBot = k*fBotPs + fBotExt;

            pFabrStressArtifact->SetDemand(fTop, fBot);

            // Compute and store required concrete strength
            if ( ststype[icase] == pgsTypes::Compression )
            {
               double c = pAllowable->GetAllowableCompressiveStressCoefficient(stages[icase],lstates[icase]);
               double fc_reqd = (IsZero(c) ? 0 : _cpp_min(fTop,fBot)/-c);
               
               if ( fc_reqd < 0 ) // the minimum stress is tensile so compression isn't an issue
                  fc_reqd = 0;

               pFabrStressArtifact->SetRequiredConcreteStrength(fc_reqd);
            }
            else
            {
               double t;
               bool bCheckMax;
               double fmax;

               pAllowable->GetAllowableTensionStressCoefficient(stages[icase],lstates[icase],&t,&bCheckMax,&fmax);

               // if this is bridge site 3, only look at the bottom stress (stress in the precompressed tensile zone)
               // otherwise, take the controlling tension
               double f = (stages[icase] == pgsTypes::BridgeSite3 ? fBot : _cpp_max(fTop,fBot));

               double fc_reqd;
               if (f>0.0)
               {
                  fc_reqd = (IsZero(t) ? 0 : BinarySign(f)*pow(f/t,2));
               }
               else
               {
                  // the maximum stress is compressive so tension isn't an issue
                  fc_reqd = 0;
               }

               if ( bCheckMax &&                  // allowable stress is limited -AND-
                    (0 < fc_reqd) &&              // there is a concrete strength that might work -AND-
                    (pow(fmax/t,2) < fc_reqd) )   // that strength will exceed the max limit on allowable
               {
                  // then that concrete strength wont really work afterall
                  if ( stages[icase] == pgsTypes::CastingYard )
                  {
                     // unless we are in the casting yard, then we can add some additional rebar
                     // and go to a higher limit
                     double talt = pAllowable->GetCastingYardAllowableTensionStressCoefficientWithRebar();
                     fc_reqd = pow(f/talt,2);
                  }
                  else
                  {
                     // too bad... this isn't going to work
                     fc_reqd = -1;
                  }
               }
               pFabrStressArtifact->SetRequiredConcreteStrength(fc_reqd);
            }

         }
      }
   
      // mid span in fab model
      vPOI = pIPoi->GetPointsOfInterest(TOGA_SPAN, TOGA_FABR_GDR, pgsTypes::BridgeSite3,POI_MIDSPAN);
      ATLASSERT( vPOI.size() == 1 );
      const pgsPointOfInterest fabr_ms_poi = vPOI.front();

      // Ultimate Moment
      const pgsFlexuralCapacityArtifact* pFabCap;
      pFabCap = m_GirderArtifact.GetPositiveMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,fabr_ms_poi.GetDistFromStart()));

      m_UltimateMomentCapacity = pFabCap->GetCapacity();

      // Required from original model
      const pgsFlexuralCapacityArtifact* pOrigCap;
      pOrigCap = pOriginalGdrArtifact->GetPositiveMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,orig_ms_poi.GetDistFromStart()));

      m_RequiredUltimateMoment = _cpp_max(pOrigCap->GetDemand(),pOrigCap->GetMinCapacity());

      // Required concrete strengths - fabricator model
      m_RequiredFci =  _cpp_max(m_GirderArtifact.GetRequiredReleaseStrength(),  ::ConvertToSysUnits(4.0, unitMeasure::KSI));
      m_RequiredFc  =  _cpp_max(m_GirderArtifact.GetRequiredConcreteStrength(), ::ConvertToSysUnits(5.0, unitMeasure::KSI));

      // Get camber from fabricator model
      m_FabricatorMaximumCamber = pCamber->GetDCamberForGirderSchedule(fabr_ms_poi,CREEP_MAXTIME);

      // Shear 
      CheckShear(pIPoi);

      m_NeedValidate = false;
   }
}

void CTxDOTOptionalDesignDocProxyAgent::CheckShear(IPointOfInterest* pIPoi)
{
   m_ShearPassed = true; // until otherwise

   const pgsStirrupCheckArtifact *pStirrups = m_GirderArtifact.GetStirrupCheckArtifact();

   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest(TOGA_SPAN, TOGA_FABR_GDR, pgsTypes::BridgeSite3, POI_SHEAR|POI_TABULAR);

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      // Only checking Strength I here. No TxDOT permit truck
      const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,poi.GetDistFromStart()) );

      const pgsVerticalShearArtifact* pShear = pPoiArtifacts->GetVerticalShearArtifact();
      if (!pShear->Passed())
      {
         m_ShearPassed = false;
         return;
      }

      const pgsLongReinfShearArtifact* pLongReinf = pPoiArtifacts->GetLongReinfShearArtifact();
      if (!pLongReinf->Passed() )
      {
         m_ShearPassed = false;
         return;
      }

      const pgsHorizontalShearArtifact* pHor = pPoiArtifacts->GetHorizontalShearArtifact();
      if ( !pHor->Passed() )
      {
         m_ShearPassed = false;
         return;
      }

      const pgsStirrupDetailArtifact* pDet = pPoiArtifacts->GetStirrupDetailArtifact();
      if ( !pDet->Passed() )
      {
         m_ShearPassed = false;
         return;
      }
   }
}
