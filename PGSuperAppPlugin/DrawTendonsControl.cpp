// DrawTendonsControl.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperColors.h"
#include "DrawTendonsControl.h"
#include <IFace\Alignment.h>
#include <IFace\Bridge.h>

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PrecastSegmentData.h>

// CDrawTendonsControl

IMPLEMENT_DYNAMIC(CDrawTendonsControl, CWnd)

CDrawTendonsControl::CDrawTendonsControl()
{
   m_pSource = NULL;
}

CDrawTendonsControl::~CDrawTendonsControl()
{
}


BEGIN_MESSAGE_MAP(CDrawTendonsControl, CWnd)
   ON_WM_PAINT()
END_MESSAGE_MAP()



// CDrawTendonsControl message handlers
void CDrawTendonsControl::CustomInit(IGirderSegmentDataSource* pSource)
{
   m_pSource = pSource;
}


void CDrawTendonsControl::OnPaint()
{
   CPaintDC dc(this); // device context for painting
   // TODO: Add your message handler code here
   // Do not call CWnd::OnPaint() for painting messages

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderSegment,pGirderSegment);

   const CGirderKey& girderKey = m_pSource->GetGirderKey();
   const CSplicedGirderData* pGirder = m_pSource->GetGirder();
   SegmentIndexType nSegments = pGirder->GetSegmentCount();

   std::vector<CComPtr<IShape>> segmentShapes;
   //std::vector<CComPtr<IPoint2dCollection>> bottomFlangeProfiles;
   CComPtr<IRect2d> bounding_box;
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      CComPtr<IShape> shape;
      pGirderSegment->GetSegmentProfile( segmentKey, pGirder, true, &shape);

      //CComPtr<IPoint2dCollection> bottomFlange;
      //pGirderSegment->GetSegmentBottomFlangeProfile( segmentKey, pGirder, &bottomFlange);

      if ( segIdx == 0 )
      {
         shape->get_BoundingBox(&bounding_box);
      }
      else
      {
         CComPtr<IRect2d> bbox;
         shape->get_BoundingBox(&bbox);
         bounding_box->Union(bbox);
      }

      segmentShapes.push_back(shape);
      //bottomFlangeProfiles.push_back(bottomFlange);
   }

   // Create a poly line for each tendon. 
   std::vector<CComPtr<IPoint2dCollection>> ducts;
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeometry);
   CollectionIndexType nDucts = pGirder->GetPostTensioning()->GetDuctCount();
   for ( CollectionIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CComPtr<IPoint2dCollection> ductPoints;
      pTendonGeometry->GetDuctCenterline(girderKey,ductIdx,pGirder,&ductPoints);

      ducts.push_back(ductPoints);
   }


   //
   // Set up coordinate mapping
   //
   Float64 left,right,top,bottom;
   bounding_box->get_Left(&left);
   bounding_box->get_Right(&right);
   bounding_box->get_Top(&top);
   bounding_box->get_Bottom(&bottom);

   gpRect2d box(left,bottom,right,top);
   gpSize2d size = box.Size();
   gpPoint2d org = box.Center();

   CRect rClient;
   GetClientRect(&rClient);
   CSize sClient = rClient.Size();

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   //mapper.SetMappingMode(grlibPointMapper::Anisotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(sClient.cx,sClient.cy);
   mapper.SetDeviceOrg(sClient.cx/2,sClient.cy/2);

   CPen pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush brush(SEGMENT_FILL_COLOR);
   CPen* pOldPen = dc.SelectObject(&pen);
   CBrush* pOldBrush = dc.SelectObject(&brush);

   CFont* pFont = GetParent()->GetFont();
   CFont* pOldFont = dc.SelectObject(pFont);

   dc.SetBkMode(TRANSPARENT);
   dc.SetTextAlign(TA_BOTTOM | TA_CENTER);

   // Draw the precast girder segments
   SegmentIndexType segIdx = 0;
   std::vector<CComPtr<IShape>>::iterator shapeIter(segmentShapes.begin());
   std::vector<CComPtr<IShape>>::iterator shapeIterEnd(segmentShapes.end());
   for ( ; shapeIter != shapeIterEnd; shapeIter++ )
   {
      CComPtr<IShape> shape = *shapeIter;
      DrawShape(&dc,mapper,shape);


      CComPtr<IShapeProperties> props;
      shape->get_ShapeProperties(&props);

      CComPtr<IPoint2d> cg;
      props->get_Centroid(&cg);

      long dx,dy;
      mapper.WPtoDP(cg,&dx,&dy);
      CString str;
      str.Format(_T("%d"),LABEL_SEGMENT(segIdx++));
      dc.TextOut(dx,dy,str);
   }

   //// Draw bottom flange line
   //std::vector<CComPtr<IPoint2dCollection>>::iterator bfIter(bottomFlangeProfiles.begin());
   //std::vector<CComPtr<IPoint2dCollection>>::iterator bfIterEnd(bottomFlangeProfiles.end());
   //for ( ; bfIter != bfIterEnd; bfIter++ )
   //{
   //   CComPtr<IPoint2dCollection> points = *bfIter;
   //   Draw(&dc,mapper,points,FALSE);
   //}

   // Draw the tendons
   CPen tendonColor(PS_SOLID,1,TENDON_LINE_COLOR);
   dc.SelectObject(&tendonColor);
   std::vector<CComPtr<IPoint2dCollection>>::iterator ductIter(ducts.begin());
   std::vector<CComPtr<IPoint2dCollection>>::iterator ductIterEnd(ducts.end());
   for ( ; ductIter != ductIterEnd; ductIter++ )
   {
      CComPtr<IPoint2dCollection> points = *ductIter;
      Draw(&dc,mapper,points,FALSE);
   }

   // Clean up
   dc.SelectObject(pOldPen);
   dc.SelectObject(pOldBrush);
   dc.SelectObject(pOldFont);
}

void CDrawTendonsControl::DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape)
{
   CComPtr<IPoint2dCollection> polypoints;
   pShape->get_PolyPoints(&polypoints);

   Draw(pDC,mapper,polypoints,TRUE);
}

void CDrawTendonsControl::Draw(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon)
{
   CollectionIndexType nPoints;
   pPolyPoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   pPolyPoints->get__Enum(&enum_points);

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

   if ( bPolygon )
      pDC->Polygon(dev_points,(int)nPoints);
   else
      pDC->Polyline(dev_points,(int)nPoints);

   delete[] points;
   delete[] dev_points;
}
