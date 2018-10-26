// CurvelImporter.cpp : Implementation of CCurvelImporter
#include "stdafx.h"
#include "WSDOTAgentImp.h"
#include "CurvelImporter.h"

#include <EAF\EAFAutoProgress.h>
#include <IFace\Project.h>

#include <Curvel.h>

HRESULT CCurvelImporter::FinalConstruct()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_Bitmap.LoadBitmap(IDB_CURVEL));
   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CCurvelImporter
STDMETHODIMP CCurvelImporter::Init(UINT nCmdID)
{
   return S_OK;
}

STDMETHODIMP CCurvelImporter::GetMenuText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("BEToolbox:Curve vertical profile model");
   return S_OK;
}

STDMETHODIMP CCurvelImporter::GetBitmapHandle(HBITMAP* phBmp)
{
   *phBmp = m_Bitmap;
   return S_OK;
}

STDMETHODIMP CCurvelImporter::GetCommandHintText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("Import BEToolbox:Curvel model\nTool tip text");
   return S_OK;   
}

STDMETHODIMP CCurvelImporter::Import(IBroker* pBroker)
{
	CFileDialog  fileDlg(TRUE,_T("curvel"),_T(""),OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Curvel File (*.curvel)|*.curvel||"));
	if (fileDlg.DoModal() == IDOK)
   {
      CString strFileName(fileDlg.GetPathName());
      // PGSuper is using the old C++ units management system... we need the new C++/COM version
      CComPtr<IUnitServer> unitServer;
      unitServer.CoCreateInstance(CLSID_UnitServer);
      unitServer->SetBaseUnits(CComBSTR(unitSysUnitsMgr::GetMassUnit().UnitTag().c_str()),
                               CComBSTR(unitSysUnitsMgr::GetLengthUnit().UnitTag().c_str()),
                               CComBSTR(unitSysUnitsMgr::GetTimeUnit().UnitTag().c_str()),
                               CComBSTR(unitSysUnitsMgr::GetTemperatureUnit().UnitTag().c_str()),
                               CComBSTR(unitSysUnitsMgr::GetAngleUnit().UnitTag().c_str()));
      CComQIPtr<IUnitConvert> convert(unitServer);
      std::auto_ptr<Curvel> curvelXML(CreateCurvelModel(strFileName,convert));
      if ( curvelXML.get() == NULL )
      {
         return E_FAIL;
      }

      VerticalCurveDataType& vc = curvelXML->VerticalCurveData();

      ProfileData2 profileData;
      profileData.Station = vc.PVIStation();
      profileData.Elevation = vc.PVIElevation();
      profileData.Grade = vc.g1();

      VertCurveData vcd;
      vcd.PVIStation = profileData.Station;
      vcd.ExitGrade = vc.g2();
      vcd.L1 = vc.Length();
      vcd.L2 = 0;
      profileData.VertCurves.push_back(vcd);

      Curvel::SuperelevationData_optional& super = curvelXML->SuperelevationData();
      bool bCrossSectionData = super.present();
      RoadwaySectionData sectionData;
      if ( bCrossSectionData )
      {
         Float64 cpo = super->ProfileGradeOffset();

         for ( int i = 0; i < 3; i++ )
         {
            CrownSlopeType& crownSlope = super->CrownSlope();
            CrownSlopeType::SuperelevationProfilePoint_sequence& superPP = crownSlope.SuperelevationProfilePoint();
            ATLASSERT(superPP.size() == 3);

            CrownData2 crown;
            crown.Station  = superPP[i].Station();
            crown.Left     = superPP[i].LeftSlope();
            crown.Right    = superPP[i].RightSlope();
            crown.CrownPointOffset = cpo;

            sectionData.Superelevations.push_back(crown);
         }
      }

      GET_IFACE2(pBroker,IEvents,pEvents);
      pEvents->HoldEvents();

      GET_IFACE2(pBroker,IRoadwayData,pRoadway);
      pRoadway->SetProfileData2(profileData);

      if ( bCrossSectionData )
         pRoadway->SetRoadwaySectionData(sectionData);

      pEvents->FirePendingEvents();

      CString strMsg;
      strMsg.Format(_T("Profile and Cross Section data was successfully imported from %s\n\nCurvel data does not include horizontal alignment information. Verify your alignment data is correct."),fileDlg.GetPathName());
      AfxMessageBox(strMsg,MB_OK | MB_ICONINFORMATION);
   }
   return S_OK;
}
