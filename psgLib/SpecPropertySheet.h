#pragma once


#include "psgLib\SpecLibraryEntry.h"

#include "SpecGeneralDescriptionPropertyPage.h"
#include "SpecGeneralSpecificationPropertyPage.h"
#include "SpecGeneralSectionPropertiesPropertyPage.h"

#include "SpecDesignReleasePropertyPage.h"
#include "SpecDesignLiftingPropertyPage.h"
#include "SpecDesignHaulingPropertyPage.h"
#include "SpecDesignFinalPropertyPage.h"
#include "SpecDesignStrandsPropertyPage.h"
#include "SpecDesignRoadwayPropertyPage.h"
#include "SpecDesignConcretePropertyPage.h"

#include "SpecGirderTemporaryPropertyPage.h"
#include "SpecGirderFinalPropertyPage.h"
#include "SpecGirderFatiguePropertyPage.h"
#include "SpecGirderPrincipalStressPropertyPage.h"

#include "SpecClosureTemporaryPropertyPage.h"
#include "SpecClosureFinalPropertyPage.h"
#include "SpecClosureFatiguePropertyPage.h"

#include "SpecPrestressingPretensionLimitsPropertyPage.h"
#include "SpecPrestressingPostTensionLimitsPropertyPage.h"
#include "SpecPrestressingOptionsPropertyPage.h"
#include "SpecPrestressingDuctsPropertyPage.h"

#include "SpecLiftingFactorsOfSafetyPropertyPage.h"
#include "SpecLiftingModulusOfRupturePropertyPage.h"
#include "SpecLiftingAnalysisPropertyPage.h"
#include "SpecLiftingLimitsPropertyPage.h"

#include "SpecHaulingMethodPropertyPage.h"
#include "SpecHaulingKDOTPropertyPage.h"
#include "SpecHaulingFactorsOfSafetyPropertyPage.h"
#include "SpecHaulingModulusOfRupturePropertyPage.h"
#include "SpecHaulingAnalysisPropertyPage.h"
#include "SpecHaulingLimitsPropertyPage.h"

#include "SpecDeadLoadRailingPropertyPage.h"
#include "SpecDeadLoadOverlayPropertyPage.h"

#include "SpecLiveLoadHL93PropertyPage.h"
#include "SpecLiveLoadDistributionFactorsPropertyPage.h"
#include "SpecLiveLoadPedestrianPropertyPage.h"

#include "SpecMomentCapacityPropertyPage.h"
#include "SpecMomentResistancefactorsPropertyPage.h"

#include "SpecShearCapacityPropertyPage.h"
#include "SpecShearResistancefactorsPropertyPage.h"
#include "SpecShearReinforcementPropertyPage.h"
#include "SpecShearInterfaceShearPropertyPage.h"

#include "SpecCreepGeneralPropertyPage.h"
#include "SpecCreepExcessCamberPropertyPage.h"

#include "SpecLossesPropertyPage.h"

#include "SpecLimitsWarningsPropertyPage.h"
#include "SpecLimitsConcretePropertyPage.h"

// Don't allow design if check is disabled.
void CheckDesignCtrl(int idc, int idd, int list[], CWnd* pme);

