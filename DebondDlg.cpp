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

// DebondDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "DebondDlg.h"
#include "GirderDescDlg.h"
#include "PGSuperColors.h"
#include <IFace\Bridge.h>
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NO_DEBOND_FILL_COLOR GREY90

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondPage dialog


CGirderDescDebondPage::CGirderDescDebondPage()
	: CPropertyPage(CGirderDescDebondPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescDebondPage)
	m_bSymmetricDebond = FALSE;
	//}}AFX_DATA_INIT

}

void CGirderDescDebondPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescDebondPage)
	DDX_Check(pDX, IDC_SYMMETRIC_DEBOND, m_bSymmetricDebond);
	//}}AFX_DATA_MAP

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);

   if ( pDX->m_bSaveAndValidate )
   {
      GET_IFACE2(pBroker, IBridge,pBridge);
      Float64 gdr_length2 = pBridge->GetGirderLength(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx)/2.0;

      m_Grid.GetData(m_GridData);

      for (std::vector<CDebondInfo>::iterator iter = m_GridData.begin(); iter != m_GridData.end(); iter++ )
      {
         CDebondInfo& debond_info = *iter;
         if (debond_info.Length1 >= gdr_length2 || debond_info.Length2 >= gdr_length2)
         {
            HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_DEBOND_GRID);
	         AfxMessageBox( "Debond length cannot exceed one half of girder length.", MB_ICONEXCLAMATION);
	         pDX->Fail();
         }
      }

      pParent->m_GirderData.Debond[pgsTypes::Straight] = m_GridData;
      pParent->m_GirderData.bSymmetricDebond           = m_bSymmetricDebond ? TRUE : FALSE;
   }
}


BEGIN_MESSAGE_MAP(CGirderDescDebondPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescDebondPage)
	ON_BN_CLICKED(IDD_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_SYMMETRIC_DEBOND, OnSymmetricDebond)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondPage message handlers
BOOL CGirderDescDebondPage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();

	m_Grid.SubclassDlgItem(IDC_DEBOND_GRID, this);
   m_Grid.CustomInit(m_bSymmetricDebond ? TRUE : FALSE);

   // List of debondable strands does not change - might was well save here and reuse it
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   pStrandGeometry->ListDebondableStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, pgsTypes::Straight, &m_Debondables);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CGirderDescDebondPage::OnSetActive() 
{
   // make sure we don't have more debonded strands than total strands
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nStrands = pParent->GetStraightStrandCount();
   std::vector<CDebondInfo>::iterator it=m_GridData.begin();
   while ( it!=m_GridData.end() )
   {
      if (nStrands < it->idxStrand1 || nStrands < it->idxStrand2)
      {
         it = m_GridData.erase(it);
      }
      else
      {
         it++;
      }
   }

   m_Grid.FillGrid(m_GridData);

   BOOL enab = nStrands>0 ? TRUE:FALSE;
   GetDlgItem(IDC_SYMMETRIC_DEBOND)->EnableWindow(enab);

   this->OnEnableDelete(false);

   enab &= CanDebondMore() ? TRUE:FALSE;
   GetDlgItem(IDC_ADD)->EnableWindow(enab);

   m_Grid.SelectRange(CGXRange().SetTable(), FALSE);

   OnChange();

   return CPropertyPage::OnSetActive();
	
}

BOOL CGirderDescDebondPage::OnKillActive()
{
   this->SetFocus();  // prevents artifacts from grid list controls (not sure why)

   return CPropertyPage::OnKillActive();
}

void CGirderDescDebondPage::OnAdd() 
{
   m_Grid.InsertRow();

   GetDlgItem(IDC_ADD)->EnableWindow(CanDebondMore()?TRUE:FALSE);

   OnChange();
}

void CGirderDescDebondPage::OnDelete() 
{
	m_Grid.DoRemoveRows();

   GetDlgItem(IDC_ADD)->EnableWindow(CanDebondMore()?TRUE:FALSE);

   OnChange();

   // Force a repaint so the picture is updated
   Invalidate();
   UpdateWindow();

   OnEnableDelete(false); // nothing selected
}

