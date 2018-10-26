#pragma once

#include <GraphicsLib\PointMapper.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\SegmentKey.h>

interface IGirderSegmentDataSource
{
public:
   virtual const CSplicedGirderData* GetGirder() = 0;
   virtual const CGirderKey& GetGirderKey() = 0;
};


// CDrawTendonsControl

class CDrawTendonsControl : public CWnd
{
	DECLARE_DYNAMIC(CDrawTendonsControl)

public:
	CDrawTendonsControl();
	virtual ~CDrawTendonsControl();

   void CustomInit(IGirderSegmentDataSource* pSource);

   afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
   IGirderSegmentDataSource* m_pSource;

   void DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape);
   void Draw(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon);
};


