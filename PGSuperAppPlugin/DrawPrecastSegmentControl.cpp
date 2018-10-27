// DrawPrecastSegmentControl.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperColors.h"
#include "DrawPrecastSegmentControl.h"

#include <IFace\Bridge.h>
#include <PgsExt\SplicedGirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CDrawPrecastSegmentControl

IMPLEMENT_DYNAMIC(CDrawPrecastSegmentControl, CWnd)

CDrawPrecastSegmentControl::CDrawPrecastSegmentControl()
{
   m_pSource = nullptr;
}

CDrawPrecastSegmentControl::~CDrawPrecastSegmentControl()
{
}


BEGIN_MESSAGE_MAP(CDrawPrecastSegmentControl, CWnd)
   ON_WM_PAINT()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CDrawPrecastSegmentControl message handlers
void CDrawPrecastSegmentControl::CustomInit(IPrecastSegmentDataSource* pSource)
{
   m_pSource = pSource;
}


void CDrawPrecastSegmentControl::OnPaint()
{
   CPaintDC dc(this); // device context for painting
   // TODO: Add your message handler code here
   // Do not call CWnd::OnPaint() for painting messages

   dc.SelectClipRgn(nullptr);

   const CSplicedGirderData* pSplicedGirder = m_pSource->GetGirder();
   SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();

   const CSegmentKey& segmentKey = m_pSource->GetSegmentKey();

   CSegmentKey prevSegmentKey(segmentKey);
   prevSegmentKey.segmentIndex--;

   CSegmentKey nextSegmentKey(segmentKey);
   nextSegmentKey.segmentIndex++;

   CComPtr<IShape> prevShape,shape,nextShape;
   CComPtr<IPoint2dCollection> prevBottomFlange, bottomFlange, nextBottomFlange;
   if ( 0 < segmentKey.segmentIndex )
   {
      CreateSegmentShape( prevSegmentKey,&prevShape,&prevBottomFlange);
   }

   CreateSegmentShape( segmentKey,  &shape, &bottomFlange);

   if ( segmentKey.segmentIndex < nSegments-1 )
   {
      CreateSegmentShape(nextSegmentKey,&nextShape,&nextBottomFlange);
   }

   // set up the clipping region so we don't draw outside of the client rect
   CRect rClient;
   GetClientRect(&rClient);
   CRgn rgn;
   rgn.CreateRectRgnIndirect(&rClient);
   dc.SelectClipRgn(&rgn);

   // setup coordinate mapping
   rClient.DeflateRect(1,1,1,1);
   CSize sClient = rClient.Size();

   CComPtr<IRect2d> rect;
   shape->get_BoundingBox(&rect);

   if ( prevShape )
   {
      // add half the width of the previous shape to the bounding box
      // (don't want to waste screen real estate with the whole previous shape)
      CComPtr<IRect2d> prevRect;
      prevShape->get_BoundingBox(&prevRect);
      Float64 width;
      prevRect->get_Width(&width);
      Float64 left;
      prevRect->get_Left(&left);
      prevRect->put_Left(left+width/2);
      rect->Union(prevRect);
   }

   if ( nextShape )
   {
      // add half the width of the next shape to the bounding box
      // (don't want to waste screen real estate with the whole next shape)
      CComPtr<IRect2d> nextRect;
      nextShape->get_BoundingBox(&nextRect);
      Float64 width;
      nextRect->get_Width(&width);
      Float64 right;
      nextRect->get_Right(&right);
      nextRect->put_Right(right-width/2);
      rect->Union(nextRect);
   }

   Float64 left, top, bottom, right;
   rect->get_Left(&left);
   rect->get_Top(&top);
   rect->get_Right(&right);
   rect->get_Bottom(&bottom);
   gpRect2d box(left,bottom,right,top);
   gpSize2d size = box.Size();
   gpPoint2d org = box.Center();

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(sClient.cx,sClient.cy);
   mapper.SetDeviceOrg(sClient.cx/2,sClient.cy/2);


   CPen this_segment_pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush this_segment_brush;
   CPen girder_pen(PS_SOLID,1,SEGMENT_BORDER_COLOR_ADJACENT);
   CBrush prev_segment_brush;
   CBrush next_segment_brush;

   COLORREF clrBadSegment = DARKSLATEGREY;

   CPen* pOldPen     = dc.GetCurrentPen();
   CBrush* pOldBrush = dc.GetCurrentBrush();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( prevShape )
   {
      Float64 framing_length = pBridge->GetSegmentFramingLength(prevSegmentKey);
      const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(prevSegmentKey.segmentIndex);
      if ( pSegment->AreSegmentVariationsValid(framing_length) )
      {
         prev_segment_brush.CreateSolidBrush(SEGMENT_FILL_COLOR_ADJACENT);
      }
      else
      {
         prev_segment_brush.CreateHatchBrush(HS_DIAGCROSS,clrBadSegment);
      }
      dc.SelectObject(&girder_pen);
      dc.SelectObject(&prev_segment_brush);
      DrawShape(&dc,mapper,prevShape);
      DrawBottomFlange(&dc,mapper,prevBottomFlange);
   }

   if ( nextShape )
   {
      Float64 framing_length = pBridge->GetSegmentFramingLength(nextSegmentKey);
      const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(nextSegmentKey.segmentIndex);
      if ( pSegment->AreSegmentVariationsValid(framing_length) )
      {
         next_segment_brush.CreateSolidBrush(SEGMENT_FILL_COLOR_ADJACENT);
      }
      else
      {
         next_segment_brush.CreateHatchBrush(HS_DIAGCROSS,clrBadSegment);
      }
      dc.SelectObject(&girder_pen);
      dc.SelectObject(&next_segment_brush);
      DrawShape(&dc,mapper,nextShape);
      DrawBottomFlange(&dc,mapper,nextBottomFlange);
   }

   Float64 framing_length = pBridge->GetSegmentFramingLength(segmentKey);
   const CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segmentKey.segmentIndex);
   if ( pSegment->AreSegmentVariationsValid(framing_length) )
   {
      this_segment_brush.CreateSolidBrush(SEGMENT_FILL_COLOR);
   }
   else
   {
      this_segment_brush.CreateHatchBrush(HS_DIAGCROSS,clrBadSegment);
   }
   dc.SelectObject(&this_segment_pen);
   dc.SelectObject(&this_segment_brush);
   DrawShape(&dc,mapper,shape);
   DrawBottomFlange(&dc,mapper,bottomFlange);


   dc.SelectObject(pOldPen);
   dc.SelectObject(pOldBrush);
}