std::vector<CString>  CGirderDescDebondPage::GetStrandList()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   std::vector<CString>  strList;
   StrandIndexType nStrands = GetNumStrands();

   // need to omit strands already selected
   StrandIndexType currnum=0;
   while( currnum< nStrands )
   {
      StrandIndexType nextnum = pStrandGeom->GetNextNumStrands(pParent->m_CurrentSpanIdx, pParent->m_CurrentGirderIdx, pgsTypes::Straight, currnum);

      Int32 is_debondable;
      // nextnum is a count. need to subtract 1 to get index
      m_Debondables->get_Item(nextnum-1, &is_debondable);

      // only add debondable strands to the list
      if (is_debondable)
      {
         CString str;
         if ( nextnum-currnum == 1 )
         {
            str.Format("%d",nextnum);
         }
         else
         {
            str.Format("%d & %d", nextnum-1, nextnum );
         }

         strList.push_back(str);
      }

      currnum = nextnum;
   }
   
   return strList;
}

std::vector<CDebondInfo> CGirderDescDebondPage::GetDebondInfo() const
{
   return m_GridData;
}


void CGirderDescDebondPage::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CProperyPage::OnPaint() for painting messages

   // Draw the girder cross section and label the strand locations
   // The basic logic from this code is take from
   // Programming Microsoft Visual C++, Fifth Edition
   // Kruglinski, Shepherd, and Wingo
   // Page 129
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CWnd* pWnd = GetDlgItem(IDC_PICTURE);
   CRect redit;
   pWnd->GetClientRect(&redit);
   CRgn rgn;
   VERIFY(rgn.CreateRectRgn(redit.left,redit.top,redit.right,redit.bottom));
   CDC* pDC = pWnd->GetDC();
   pDC->SelectClipRgn(&rgn);
   pWnd->Invalidate();
   pWnd->UpdateWindow();

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);
   CComPtr<IShape> shape;
   pgsPointOfInterest poi(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx,0.00);

   pSectProp->GetGirderShape(poi,false,&shape);

   CComQIPtr<IXYPosition> position(shape);
   CComPtr<IPoint2d> lp;
   position->get_LocatorPoint(lpBottomCenter,&lp);
   lp->Move(0,0);
   position->put_LocatorPoint(lpBottomCenter,lp);


   // Get the world height to be equal to the height of the area 
   // occupied by the strands
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   double y_min =  DBL_MAX;
   double y_max = -DBL_MAX;
   StrandIndexType nStrands = GetNumStrands();
   CComPtr<IPoint2dCollection> points;
   pStrandGeometry->GetStrandPositionsEx(poi,nStrands, pgsTypes::Straight,&points);
   for ( StrandIndexType strIdx = 0; strIdx < nStrands; strIdx++ )
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strIdx,&point);
      double y;
      point->get_Y(&y);
      y_min = _cpp_min(y,y_min);
      y_max = _cpp_max(y,y_max);
   }
   gpSize2d size;
   
   GET_IFACE2(pBroker,IGirder,pGirder);
   size.Dx() = pGirder->GetBottomWidth(poi);

   size.Dy() = (y_max - y_min);
   if ( IsZero(size.Dy()) )
      size.Dy() = size.Dx()/2;

   CSize csize = redit.Size();

   CComPtr<IRect2d> box;
   shape->get_BoundingBox(&box);

   CComPtr<IPoint2d> objOrg;
   box->get_BottomCenter(&objOrg);

   gpPoint2d org;
   double x,y;
   objOrg->get_X(&x);
   objOrg->get_Y(&y);
   org.X() = x;
   org.Y() = y;

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx-10,csize.cy);
   mapper.SetDeviceOrg(csize.cx/2,csize.cy-5);

   CPen solid_pen(PS_SOLID,1,GIRDER_BORDER_COLOR);
   CBrush solid_brush(GIRDER_FILL_COLOR);

   CPen void_pen(PS_SOLID,1,VOID_BORDER_COLOR);
   CBrush void_brush(GetSysColor(COLOR_WINDOW));

   CPen* pOldPen     = pDC->SelectObject(&solid_pen);
   CBrush* pOldBrush = pDC->SelectObject(&solid_brush);

   CComQIPtr<ICompositeShape> compshape(shape);
   if ( compshape )
   {
      CollectionIndexType nShapes;
      compshape->get_Count(&nShapes);
      for ( CollectionIndexType idx = 0; idx < nShapes; idx++ )
      {
         CComPtr<ICompositeShapeItem> item;
         compshape->get_Item(idx,&item);

         CComPtr<IShape> s;
         item->get_Shape(&s);

         VARIANT_BOOL bVoid;
         item->get_Void(&bVoid);

         if ( bVoid )
         {
            pDC->SelectObject(&void_pen);
            pDC->SelectObject(&void_brush);
         }
         else
         {
            pDC->SelectObject(&solid_pen);
            pDC->SelectObject(&solid_brush);
         }

         DrawShape(pDC,s,mapper);
      }
   }
   else
   {
      DrawShape(pDC,shape,mapper);
   }

   DrawStrands(pDC,mapper);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   pWnd->ReleaseDC(pDC);
}

