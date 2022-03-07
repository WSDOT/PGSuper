///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// CrownSlopePage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "CrownSlopePage.h"
#include "AlignmentDescriptionDlg.h"
#include <EAF\EAFDocument.h>

#include "PGSuperColors.h"
#include <GraphicsLib\GraphicsLib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void DrawArrowLine(CDC* pDC, long xstart, long ystart, long xend, long yend, long arrowWidth, long arrowLength)
{
   // draw shaft
   pDC->MoveTo(xstart,ystart);
   pDC->LineTo(xend,yend);

   // create a left pointing arrow in double coords and move and rotate it to our end
   gpPolygon2d arrow;
   arrow.AddPoint(gpPoint2d(0.0, arrowWidth/2));
   arrow.AddPoint(gpPoint2d(0.0, -arrowWidth/2));
   arrow.AddPoint(gpPoint2d(arrowLength, 0.0));

   gpVector2d arrowvec(gpSize2d(xend - xstart, yend - ystart));
   Float64 arrowdir = arrowvec.GetDirection();

   arrow.Rotate(gpPoint2d(0, 0), arrowdir);

   arrow.Offset(xend, yend);

   POINT points[3];
   for (long i = 0; i < 3; i++)
   {
      const gpPoint2d* pnt(arrow.GetPoint(i));
      points[i].x = (long)Round(pnt->X());
      points[i].y = (long)Round(pnt->Y());
   }

   pDC->Polygon(points, 3);
}

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopePage property page

IMPLEMENT_DYNCREATE(CCrownSlopePage, CPropertyPage)

CCrownSlopePage::CCrownSlopePage() : CPropertyPage(CCrownSlopePage::IDD), m_Grid(&m_RoadwaySectionData)
{
	//{{AFX_DATA_INIT(CCrownSlopePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CCrownSlopePage::~CCrownSlopePage()
{
}

IBroker* CCrownSlopePage::GetBroker()
{
   CAlignmentDescriptionDlg* pParent = (CAlignmentDescriptionDlg*)GetParent();
   return pParent->m_pBroker;
}

void CCrownSlopePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCrownSlopePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_CBItemData(pDX, IDC_SLOPE_MEASURE_TYPE, m_RoadwaySectionData.slopeMeasure);
   DDX_CBItemData(pDX, IDC_NUMSEGMENTS_COMBO,  m_RoadwaySectionData.NumberOfSegmentsPerSection);
   DDX_CBItemData(pDX, IDC_RIDGEPT_COMBO,  m_RoadwaySectionData.AlignmentPointIdx);
   DDX_CBItemData(pDX, IDC_PROFILE_GRADE_POINT, m_RoadwaySectionData.ProfileGradePointIdx);
	DDX_Control(pDX, IDC_VIEW_TEMPLATE_SPIN, m_SelTemplateSpinner);

   // Grid owns a pointer to our roadway data
   if ( pDX->m_bSaveAndValidate )
   {
      ROWCOL badRow;
      if (!m_Grid.IsGridDataValid(&badRow))
      {
         CString str;
         str.Format(_T("Invalid data in grid template %d"), badRow);
         AfxMessageBox(str, MB_OK | MB_ICONWARNING);
         pDX->Fail();
      }
      else if (!m_Grid.SortCrossSections(false) )
      {
         AfxMessageBox(_T("Invalid cross section parameters. Are there duplicate stations?"), MB_OK | MB_ICONWARNING);
         pDX->Fail();
      }
   }
   else
   {
      m_Grid.InitRoadwaySectionData(true);
   }
}