void CDrawPrecastSegmentControl::CreateSegmentShape(const CSegmentKey& segmentKey,IShape** ppShape,IPoint2dCollection** ppPoints)
{
   const CSplicedGirderData* pSplicedGirder = m_pSource->GetGirder();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);
   pGirder->GetSegmentProfile(segmentKey,pSplicedGirder,true,ppShape);
   pGirder->GetSegmentBottomFlangeProfile(segmentKey,pSplicedGirder,true,ppPoints);
}

void CDrawPrecastSegmentControl::DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape)
{
   if ( pShape == nullptr )
      return;

   CComPtr<IPoint2dCollection> polypoints;
   pShape->get_PolyPoints(&polypoints);

   CollectionIndexType nPoints;
   polypoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   polypoints->get__Enum(&enum_points);

   ULONG nFetched;
   enum_points->Next((ULONG)nPoints,points,&nFetched);
   ATLASSERT(nFetched == nPoints);

   CPoint* dev_points = new CPoint[nPoints];
   for ( CollectionIndexType i = 0; i < nPoints; i++ )
   {
      LONG dx,dy;
      mapper.WPtoDP(points[i],&dx,&dy);
      dev_points[i] = CPoint(dx,dy);

      points[i]->Release();
   }

   pDC->Polygon(dev_points,(int)nPoints);

   delete[] points;
   delete[] dev_points;
}

void CDrawPrecastSegmentControl::DrawBottomFlange(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPoints)
{
   if ( pPoints == nullptr )
      return;

   CollectionIndexType nPoints;
   pPoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   pPoints->get__Enum(&enum_points);

   ULONG nFetched;
   enum_points->Next((ULONG)nPoints,points,&nFetched);
   ATLASSERT(nFetched == nPoints);

   CPoint* dev_points = new CPoint[nPoints];
   for ( CollectionIndexType i = 0; i < nPoints; i++ )
   {
      LONG dx,dy;
      mapper.WPtoDP(points[i],&dx,&dy);
      dev_points[i] = CPoint(dx,dy);

      points[i]->Release();
   }

   pDC->Polyline(dev_points,(int)nPoints);

   delete[] points;
   delete[] dev_points;
}

BOOL CDrawPrecastSegmentControl::OnEraseBkgnd(CDC* pDC)
{
   CBrush brush(::GetSysColor(COLOR_WINDOW));
   brush.UnrealizeObject();

   CPen pen(PS_SOLID,1,::GetSysColor(COLOR_WINDOW));
   pen.UnrealizeObject();

   CBrush* pOldBrush = pDC->SelectObject(&brush);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CRect rect;
   GetClientRect(&rect);
   pDC->Rectangle(rect);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   return TRUE;

   // default isn't working so we have to do our own erasing
   //return CWnd::OnEraseBkgnd(pDC);
}