void CGirderDescDebondPage::DrawShape(CDC* pDC,IShape* shape,grlibPointMapper& mapper)
{
   CComPtr<IPoint2dCollection> objPoints;
   shape->get_PolyPoints(&objPoints);

   CollectionIndexType nPoints;
   objPoints->get_Count(&nPoints);

   CPoint* points = new CPoint[nPoints];

   CComPtr<IPoint2d> point;
   long dx,dy;

   long i = 0;
   CComPtr<IEnumPoint2d> enumPoints;
   objPoints->get__Enum(&enumPoints);
   while ( enumPoints->Next(1,&point,NULL) != S_FALSE )
   {
      mapper.WPtoDP(point,&dx,&dy);

      points[i] = CPoint(dx,dy);

      point.Release();
      i++;
   }

   pDC->Polygon(points,nPoints);

   delete[] points;
}

void CGirderDescDebondPage::DrawStrands(CDC* pDC,grlibPointMapper& mapper)
{
   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CPen strand_pen(PS_SOLID,1,STRAND_BORDER_COLOR);
   CPen no_debond_pen(PS_SOLID,1,NO_DEBOND_FILL_COLOR);
   CPen debond_pen(PS_SOLID,1,DEBOND_FILL_COLOR);
   CPen* old_pen = (CPen*)pDC->SelectObject(&strand_pen);

   CBrush strand_brush(STRAND_FILL_COLOR);
   CBrush no_debond_brush(NO_DEBOND_FILL_COLOR);
   CBrush debond_brush(DEBOND_FILL_COLOR);
   CBrush* old_brush = (CBrush*)pDC->SelectObject(&strand_brush);

   pDC->SetTextAlign(TA_CENTER);
   CFont font;
   font.CreatePointFont(80,"Arial",pDC);
   CFont* old_font = pDC->SelectObject(&font);
   pDC->SetBkMode(TRANSPARENT);

   pgsPointOfInterest poi(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx,0.00); // assume all girders are the same

   // Draw all the strands bonded
   StrandIndexType nStrands = GetNumStrands();
   CComPtr<IPoint2dCollection> points;
   pStrandGeometry->GetStrandPositionsEx(poi,nStrands, pgsTypes::Straight,&points);

   const int strand_size = 2;
   for ( StrandIndexType strIdx = 0; strIdx <nStrands; strIdx++ )
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strIdx,&point);

      Int32 is_debondable;
      m_Debondables->get_Item(strIdx, &is_debondable);

      long dx,dy;
      mapper.WPtoDP(point,&dx,&dy);

      CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

      if (is_debondable)
      {
         pDC->SelectObject(&strand_pen);
         pDC->SelectObject(&strand_brush);
      }
      else
      {
         pDC->SelectObject(&no_debond_pen);
         pDC->SelectObject(&no_debond_brush);
      }

      pDC->Ellipse(&rect);

      CString strLabel;
      strLabel.Format("%d",strIdx+1);
      pDC->TextOut(dx,dy,strLabel);
   }

   // Redraw the debonded strands
   pDC->SelectObject(&debond_pen);
   pDC->SelectObject(&debond_brush);

   std::vector<CDebondInfo>::iterator iter;
   m_Grid.GetData(m_GridData);
   for ( iter = m_GridData.begin(); iter != m_GridData.end(); iter++ )
   {
      CDebondInfo& debond_info = *iter;

      if ( debond_info.idxStrand1 == Uint32_Max )
         continue;

      CComPtr<IPoint2d> point;
      points->get_Item(debond_info.idxStrand1,&point);

      long dx,dy;
      mapper.WPtoDP(point,&dx,&dy);

      CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

      pDC->Ellipse(&rect);

      if ( debond_info.idxStrand2 != Uint32_Max )
      {
         point.Release();
         points->get_Item(debond_info.idxStrand2,&point);

         long dx,dy;
         mapper.WPtoDP(point,&dx,&dy);

         CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

         pDC->Ellipse(&rect);
      }
   }

   pDC->SelectObject(old_pen);
   pDC->SelectObject(old_brush);
   pDC->SelectObject(old_font);

}