BEGIN_MESSAGE_MAP(CCrownSlopePage, CPropertyPage)
	//{{AFX_MSG_MAP(CCrownSlopePage)
   ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_ADD, OnAdd)
   ON_UPDATE_COMMAND_UI(IDC_ADD, &CCrownSlopePage::OnUpdateAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
   ON_UPDATE_COMMAND_UI(IDC_REMOVE, &CCrownSlopePage::OnUpdateRemove)
	ON_BN_CLICKED(IDC_SORT, OnSort)
   ON_UPDATE_COMMAND_UI(IDC_SORT, &CCrownSlopePage::OnUpdateSort)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
   ON_CBN_SELCHANGE(IDC_NUMSEGMENTS_COMBO, &CCrownSlopePage::OnCbnSelchangeNumsegmentsCombo)
   ON_UPDATE_COMMAND_UI(IDC_NUMSEGMENTS_COMBO, &CCrownSlopePage::OnUpdateAdd)
   ON_CBN_SELCHANGE(IDC_RIDGEPT_COMBO, &CCrownSlopePage::OnCbnSelchangeRidgeptCombo)
   ON_CBN_SELCHANGE(IDC_PROFILE_GRADE_POINT, &CCrownSlopePage::OnCbnSelchangeProfileGradePointCombo)
   ON_UPDATE_COMMAND_UI(IDC_RIDGEPT_COMBO, &CCrownSlopePage::OnUpdateAdd)
   ON_NOTIFY(UDN_DELTAPOS, IDC_VIEW_TEMPLATE_SPIN, &CCrownSlopePage::OnDeltaposViewTemplateSpin)
   ON_CBN_SELCHANGE(IDC_SLOPE_MEASURE_TYPE, &CCrownSlopePage::OnCbnSelchangeSlopeMeasureType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopePage message handlers

void CCrownSlopePage::OnGridTemplateClicked(IndexType templIdx)
{
   // called by grid from on click event
   m_SelectedTemplate = templIdx;

   m_SelTemplateSpinner.SetPos((int)templIdx);

   UpdateViewSpinner();
}

void CCrownSlopePage::OnAdd()
{
   if (!m_Grid.AppendRow())
   {
      return;
   }

   UpdateNumSegsCtrl();
   UpdateRidgeptData();
   UpdateProfilePointData();

   int nTemplates = GetTemplateCount();
   m_SelTemplateSpinner.SetRange(0, nTemplates-1);

   OnGridTemplateClicked(nTemplates - 1);
}

void CCrownSlopePage::OnRemove() 
{
   m_Grid.RemoveRows();

   if (m_Grid.IsGridEmpty())
   {
      // Just emptied out the grid. data was changed by grid to defaults
      UpdateNumSegsCtrl();
      UpdateRidgeptData();
      UpdateProfilePointData();
   }

   int nTemplates = GetTemplateCount();
   m_SelTemplateSpinner.SetRange(0, nTemplates-1);

   OnGridTemplateClicked(nTemplates - 1);
}

void CCrownSlopePage::OnSort() 
{
   ROWCOL badRow;
   if (m_Grid.IsGridDataValid(&badRow))
   {
      if (!m_Grid.SortCrossSections(true))
      {
            ::AfxMessageBox(_T("Each template must have a unique Station"), MB_OK | MB_ICONWARNING);
      }
      else
      {
         OnChange();
      }
   }
   else
   {
      CString msg;
      msg.Format(_T("Data error in grid template %d. Cannot sort."), badRow);
      ::AfxMessageBox(msg, MB_OK | MB_ICONWARNING);
   }
}

BOOL CCrownSlopePage::OnInitDialog() 
{
   m_SelectedTemplate = 0;

	m_Grid.SubclassDlgItem(IDC_VCURVE_GRID, this);
   m_Grid.CustomInit();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SLOPE_MEASURE_TYPE);
   int idx = pCB->AddString(_T("relative to Alignment point"));
   pCB->SetItemData(idx, (DWORD_PTR)RoadwaySectionData::RelativeToAlignmentPoint);
   idx = pCB->AddString(_T("left to right"));
   pCB->SetItemData(idx, (DWORD_PTR)RoadwaySectionData::FromLeftEdge);

   FillNumSegsCtrl();
   UpdateRidgeptData();
   UpdateProfilePointData();

   CPropertyPage::OnInitDialog();

   OnCbnSelchangeSlopeMeasureType();

   UDACCEL accel[2];
   accel[0].nInc = 1;
   accel[0].nSec = 5;
   accel[1].nInc = 5;
   accel[1].nSec = 5;
   m_SelTemplateSpinner.SetAccel(2,accel);

   int nTemplates = GetTemplateCount();
   m_SelTemplateSpinner.SetRange(0, nTemplates);
   m_SelTemplateSpinner.SetPos(0);

   UpdateViewSpinner();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCrownSlopePage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_ALIGNMENT_SUPERELEVATION );
}

