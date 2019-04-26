///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

   DDX_CBItemData(pDX, IDC_NUMSEGMENTS_COMBO,  (int&)m_RoadwaySectionData.NumberOfSegmentsPerSection);
   DDX_CBItemData(pDX, IDC_RIDGEPT_COMBO,  (int&)m_RoadwaySectionData.ControllingRidgePointIdx);

   // Grid owns a pointer to our roadway data
   if ( pDX->m_bSaveAndValidate )
   {
      if (!m_Grid.SortCrossSections() || !m_Grid.UpdateRoadwaySectionData() )
      {
         AfxMessageBox(_T("Invalid cross section parameters"));
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
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
   ON_UPDATE_COMMAND_UI(IDC_REMOVE, &CCrownSlopePage::OnUpdateRemove)
	ON_BN_CLICKED(IDC_SORT, OnSort)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
   ON_CBN_SELCHANGE(IDC_NUMSEGMENTS_COMBO, &CCrownSlopePage::OnCbnSelchangeNumsegmentsCombo)
   ON_CBN_SELCHANGE(IDC_RIDGEPT_COMBO, &CCrownSlopePage::OnCbnSelchangeRidgeptCombo)
   ON_CBN_SELCHANGE(IDC_VIEW_TEMPLATE_COMBO, &CCrownSlopePage::OnCbnSelchangeViewTemplateCombo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopePage message handlers

void CCrownSlopePage::OnAdd() 
{
   m_Grid.AppendRow();

   UpdateNumSegsCtrl();
   UpdateRidgeptData();
   UpdateViewCombo();
}

void CCrownSlopePage::OnRemove() 
{
   m_Grid.RemoveRows();

   if (m_Grid.IsGridEmpty())
   {
      // Just emptied out the grid. Force data to defaults
      m_RoadwaySectionData.ControllingRidgePointIdx = 1;
      m_RoadwaySectionData.NumberOfSegmentsPerSection = 2;
      m_RoadwaySectionData.RoadwaySectionTemplates.clear();

      UpdateNumSegsCtrl();
      UpdateRidgeptData();
   }

   UpdateViewCombo();
}

void CCrownSlopePage::OnSort() 
{
   if (!m_Grid.SortCrossSections())
   {
      ::AfxMessageBox(_T("Each template must have a unique Station"),MB_OK | MB_ICONWARNING); 
   }
}

BOOL CCrownSlopePage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_VCURVE_GRID, this);
   m_Grid.CustomInit();

   FillNumSegsCtrl();
   UpdateRidgeptData();

   CPropertyPage::OnInitDialog();

   UpdateViewCombo();
	
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
   if (cursel == CB_ERR)
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
      m_RoadwaySectionData.ControllingRidgePointIdx = max(1, numridgepts / 2);
   }

   pcbRidgePts->SetCurSel((int)m_RoadwaySectionData.ControllingRidgePointIdx-1);

   BOOL bEnable = m_Grid.IsGridEmpty() ? FALSE : TRUE;
   pcbRidgePts->EnableWindow(bEnable);
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

   m_Grid.InitRoadwaySectionData(true);

   OnChange();
}


void CCrownSlopePage::OnCbnSelchangeRidgeptCombo()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_RIDGEPT_COMBO);
   int cursel = pcb->GetCurSel();
   if (cursel != CB_ERR)
   {
      m_RoadwaySectionData.ControllingRidgePointIdx = (IndexType)pcb->GetItemData(cursel);
   }
   else
   {
      m_RoadwaySectionData.ControllingRidgePointIdx = 1;
   }

   OnChange(); // redraw schematic
}