StrandIndexType CGirderDescDebondPage::GetNumStrands()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nStrands =  pParent->GetStraightStrandCount();

   return nStrands;
}

bool CGirderDescDebondPage:: CanDebondMore()
{
   // how many can be debonded?
   StrandIndexType nStrands = GetNumStrands(); 
   StrandIndexType nDebondable = 0; // number of strand that can be debonded
   for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
   {
      Int32 is_debondable;
      m_Debondables->get_Item(strandIdx, &is_debondable);
      if (is_debondable)
      {
         nDebondable++;
      }
   }
   
   StrandIndexType nDebonded =  m_Grid.GetNumDebondedStrands(); // number of strands that are debonded

   return nDebonded < nDebondable; // can debond more if the number of debondable exceed number of debonded
}

void CGirderDescDebondPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	
   CRect cRect;
   GetClientRect(&cRect);

   CWnd* pOK = GetDlgItem(IDOK);
   CWnd* pCancel = GetDlgItem(IDCANCEL);
   CWnd* pAdd = GetDlgItem(IDC_ADD);
   CWnd* pDelete = GetDlgItem(IDC_DELETE);
   CWnd* pGrid = GetDlgItem(IDC_DEBOND_GRID);
   CWnd* pPicture = GetDlgItem(IDC_PICTURE);

   if ( pOK == NULL )
      return; // child controls have not yet been created

   // Move the cancel button to the bottom right corner
   CRect btnRect;
   pDelete->GetClientRect(&btnRect);
   CSize btnSize = btnRect.Size();

   btnRect.BottomRight() = cRect.BottomRight();
   btnRect.BottomRight() -= CSize(5,5);
   btnRect.TopLeft() = btnRect.BottomRight() - btnSize;

   pCancel->MoveWindow(btnRect);

   // Move the ok button to the left of the cancel button
   btnRect -= CSize(btnSize.cx + 5,0);
   pOK->MoveWindow(btnRect);

   // Move the add button above the ok button
   btnRect -= CSize(0,btnSize.cy + 5);
   pAdd->MoveWindow(btnRect);

   // Move delete bottom to the right of the add button
   btnRect += CSize(btnSize.cx + 5,0);
   pDelete->MoveWindow(btnRect);

   // Compute remaining client rect (above the buttons)
   cRect.BottomRight() = CPoint(btnRect.right,btnRect.top-5);
   CSize cSize = cRect.Size();

   CRect rPic(cRect);
   rPic.TopLeft() += CSize(5,5);
   rPic.BottomRight() = CPoint( cRect.right, cRect.bottom/2 - 3);
   pPicture->MoveWindow(rPic);

   CRect rGrid(cRect);
   rGrid.TopLeft() = CPoint( cRect.left+5, cRect.bottom/2 + 2);
   pGrid->MoveWindow(rGrid);

   Invalidate();
   UpdateWindow();
}

void CGirderDescDebondPage::OnSymmetricDebond() 
{
   UINT checked = IsDlgButtonChecked(IDC_SYMMETRIC_DEBOND);
   m_Grid.SymmetricDebond( checked != 0 );
}

void CGirderDescDebondPage::OnHelp() 
{
	::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_DEBOND );
}

void CGirderDescDebondPage::OnChange() 
{
   long ns = GetNumStrands(); 
   long ndbs =  m_Grid.GetNumDebondedStrands();
   double percent = 0.0;
   if (ns>0)
      percent = 100.0 * (double)ndbs/ns;

   CString str;
   str.Format("Number of straight strands = %d", ns);
   CWnd* pNs = GetDlgItem(IDC_NUMSTRAIGHT);
   pNs->SetWindowText(str);

   str.Format("Number of debonded strands = %d", ndbs);
   CWnd* pNdb = GetDlgItem(IDC_NUM_DEBONDED);
   pNdb->SetWindowText(str);

   str.Format("Percentage of straight strands debonded = %.1f%%", percent);
   CWnd* pPcnt = GetDlgItem(IDC_PERCENT_DEBONDED);
   pPcnt->SetWindowText(str);
}


void CGirderDescDebondPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_DELETE);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}