void CCrownSlopePage::FillNumSegsCtrl()
{
   CComboBox* pcbRidgePts = (CComboBox*)GetDlgItem(IDC_NUMSEGMENTS_COMBO);

   pcbRidgePts->ResetContent();

   for ( IndexType is = 2; is<=12;is++)
   {
      CString strval;
      strval.Format(_T("%d"),is);
      int idx = pcbRidgePts->AddString(strval);
      pcbRidgePts->SetItemData(idx,is);
   }
}

void CCrownSlopePage::UpdateNumSegsCtrl()
{
   CComboBox* pcbRidgePts = (CComboBox*)GetDlgItem(IDC_NUMSEGMENTS_COMBO);
   int cursel = pcbRidgePts->GetCurSel();
   if (cursel == CB_ERR || cursel > m_RoadwaySectionData.NumberOfSegmentsPerSection-2)
   {
      pcbRidgePts->SetCurSel((int)m_RoadwaySectionData.NumberOfSegmentsPerSection - 2);
   }

   BOOL bEnable = m_Grid.IsGridEmpty() ? FALSE : TRUE;
   pcbRidgePts->EnableWindow(bEnable);
}

void CCrownSlopePage::UpdateRidgeptData()
{
   CComboBox* pcbRidgePts = (CComboBox*)GetDlgItem(IDC_RIDGEPT_COMBO);
   int cursel = pcbRidgePts->GetCurSel();
   int curcnt = pcbRidgePts->GetCount();

   IndexType numsegs = m_RoadwaySectionData.NumberOfSegmentsPerSection;
   IndexType numridgepts = m_RoadwaySectionData.NumberOfSegmentsPerSection-1;

   if (curcnt != numridgepts)
   {
      pcbRidgePts->ResetContent();

      for (IndexType is = 1; is <= numridgepts; is++)
      {
         CString strval;
         strval.Format(_T("%d"), is);
         int idx = pcbRidgePts->AddString(strval);
         pcbRidgePts->SetItemData(idx, is);
      }
   }

   if (cursel != CB_ERR && cursel > numridgepts - 1)
   {
      // pick a reasonable default
      m_RoadwaySectionData.AlignmentPointIdx = max(1, numridgepts / 2);
   }

   pcbRidgePts->SetCurSel((int)m_RoadwaySectionData.AlignmentPointIdx-1);

   BOOL bEnable = m_Grid.IsGridEmpty() ? FALSE : TRUE;
   pcbRidgePts->EnableWindow(bEnable);
}

void CCrownSlopePage::UpdateProfilePointData()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_PROFILE_GRADE_POINT);
   int cursel = pCB->GetCurSel();
   int curcnt = pCB->GetCount();

   IndexType numsegs = m_RoadwaySectionData.NumberOfSegmentsPerSection;
   IndexType numridgepts = m_RoadwaySectionData.NumberOfSegmentsPerSection - 1;

   if (curcnt != numridgepts)
   {
      pCB->ResetContent();

      for (IndexType is = 1; is <= numridgepts; is++)
      {
         CString strval;
         strval.Format(_T("%d"), is);
         int idx = pCB->AddString(strval);
         pCB->SetItemData(idx, is);
      }
   }

   if (cursel != CB_ERR && cursel > numridgepts - 1)
   {
      // pick a reasonable default
      m_RoadwaySectionData.ProfileGradePointIdx = max(1, numridgepts / 2);
   }

   pCB->SetCurSel((int)m_RoadwaySectionData.ProfileGradePointIdx - 1);

   BOOL bEnable = m_Grid.IsGridEmpty() ? FALSE : TRUE;
   pCB->EnableWindow(bEnable);
}