RoadwaySectionTemplate CCrownSlopePage::GetSelectedTemplate()
{
   // Create a default if we fail
   RoadwaySectionTemplate templ;
   templ.LeftSlope = 0.0;
   templ.RightSlope = 0.0;

   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_VIEW_TEMPLATE_COMBO);
   int cursel = pcb->GetCurSel();
   if (cursel != CB_ERR)
   {
      if (m_Grid.UpdateRoadwaySectionData())
      {
         if (m_RoadwaySectionData.RoadwaySectionTemplates.size() > cursel)
         {
            return m_RoadwaySectionData.RoadwaySectionTemplates[cursel];
         }
         else
         {
            ATLASSERT(m_RoadwaySectionData.RoadwaySectionTemplates.empty() ? TRUE : FALSE); // combo showing something it should not
         }
      }
   }

   return templ;
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

   RoadwaySectionTemplate currTempl = GetSelectedTemplate();
   m_DrawnRidgePoints.clear();

   if (currTempl.SegmentDataVec.empty())
   {
      // no interior segments. Just draw a triangle centered in view. Use 10 as a default distance
      Float64 yval(0.0);
      m_DrawnRidgePoints.push_back(gpPoint2d(0.0, yval));
      yval += -1.0 * currTempl.LeftSlope * 10.0;
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
      if (sum_lengths_of_interior_segments > 0.0)
      {
         exterior_seg_length = sum_lengths_of_interior_segments / 3.0;
      }

      Float64 xval(0.0), yval(0.0);
      // leftmost segment
      m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));
      xval += exterior_seg_length;
      yval += -1.0 * currTempl.LeftSlope * exterior_seg_length;
      m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));

      // exterior segments. track which side of controlling ridge point we are on
      IndexType ridgePtIdx(2);
      for (auto& rseg : currTempl.SegmentDataVec)
      {
         xval += rseg.Length;

         Float64 flipSlope = (ridgePtIdx <= m_RoadwaySectionData.ControllingRidgePointIdx) ? -1.0 : 1.0;

         yval += flipSlope * rseg.Slope * rseg.Length;

         m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));

         ridgePtIdx++;
      }

      // rightmost segment
      xval += exterior_seg_length;
      yval += 1.0 * currTempl.RightSlope * exterior_seg_length;
      m_DrawnRidgePoints.push_back(gpPoint2d(xval, yval));
   }

   gpRect2d ridgeBounds = GetRidgePointBounds();

   // zoom out a bit
   gpSize2d rpsize(ridgeBounds.Size());
   rpsize *= 0.05;
   ridgeBounds = ridgeBounds.InflateBy(rpsize);
   rpsize = ridgeBounds.Size();

   gpPoint2d org(ridgeBounds.BottomLeft());

   CSize csize = redit.Size();

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(rpsize);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx,csize.cy);
   mapper.SetDeviceOrg(0,csize.cy/2);

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

   CRect elrect(CPoint(0,0),CSize(LINE_WID*4, LINE_WID*4));

   for (IndexType irp = 1; irp < numrp - 1; irp++)
   {
      const gpPoint2d& rp = m_DrawnRidgePoints[irp];
      long dx, dy;
      mapper.WPtoDP(rp.X(), rp.Y(), &dx, &dy);

      if (irp == m_RoadwaySectionData.ControllingRidgePointIdx)
      {
         pDC->SelectObject(crp_brush);
         pDC->SelectObject(crp_pen);
         pDC->SetTextColor(RED);
      }
      else
      {
         pDC->SelectObject(rp_brush);
         pDC->SelectObject(rp_pen);
         pDC->SetTextColor(BLACK);
      }

      elrect.MoveToXY(dx-LINE_WID*2,dy-LINE_WID*2);

      pDC->Ellipse(elrect);

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

void CCrownSlopePage::UpdateViewCombo()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_VIEW_TEMPLATE_COMBO);
   int cursel = pcb->GetCurSel();

   int size = m_Grid.GetRowCount() - 1;
   size = size == 0 ? 1 : size; //always have flat default

   pcb->ResetContent();

   for (int i = 0; i < size; i++)
   {
      CString str;
      str.Format(_T("Template %d"), i + 1);
      pcb->AddString(str);
   }

   cursel = (cursel != CB_ERR && cursel < size) ? cursel : 0;

   pcb->SetCurSel(cursel);

   // redraw image
   OnChange();
}


void CCrownSlopePage::OnCbnSelchangeViewTemplateCombo()
{
   OnChange();
}

void CCrownSlopePage::OnUpdateRemove(CCmdUI * pCmdUI)
{
   pCmdUI->Enable(m_Grid.IsRowSelected());
}

LRESULT CCrownSlopePage::OnKickIdle(WPARAM, LPARAM)
{
	UpdateDialogControls(this, FALSE);
	return 0L;
}