// CSpecPropertySheet
class CSpecPropertySheet : public CMFCPropertySheet
{
   DECLARE_DYNAMIC(CSpecPropertySheet)

public:
   CSpecPropertySheet(SpecLibraryEntry& rentry, UINT nIDCaption,
      bool allowEditing,
      CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
   CSpecPropertySheet(SpecLibraryEntry& rentry, LPCTSTR pszCaption,
      bool allowEditing,
      CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
   virtual ~CSpecPropertySheet();

   bool m_bAllowEditing;
   SpecLibraryEntry& m_Entry;
   CString m_Name;
   CString m_Description;

protected:
   DECLARE_MESSAGE_MAP()

   void Init();

   CSpecGeneralDescriptionPropertyPage m_General_Description;
   CSpecGeneralSpecificationPropertyPage m_General_Specification;
   CSpecGeneralSectionPropertiesPropertyPage m_General_SectionProperties;

   CSpecDesignReleasePropertyPage m_Design_Release;
   CSpecDesignLiftingPropertyPage m_Design_Lifting;
   CSpecDesignHaulingPropertyPage m_Design_Hauling;
   CSpecDesignFinalPropertyPage m_Design_Final;
   CSpecDesignStrandsPropertyPage m_Design_Strands;
   CSpecDesignRoadwayPropertyPage m_Design_Roadway;
   CSpecDesignConcretePropertyPage m_Design_Concrete;

   CSpecGirderTemporaryPropertyPage m_Girder_Temporary;
   CSpecGirderFinalPropertyPage m_Girder_Final;
   CSpecGirderFatiguePropertyPage m_Girder_Fatigue;
   CSpecGirderPrincipalStressPropertyPage m_Girder_PrincipalStress;

   CSpecClosureTemporaryPropertyPage m_Closure_Temporary;
   CSpecClosureFinalPropertyPage m_Closure_Final;
   CSpecClosureFatiguePropertyPage m_Closure_Fatigue;

   CSpecPrestressingPretensionLimitsPropertyPage m_Prestressing_PSLimits;
   CSpecPrestressingPostTensionLimitsPropertyPage m_Prestressing_PTLimits;
   CSpecPrestressingOptionsPropertyPage m_Prestressing_Options;
   CSpecPrestressingDuctsPropertyPage m_Prestressing_Ducts;

   CSpecLiftingFactorsOfSafetyPropertyPage m_Lifting_FactorsOfSafety;
   CSpecLiftingModulusOfRupturePropertyPage m_Lifting_ModulusOfRupture;
   CSpecLiftingAnalysisPropertyPage m_Lifting_Analysis;
   CSpecLiftingLimitsPropertyPage m_Lifting_Limits;

   CSpecHaulingMethodPropertyPage m_Hauling_Method;
   CSpecHaulingKDOTPropertyPage m_Hauling_KDOT;
   CSpecHaulingFactorsOfSafetyPropertyPage m_Hauling_FactorsOfSafety;
   CSpecHaulingModulusOfRupturePropertyPage m_Hauling_ModulusOfRupture;
   CSpecHaulingAnalysisPropertyPage m_Hauling_Analysis;
   CSpecHaulingLimitsPropertyPage m_Hauling_Limits;

   CSpecDeadLoadRailingPropertyPage m_DeadLoad_Railing;
   CSpecDeadLoadOverlayPropertyPage m_DeadLoad_Overlay;
   CMFCPropertyPage m_DeadLoad_Haunch;

   CSpecLiveLoadHL93PropertyPage m_LiveLoad_HL93;
   CSpecLiveLoadDistributionFactorsPropertyPage  m_LiveLoad_DistributionFactors;
   CSpecLiveLoadPedestrianPropertyPage m_LiveLoad_Pedestrian;

   CSpecMomentCapacityPropertyPage m_Moment_Capacity;
   CSpecMomentResistanceFactorsPropertyPage m_Moment_ResistanceFactors;

   CSpecShearCapacityPropertyPage m_Shear_Capacity;
   CSpecShearResistanceFactorsPropertyPage m_Shear_ResistanceFactors;
   CSpecShearReinforcementPropertyPage m_Shear_Reinforcement;
   CSpecShearInterfaceShearPropertyPage m_Shear_InterfaceShear;

   CSpecCreepGeneralPropertyPage m_Creep_General;
   CSpecCreepExcessCamberPropertyPage m_Creep_ExcessCamber;

   CSpecLossesPropertyPage m_Losses;

   CSpecLimitsWarningsPropertyPage m_Limits_Warnings;
   CSpecLimitsConcretePropertyPage m_Limits_Concrete;

   CMFCPropertySheetCategoryInfo* m_pHauling;
   CMFCPropertySheetCategoryInfo* m_pWSDOTHauling;

public:
   virtual BOOL OnInitDialog();

   // our own function for mapping code reference changes from 2017 LRFD Crosswalk
   LPCTSTR LrfdCw8th(LPCTSTR oldStr, LPCTSTR newStr)
   {
      return (GetSpecVersion() < WBFL::LRFD::BDSManager::Edition::EighthEdition2017) ? oldStr : newStr;
   }

   WBFL::LRFD::BDSManager::Edition GetSpecVersion();
   void CheckShearCapacityMethod();

   void ExchangeGeneralDescriptionData(CDataExchange* pDX);
   void ExchangeGeneralSpecificationData(CDataExchange* pDX);
   void ExchangeGeneralSectionPropertiesData(CDataExchange* pDX);

   void ExchangeDesignReleaseData(CDataExchange* pDX);
   void ExchangeDesignLiftingData(CDataExchange* pDX);
   void ExchangeDesignHaulingData(CDataExchange* pDX);
   void ExchangeDesignFinalData(CDataExchange* pDX);
   void ExchangeDesignStrandsData(CDataExchange* pDX);
   void ExchangeDesignRoadwayData(CDataExchange* pDX);
   void ExchangeDesignConcreteData(CDataExchange* pDX);

   void ExchangeGirderTemporaryData(CDataExchange* pDX);
   void ExchangeGirderFinalData(CDataExchange* pDX);
   void ExchangeGirderFatigueData(CDataExchange* pDX);
   void ExchangeGirderPrincipalStressData(CDataExchange* pDX);

   void ExchangeClosureTemporaryData(CDataExchange* pDX);
   void ExchangeClosureFinalData(CDataExchange* pDX);
   void ExchangeClosureFatigueData(CDataExchange* pDX);

   void ExchangePrestressLimitsPretensionData(CDataExchange* pDX);
   void ExchangePrestressLimitsPostTensionData(CDataExchange* pDX);
   void ExchangePrestressLimitsOptionsData(CDataExchange* pDX);
   void ExchangePrestressLimitsDuctsData(CDataExchange* pDX);

   void ExchangeLiftingFactorsOfSafetyData(CDataExchange* pDX);
   void ExchangeLiftingModulusOfRuptureData(CDataExchange* pDX);
   void ExchangeLiftingAnalysisData(CDataExchange* pDX);
   void ExchangeLiftingLimitsData(CDataExchange* pDX);

   void ExchangeHaulingMethodData(CDataExchange* pDX);
   void ExchangeHaulingKDOTData(CDataExchange* pDX);
   void ExchangeHaulingFactorsOfSafetyData(CDataExchange* pDX);
   void ExchangeHaulingModulusOfRuptureData(CDataExchange* pDX);
   void ExchangeHaulingAnalysisData(CDataExchange* pDX);
   void ExchangeHaulingLimitsData(CDataExchange* pDX);

   void ExchangeDeadLoadRailingData(CDataExchange* pDX);
   void ExchangeDeadLoadOverlayData(CDataExchange* pDX);

   void ExchangeLiveLoadHL93Data(CDataExchange* pDX);
   void ExchangeLiveLoadDistributionFactorsData(CDataExchange* pDX);
   void ExchangeLiveLoadPedestrianData(CDataExchange* pDX);

   void ExchangeMomentCapacityData(CDataExchange* pDX);
   void ExchangeMomentResistanceFactorsData(CDataExchange* pDX);

   void ExchangeShearCapacityData(CDataExchange* pDX);
   void ExchangeShearResistanceFactorsData(CDataExchange* pDX);
   void ExchangeShearReinforcementData(CDataExchange* pDX);
   void ExchangeShearInterfaceData(CDataExchange* pDX);

   void ExchangeCreepGeneralData(CDataExchange* pDX);
   void ExchangeCreepExcessCamberData(CDataExchange* pDX);

   void ExchangeLossesData(CDataExchange* pDX);

   void ExchangeLimitsWarningsData(CDataExchange* pDX);
   void ExchangeLimitsConcreteData(CDataExchange* pDX);
};