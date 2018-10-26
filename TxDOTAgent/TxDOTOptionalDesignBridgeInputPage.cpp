// TxDOTOptionalDesignBridgeInputPage.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignBridgeInputPage.h"

#include "TxDOTOptionalDesignUtilities.h"

#include <MfcTools\CustomDDX.h>
#include <EAF\EAFDisplayUnits.h>


// CTxDOTOptionalDesignBridgeInputPage dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignBridgeInputPage, CPropertyPage)

CTxDOTOptionalDesignBridgeInputPage::CTxDOTOptionalDesignBridgeInputPage()
	: CPropertyPage(CTxDOTOptionalDesignBridgeInputPage::IDD),
   m_pBrokerRetriever(NULL),
   m_pData(NULL)
{
}

CTxDOTOptionalDesignBridgeInputPage::~CTxDOTOptionalDesignBridgeInputPage()
{
}

void CTxDOTOptionalDesignBridgeInputPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetClassicBroker();
   if (pBroker==NULL)
      return;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_Text(pDX, IDC_BRIDGE, m_Bridge);
   DDX_Text(pDX, IDC_BRIDGE_ID, m_BridgeID);
   DDX_Text(pDX, IDC_JOB_NUMBER, m_JobNumber);
   DDX_Text(pDX, IDC_ENGINEER, m_Engineer);
   DDX_Text(pDX, IDC_COMPANY, m_Company);
   DDX_Text(pDX, IDC_COMMENTS, m_Comments);

   if (!(!pDX->m_bSaveAndValidate && m_SpanNo==-1)) // don't fill bogus values
   {
      DDX_Text(pDX, IDC_SPAN_NO, m_SpanNo);
      DDV_MinMaxInt(pDX, m_SpanNo, 1, 999);
   }

   if (!(!pDX->m_bSaveAndValidate && m_BeamNo==-1)) // don't fill bogus values
   {
      DDX_Text(pDX, IDC_BEAM_NO, m_BeamNo);
      DDV_MinMaxInt(pDX, m_BeamNo, 1, 999);
   }

   DDX_CBString(pDX, IDC_BEAM_TYPE, m_BeamType);

   DDX_UnitValueAndTag( pDX, IDC_BEAM_SPACING,   IDC_BEAM_SPACING_UNITS,  m_BeamSpacing, pDisplayUnits->GetSpanLengthUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_BEAM_SPACING,m_BeamSpacing, pDisplayUnits->GetSpanLengthUnit() );

   Float64 min_span_length = ::ConvertToSysUnits(20.0, unitMeasure::Feet);

   DDX_UnitValueAndTag( pDX, IDC_SPAN_LENGTH,   IDC_SPAN_LENGTH_UNITS,  m_SpanLength, pDisplayUnits->GetSpanLengthUnit() );
   DDV_UnitValueLimitOrMore( pDX, IDC_SPAN_LENGTH,m_SpanLength, min_span_length, pDisplayUnits->GetSpanLengthUnit() );

   Float64 min_slab_thickness = ::ConvertToSysUnits(4.0,  unitMeasure::Inch);
   Float64 max_slab_thickness = ::ConvertToSysUnits(24.0, unitMeasure::Inch);

   DDX_UnitValueAndTag( pDX, IDC_SLAB_THICKNESS,   IDC_SLAB_THICKNESS_UNITS,  m_SlabThickness, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueRange( pDX, IDC_SLAB_THICKNESS,m_SlabThickness,min_slab_thickness, max_slab_thickness, pDisplayUnits->GetComponentDimUnit() );

   if (!(!pDX->m_bSaveAndValidate && m_RelativeHumidity==Float64_Inf)) // don't fill bogus values
   {
      DDX_Text(pDX, IDC_RELATIVE_HUMIDITY, m_RelativeHumidity);
      DDV_MinMaxDouble(pDX, m_RelativeHumidity, 30., 100.);
   }

   if (!(!pDX->m_bSaveAndValidate && m_LldfMoment==Float64_Inf)) // don't fill bogus values
   {
      DDX_Text(pDX, IDC_LLDF_MOMENT, m_LldfMoment);
	   DDV_MinMaxDouble(pDX, m_LldfMoment, 0.0, 999);
   }

   if (!(!pDX->m_bSaveAndValidate && m_LldfShear==Float64_Inf)) // don't fill bogus values
   {
      DDX_Text(pDX, IDC_LLDF_SHEAR, m_LldfShear);
	   DDV_MinMaxDouble(pDX, m_LldfShear, 0.0, 999);
   }

   Float64 min_ec  = ::ConvertToSysUnits( 2000.0,  unitMeasure::KSI); 
   Float64 max_ec  = ::ConvertToSysUnits(10000.0,  unitMeasure::KSI); 

   DDX_UnitValueAndTag(pDX,IDC_EC_SLAB, IDC_EC_SLAB_UNITS, m_EcSlab, pDisplayUnits->GetStressUnit() );
   DDV_UnitValueRange( pDX, IDC_EC_SLAB,m_EcSlab,min_ec, max_ec, pDisplayUnits->GetStressUnit() );

   DDX_UnitValueAndTag(pDX,IDC_EC_BEAM, IDC_EC_BEAM_UNITS, m_EcBeam, pDisplayUnits->GetStressUnit() );
   DDV_UnitValueRange( pDX, IDC_EC_BEAM,m_EcBeam,min_ec, max_ec, pDisplayUnits->GetStressUnit() );

   Float64 min_fc  = ::ConvertToSysUnits( 4.0,  unitMeasure::KSI); 
   Float64 max_fci = ::ConvertToSysUnits(10.0,  unitMeasure::KSI); 
   Float64 max_fc  = ::ConvertToSysUnits(15.0,  unitMeasure::KSI); 

   DDX_UnitValueAndTag(pDX,IDC_FC_SLAB, IDC_FC_SLAB_UNITS, m_FcSlab, pDisplayUnits->GetStressUnit() );
   DDV_UnitValueRange( pDX,IDC_FC_SLAB,m_FcSlab,min_fc, max_fc, pDisplayUnits->GetStressUnit() );

   DDX_UnitValueAndTag(pDX,IDC_FT, IDC_FT_UNITS, m_Ft, pDisplayUnits->GetStressUnit() );
   DDX_UnitValueAndTag(pDX,IDC_FB, IDC_FB_UNITS, m_Fb, pDisplayUnits->GetStressUnit() );
   DDX_UnitValueAndTag(pDX,IDC_MU, IDC_MU_UNITS, m_Mu, pDisplayUnits->GetMomentUnit() );

   DDX_UnitValueAndTag( pDX, IDC_W_NONCOMP_DC,   IDC_W_NONCOMP_DC_UNITS,  m_WNonCompDc, pDisplayUnits->GetForcePerLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_W_NONCOMP_DC,m_WNonCompDc, pDisplayUnits->GetForcePerLengthUnit() );

   DDX_UnitValueAndTag( pDX, IDC_W_COMP_DC,   IDC_W_COMP_DC_UNITS,  m_WCompDc, pDisplayUnits->GetForcePerLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_W_COMP_DC,m_WCompDc, pDisplayUnits->GetForcePerLengthUnit() );

   DDX_UnitValueAndTag( pDX, IDC_W_COMP_DW,   IDC_W_COMP_DW_UNITS,  m_WCompDw, pDisplayUnits->GetForcePerLengthUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_W_COMP_DW,m_WCompDw, pDisplayUnits->GetForcePerLengthUnit() );

   if (pDX->m_bSaveAndValidate)
   {
      SaveDialogData();
   }
}


BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignBridgeInputPage, CPropertyPage)
   ON_WM_ERASEBKGND()
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CTxDOTOptionalDesignBridgeInputPage message handlers

BOOL CTxDOTOptionalDesignBridgeInputPage::OnInitDialog()
{
   // Load data into local members
   LoadDialogData();

   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTxDOTOptionalDesignBridgeInputPage::LoadDialogData()
{
   // get all of our data from data source;
   m_Bridge = m_pData->GetBridge();
   m_BridgeID = m_pData->GetBridgeID();
   m_JobNumber = m_pData->GetJobNumber();
   m_Engineer = m_pData->GetEngineer();
   m_Company = m_pData->GetCompany();
   m_Comments = m_pData->GetComments();

   m_SpanNo = m_pData->GetSpanNo();
   m_BeamNo = m_pData->GetBeamNo();

   m_BeamType = m_pData->GetBeamType();
   m_BeamSpacing = m_pData->GetBeamSpacing();
   m_SpanLength = m_pData->GetSpanLength();
   m_SlabThickness = m_pData->GetSlabThickness();
   m_RelativeHumidity = m_pData->GetRelativeHumidity();
   m_LldfMoment = m_pData->GetLldfMoment();
   m_LldfShear = m_pData->GetLldfShear();

   m_EcSlab = m_pData->GetEcSlab();
   m_EcBeam = m_pData->GetEcBeam();
   m_FcSlab = m_pData->GetFcSlab();

   m_Ft = m_pData->GetFt();
   m_Fb = m_pData->GetFb();
   m_Mu = m_pData->GetMu();

   m_WNonCompDc = m_pData->GetWNonCompDc();
   m_WCompDc = m_pData->GetWCompDc();
   m_WCompDw = m_pData->GetWCompDw();

   // load beam type dialog
   LoadGirderNames();
}

void CTxDOTOptionalDesignBridgeInputPage::SaveDialogData()
{
   // Set all of our data back to data source
   m_pData->SetBridge(m_Bridge);
   m_pData->SetBridgeID(m_BridgeID);
   m_pData->SetJobNumber(m_JobNumber);
   m_pData->SetEngineer(m_Engineer);
   m_pData->SetCompany(m_Company);
   m_pData->SetComments(m_Comments);

   m_pData->SetSpanNo(m_SpanNo);
   m_pData->SetBeamNo(m_BeamNo);

   m_pData->SetBeamType(m_BeamType);
   m_pData->SetBeamSpacing(m_BeamSpacing);
   m_pData->SetSpanLength(m_SpanLength);
   m_pData->SetSlabThickness(m_SlabThickness);
   m_pData->SetRelativeHumidity(m_RelativeHumidity);
   m_pData->SetLldfMoment(m_LldfMoment);
   m_pData->SetLldfShear(m_LldfShear);

   m_pData->SetEcSlab(m_EcSlab);
   m_pData->SetEcBeam(m_EcBeam);
   m_pData->SetFcSlab(m_FcSlab);

   m_pData->SetFt(m_Ft);
   m_pData->SetFb(m_Fb);
   m_pData->SetMu(m_Mu);

   m_pData->SetWNonCompDc(m_WNonCompDc);
   m_pData->SetWCompDc(m_WCompDc);
   m_pData->SetWCompDw(m_WCompDw);
}

void CTxDOTOptionalDesignBridgeInputPage::LoadGirderNames()
{
   // find all .togt files in our template folder
   CString suffix;
   suffix.LoadString(IDS_TEMPLATE_SUFFIX);
   CString strFileSpec = GetTOGAFolder() + CString("\\*.") + suffix;

   CFileFind template_finder;
   BOOL bMoreTemplates = template_finder.FindFile(strFileSpec);
   if (!bMoreTemplates)
   {
      CString msg;
      msg.Format("Fatal Error - could not find any Toga template files using the file spec:\n %s \n This is an installation problem.",strFileSpec);
      ::AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
      return;
   }

   CComboBox* pgdr_ctrl = (CComboBox*)GetDlgItem(IDC_BEAM_TYPE);

   // Make sure our beam type is in list. The beam type is empty for new files
   bool newfile = m_BeamType.IsEmpty();
   bool beam_type_found = false;

   while ( bMoreTemplates )
   {
      bMoreTemplates = template_finder.FindNextFile();
      CString strTemplate = template_finder.GetFileTitle();

      if (newfile)
      {
         // New files will have a blank girder name - use the first in list if this is the case
         m_BeamType = strTemplate;
         newfile = false;
         beam_type_found = true;
      }

      if (!beam_type_found)
      {
         beam_type_found = m_BeamType==strTemplate;
      }

      pgdr_ctrl->AddString(strTemplate);
   }

   if (!beam_type_found)
   {
      CString msg;
      msg.Format("ERROR!! - The beam type: \"%s\" saved in this file was not found. \n This is most likely an installation problem.\nYou may select another beam type, but proceed at your own risk.",m_BeamType);
      ::AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
   }
}
BOOL CTxDOTOptionalDesignBridgeInputPage::OnEraseBkgnd(CDC* pDC)
{
   // Set brush to dialog background color
   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   // Save old brush
   CBrush* pOldBrush = pDC->SelectObject(&backBrush);

   CRect rect;
   pDC->GetClipBox(&rect);     // Erase the area needed

   pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
       PATCOPY);
   pDC->SelectObject(pOldBrush);

   return true;
}

HBRUSH CTxDOTOptionalDesignBridgeInputPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;
}