void CCrownSlopePage::OnCbnSelchangeNumsegmentsCombo()
{
   CComboBox* pcbNsegs = (CComboBox*)GetDlgItem(IDC_NUMSEGMENTS_COMBO);
   int cursel = pcbNsegs->GetCurSel();
   IndexType numSegs = (IndexType)pcbNsegs->GetItemData(cursel);

   m_RoadwaySectionData.NumberOfSegmentsPerSection = numSegs;

   RoadwaySegmentData defaultSeg;
   defaultSeg.Length = 0.0;
   defaultSeg.Slope = 0.0;

   for (auto& rst : m_RoadwaySectionData.RoadwaySectionTemplates)
   {
      rst.SegmentDataVec.resize(numSegs-2, defaultSeg);
   }

   UpdateRidgeptData();
   UpdateProfilePointData();

   m_Grid.InitRoadwaySectionData(true);

   OnChange();
}


void CCrownSlopePage::OnCbnSelchangeRidgeptCombo()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_RIDGEPT_COMBO);
   int cursel = pcb->GetCurSel();
   if (cursel != CB_ERR)
   {
      m_RoadwaySectionData.AlignmentPointIdx = (IndexType)pcb->GetItemData(cursel);
   }
   else
   {
      m_RoadwaySectionData.AlignmentPointIdx = 1;
   }

   OnChange(); // redraw schematic
}

void CCrownSlopePage::OnCbnSelchangeProfileGradePointCombo()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_PROFILE_GRADE_POINT);
   int cursel = pcb->GetCurSel();
   if (cursel != CB_ERR)
   {
      m_RoadwaySectionData.ProfileGradePointIdx = (IndexType)pcb->GetItemData(cursel);
   }
   else
   {
      m_RoadwaySectionData.ProfileGradePointIdx = 1;
   }

   OnChange(); // redraw schematic
}

int CCrownSlopePage::GetTemplateCount()
{
   return m_Grid.GetRowCount() - 1;
}

bool CCrownSlopePage::GetSelectedTemplate(RoadwaySectionTemplate* pTemplate)
{
   // Create a default if we fail
   pTemplate->LeftSlope = 0.0;
   pTemplate->RightSlope = 0.0;

   if (m_SelectedTemplate != INVALID_INDEX)
   {
      if (m_Grid.UpdateRoadwaySectionData())
      {
         if (m_RoadwaySectionData.RoadwaySectionTemplates.size() > m_SelectedTemplate)
         {
            *pTemplate = m_RoadwaySectionData.RoadwaySectionTemplates[m_SelectedTemplate];
            return true;
         }
         else
         {
            ATLASSERT(m_RoadwaySectionData.RoadwaySectionTemplates.empty() ? TRUE : FALSE); // combo showing something it should not
            return false;
         }
      }
      else
      {
         return false;
      }
   }

   return true;
}

gpRect2d CCrownSlopePage::GetRidgePointBounds()
{
   Float64 xmax(-Float64_Max), xmin(Float64_Max);
   Float64 ymax(-Float64_Max), ymin(Float64_Max);

   for (const auto& rp : m_DrawnRidgePoints)
   {
      xmax = max(rp.X(), xmax);
      xmin = min(rp.X(), xmin);
      ymax = max(rp.Y(), ymax);
      ymin = min(rp.Y(), ymin);
   }

   return gpRect2d(xmin,ymin,xmax,ymax);
}


void CCrownSlopePage::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
   CWnd* pWnd = GetDlgItem(IDC_PICTURE);
   CRect redit;
   pWnd->GetClientRect(&redit);
   CRgn rgn;
   VERIFY(rgn.CreateRectRgn(redit.left,redit.top,redit.right,redit.bottom));
   CDC* pDC = pWnd->GetDC();
   pDC->SelectClipRgn(&rgn);
   pWnd->Invalidate();
   pWnd->UpdateWindow();

   m_DrawnRidgePoints.clear();

   RoadwaySectionTemplate currTempl; 
   if (!GetSelectedTemplate(&currTempl))
   {
      CString str(_T("Data Error in Grid. Cannot Draw Section"));
      CSize tsiz = pDC->GetTextExtent(str);
      pDC->TextOut(redit.CenterPoint().x - tsiz.cx/2, redit.CenterPoint().y-tsiz.cy/2, str);
      return;
   }

   if (currTempl.SegmentDataVec.empty())
   {
      // no interior segments. Just draw a triangle centered in view. Use 10 as a default distance
      Float64 yval(0.0);
      m_DrawnRidgePoints.push_back(gpPoint2d(0.0, yval));
      yval += (m_RoadwaySectionData.slopeMeasure == RoadwaySectionData::RelativeToAlignmentPoint ? -1.0 : 1.0) * currTempl.LeftSlope * 10.0;
      m_DrawnRidgePoints.push_back(gpPoint2d(10.0, yval));
      yval += 1.0 * currTempl.RightSlope * 10.0;
      m_DrawnRidgePoints.push_back(gpPoint2d(20.0, yval));
   }
   else
   {
      // We have some interior segments. Sum up their lengths and use the middle 3/5 of the view to draw them
      Float64 sum_lengths_of_interior_segments(0.0);
      for (auto& rseg : currTempl.SegmentDataVec)
      {
         sum_lengths_of_interior_segments += rseg.Length;
      }

      Float64 exterior_seg_length(10.0); // default if all segments are zero length
      if (0.0 < sum_lengths_of_interior_segments)
      {
         exterior_seg_length = sum_lengths_of_interior_segments / 3.0;
      }

      Float64 xval(0.0), yval(0.0);
      // leftmost segment
      m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));
      xval += exterior_seg_length;
      yval += (m_RoadwaySectionData.slopeMeasure == RoadwaySectionData::RelativeToAlignmentPoint ? -1.0 : 1.0) * currTempl.LeftSlope * exterior_seg_length;
      m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));

      // exterior segments. track which side of controlling ridge point we are on
      IndexType ridgePtIdx(2);
      for (auto& rseg : currTempl.SegmentDataVec)
      {
         xval += rseg.Length;

         Float64 flipSlope = (m_RoadwaySectionData.slopeMeasure == RoadwaySectionData::RelativeToAlignmentPoint ? ((ridgePtIdx <= m_RoadwaySectionData.AlignmentPointIdx) ? -1.0 : 1.0) : 1.0);

         yval += flipSlope * rseg.Slope * rseg.Length;

         m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));

         ridgePtIdx++;
      }

      // rightmost segment
      xval += exterior_seg_length;
      yval += 1.0 * currTempl.RightSlope * exterior_seg_length;
      m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));
   }

   // bounds of window
   CSize csize = redit.Size();

   // bounding rect around model
   gpRect2d ridgeBounds = GetRidgePointBounds();

   // zoom out a bit
   gpSize2d rpsize(ridgeBounds.Size());
   rpsize.Dx() *= 0.025;
   rpsize.Dy() *= 0.1;
   ridgeBounds = ridgeBounds.InflateBy(rpsize);
   rpsize = ridgeBounds.Size();

   gpPoint2d org(ridgeBounds.Center());

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(GraphSize(rpsize.Dx(),rpsize.Dy()));
   mapper.SetWorldOrg(GraphPoint(org.X(),org.Y()));
   mapper.SetDeviceExt(csize.cx,csize.cy);
   mapper.SetDeviceOrg(csize.cx/2,csize.cy/2);

   // Draw primary template lines
   const int LINE_WID = 2;
   CPen solid_pen(PS_SOLID,LINE_WID,ALIGNMENT_COLOR);
   CBrush solid_brush(ALIGNMENT_COLOR);
   CFont font;
   font.CreatePointFont(80,_T("Arial"),pDC);

   CFont* oldFont = pDC->SelectObject(&font);
   CPen* pOldPen     = pDC->SelectObject(&solid_pen);
   CBrush* pOldBrush = pDC->SelectObject(&solid_brush);

   // Start arrow line
   const gpPoint2d& rp0 = m_DrawnRidgePoints[0];
   const gpPoint2d& rp1 = m_DrawnRidgePoints[1];
   long dx0, dy0, dx1, dy1;
   mapper.WPtoDP(rp0.X(), rp0.Y(), &dx0, &dy0);
   mapper.WPtoDP(rp1.X(), rp1.Y(), &dx1, &dy1);

   DrawArrowLine(pDC, dx1, dy1, dx0, dy0, LINE_WID*4, LINE_WID*8);

   // Interior lines
   IndexType numrp = m_DrawnRidgePoints.size();
   for (IndexType irp = 1; irp < numrp - 1; irp++)
   {
      const gpPoint2d& rp = m_DrawnRidgePoints[irp];

      long dx, dy;
      mapper.WPtoDP(rp.X(), rp.Y(), &dx, &dy);
      if (irp == 1)
      {
         pDC->MoveTo(dx, dy);
      }
      else
      {
         pDC->LineTo(dx, dy);
      }
   }

   // End arrow line
   const gpPoint2d& rps = m_DrawnRidgePoints[numrp-2];
   const gpPoint2d& rpe = m_DrawnRidgePoints[numrp-1];
   long dxs, dys, dxe, dye;
   mapper.WPtoDP(rps.X(), rps.Y(), &dxs, &dys);
   mapper.WPtoDP(rpe.X(), rpe.Y(), &dxe, &dye);

   DrawArrowLine(pDC, dxs, dys, dxe, dye, LINE_WID*4, LINE_WID*8);

   // Draw dots on interior ridge points
   CBrush rp_brush(BLACK);
   CPen rp_pen(PS_SOLID,LINE_WID,BLACK);
   CBrush crp_brush(RED);
   CPen crp_pen(PS_SOLID,LINE_WID,RED);
   CBrush pgp_brush(GREEN);
   CPen pgp_pen(PS_SOLID, LINE_WID, GREEN);

   CRect elrect(CPoint(0,0),CSize(LINE_WID*4, LINE_WID*4));

   for (IndexType irp = 1; irp < numrp - 1; irp++)
   {
      const gpPoint2d& rp = m_DrawnRidgePoints[irp];
      long dx, dy;
      mapper.WPtoDP(rp.X(), rp.Y(), &dx, &dy);

      if (irp == m_RoadwaySectionData.AlignmentPointIdx && irp == m_RoadwaySectionData.ProfileGradePointIdx)
      {
         // if the controlling ridge point and profile grade point are the same, then draw a circle that half in each color
         elrect.MoveToXY(dx - LINE_WID * 2, dy - LINE_WID * 2);

         pDC->SelectObject(crp_brush);
         pDC->SelectObject(crp_pen);
         pDC->SetTextColor(RED);
         pDC->Pie(elrect, CPoint(elrect.left + elrect.Width() / 2, elrect.top), CPoint(elrect.left + elrect.Width() / 2, elrect.bottom));

         pDC->SelectObject(pgp_brush);
         pDC->SelectObject(pgp_pen);
         pDC->SetTextColor(GREEN);
         pDC->Pie(elrect, CPoint(elrect.left + elrect.Width() / 2, elrect.bottom), CPoint(elrect.left + elrect.Width() / 2, elrect.top));
      }
      else if (irp == m_RoadwaySectionData.AlignmentPointIdx)
      {
         pDC->SelectObject(crp_brush);
         pDC->SelectObject(crp_pen);
         pDC->SetTextColor(RED);
         elrect.MoveToXY(dx - LINE_WID * 2, dy - LINE_WID * 2);
         pDC->Ellipse(elrect);
      }
      else if (irp == m_RoadwaySectionData.ProfileGradePointIdx)
      {
         pDC->SelectObject(pgp_brush);
         pDC->SelectObject(pgp_pen);
         pDC->SetTextColor(GREEN);
         elrect.MoveToXY(dx - LINE_WID * 2, dy - LINE_WID * 2);
         pDC->Ellipse(elrect);
      }
      else
      {
         pDC->SelectObject(rp_brush);
         pDC->SelectObject(rp_pen);
         pDC->SetTextColor(BLACK);
         elrect.MoveToXY(dx - LINE_WID * 2, dy - LINE_WID * 2);
         pDC->Ellipse(elrect);
      }


      CString str;
      str.Format(_T("%d"),irp);
      CSize tsiz = pDC->GetTextExtent(str);
      pDC->TextOut(dx+tsiz.cx, dy+LINE_WID*3, str);
   }

   // Last draw segment numbers
   pDC->SetTextColor(ALIGNMENT_COLOR);
   for (IndexType irp = 0; irp < numrp-1; irp++)
   {
      const gpPoint2d& rps = m_DrawnRidgePoints[irp];
      const gpPoint2d& rpe = m_DrawnRidgePoints[irp+1];

      long midx, midy;
      mapper.WPtoDP((rps.X()+rpe.X())/2.0, (rps.Y()+rpe.Y())/2.0, &midx, &midy);

      CString str;
      str.Format(_T("%d"),irp+1);
      CSize tsiz = pDC->GetTextExtent(str);
      pDC->TextOut(midx+tsiz.cx/2, midy-tsiz.cy/2, str);
   }

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
   pDC->SelectObject(oldFont);

   pWnd->ReleaseDC(pDC);
}

void CCrownSlopePage::OnChange() 
{
   CWnd* pPicture = GetDlgItem(IDC_PICTURE);
   CRect rect;
   pPicture->GetWindowRect(rect);
   ScreenToClient(&rect);
   InvalidateRect(rect);
   UpdateWindow();
}

void CCrownSlopePage::UpdateViewSpinner()
{
   int nTemplates = GetTemplateCount();
   nTemplates = nTemplates == 0 ? 1 : nTemplates; //always have flat default

   if (nTemplates <= m_SelectedTemplate)
   {
      m_SelectedTemplate = nTemplates - 1;
   }

   CString str;
   str.Format(_T("Template %d "), m_SelectedTemplate + 1);

   CStatic* pedit = (CStatic*)GetDlgItem(IDC_VIEW_TEMPLATE_EDIT);
   pedit->SetWindowText(str);

   // redraw image
   OnChange();
}

void CCrownSlopePage::OnUpdateAdd(CCmdUI * pCmdUI)
{
   ROWCOL badRow;
   pCmdUI->Enable( m_Grid.IsGridDataValid(&badRow));
}

void CCrownSlopePage::OnUpdateRemove(CCmdUI * pCmdUI)
{
   ROWCOL badRow;
   pCmdUI->Enable(m_Grid.IsRowSelected() && !m_Grid.IsGridEmpty() && m_Grid.IsGridDataValid(&badRow));
}

void CCrownSlopePage::OnUpdateSort(CCmdUI * pCmdUI)
{
   ROWCOL badRow;
   pCmdUI->Enable(!m_Grid.IsGridEmpty() && m_Grid.IsGridDataValid(&badRow));
}

LRESULT CCrownSlopePage::OnKickIdle(WPARAM, LPARAM)
{
	UpdateDialogControls(this, FALSE);
	return 0L;
}

void CCrownSlopePage::OnDeltaposViewTemplateSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
   LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
   *pResult = 0;

   int nTemplates = GetTemplateCount();

   // this is what the selection will be
   //int new_sel = (int)m_SelectedTemplate - pNMUpDown->iDelta; // left arrow is positive
   int new_sel = pNMUpDown->iPos + pNMUpDown->iDelta;

   if (new_sel < 0 || nTemplates <= new_sel)
   {
      // we are going to scroll over the top or under the bottom
      // return with pResult being non-zero to prevent change
      *pResult = 1;
      return;
   }

   m_SelectedTemplate = new_sel;

   UpdateViewSpinner();
}


void CCrownSlopePage::OnCbnSelchangeSlopeMeasureType()
{
   // TODO: Add your control notification handler code here
   CWnd* pWnd = GetDlgItem(IDC_SLOPE_NOTE);
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SLOPE_MEASURE_TYPE);
   int idx = pCB->GetCurSel();
   if (idx == CB_ERR)
      idx = 0;

   RoadwaySectionData::SlopeMeasure slopeMeasure = (RoadwaySectionData::SlopeMeasure)(pCB->GetItemData(idx));
   if (slopeMeasure == RoadwaySectionData::RelativeToAlignmentPoint)
   {
      pWnd->SetWindowText(_T("Slope Sign Convention: Slopes upward and away from the Alignment Point are positive, downward and away are negative. See Help for details."));
   }
   else
   {
      pWnd->SetWindowText(_T("Slope Sign Convention: Slopes upward and to the right are positive, downward and to the right are negative. See Help for details."));
   }

   m_RoadwaySectionData.slopeMeasure = slopeMeasure;

   OnChange();
}
