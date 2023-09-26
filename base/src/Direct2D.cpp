#include "Direct2D.h"
#include "ColorPalette.h"

extern ApplicationCore *gp_appCore;

Direct2D::Direct2D(const HWND ah_window, const RECT *const ap_viewRect) :
	mh_window(ah_window)
{
	if (ap_viewRect) {
		mp_viewRect = new RECT;
		*mp_viewRect = *ap_viewRect;
	} 
	else {
		mp_viewRect = nullptr;
	}

	mp_renderTarget = nullptr;
	mp_brush = nullptr;
	mp_strokeStyle = nullptr;

	m_brushColor = RGB_TO_COLORF(NEUTRAL_50);
	m_backgroundColor = RGB_TO_COLORF(NEUTRAL_800);
	m_strokeWidth = 1.0f;
}

Direct2D::~Direct2D()
{
	DestroyDeviceResources();

	if (mp_viewRect) {
		delete mp_viewRect;
	}
}

int Direct2D::Create()
{
	if (!mp_viewRect) {
		RECT viewRect;
		::GetClientRect(mh_window, &viewRect);

		mp_viewRect = new RECT;
		*mp_viewRect = viewRect;
	}

	return static_cast<int>(CreateDeviceResources());
}

void Direct2D::BeginDraw()
{
	// disable the WM_PAINT flag
	::ValidateRect(mh_window, nullptr);

	mp_renderTarget->BeginDraw();
}

void Direct2D::EndDraw()
{
	if (D2DERR_RECREATE_TARGET == mp_renderTarget->EndDraw()) {
		DestroyDeviceResources();
		if (S_OK != CreateDeviceResources()) {
			// TODO:: if create has failed
			return;
		}

		::InvalidateRect(mh_window, mp_viewRect, FALSE);
	}
}

void Direct2D::Clear()
{
	mp_renderTarget->Clear(m_backgroundColor);
}

HRESULT Direct2D::CreateDeviceResources()
{
	// declaring a pointer for a window-based render target and to get its address
	ID2D1HwndRenderTarget *p_hwndRenderTarget;
	D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties();
	properties.dpiX = 96.0f;
	properties.dpiY = 96.0f;
	D2D1_SIZE_U viewSize = {
		static_cast<unsigned int>(mp_viewRect->right - mp_viewRect->left),
		static_cast<unsigned int>(mp_viewRect->bottom - mp_viewRect->top)
	};

	auto factory = gp_appCore->GetFactory();
	if (S_OK != factory->CreateHwndRenderTarget(
		properties, D2D1::HwndRenderTargetProperties(mh_window, viewSize), &p_hwndRenderTarget
	)) {
		return D2DERR_WIN32_ERROR;
	}
	// set its address to a parent interface if HwndRenderTagre is created
	mp_renderTarget = p_hwndRenderTarget;

	ID2D1SolidColorBrush *p_solidBrush;
	if (S_OK != mp_renderTarget->CreateSolidColorBrush(m_brushColor, &p_solidBrush)) {
		InterfaceRelease(&mp_renderTarget);

		return D2DERR_WIN32_ERROR;
	}
	mp_brush = p_solidBrush;

	D2D1_STROKE_STYLE_PROPERTIES storkeStypeProperties = {
		D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND,
		D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND,
		10.0f, D2D1_DASH_STYLE_SOLID, 0.0f
	};

	if (S_OK != factory->CreateStrokeStyle(storkeStypeProperties, nullptr, 0, &mp_strokeStyle)) {
		InterfaceRelease(&mp_renderTarget);
		InterfaceRelease(&mp_brush);

		return D2DERR_WIN32_ERROR;
	}

	return S_OK;
}

void Direct2D::DestroyDeviceResources()
{
	InterfaceRelease(&mp_renderTarget);
	InterfaceRelease(&mp_brush);
	InterfaceRelease(&mp_strokeStyle);
}

ID2D1LinearGradientBrush *const Direct2D::CreateLinearGradientBrush(
	const D2D1_GRADIENT_STOP *const a_gradientStopList,
	const unsigned int gradientStopsCount,
	const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *const a_gradientPositionData
)
{
	ID2D1GradientStopCollection *p_gradientStop = nullptr;
	HRESULT hResult = mp_renderTarget->CreateGradientStopCollection(
		a_gradientStopList, gradientStopsCount,
		D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP,
		&p_gradientStop
	);

	if (S_OK != hResult) {
		return nullptr;
	}

	ID2D1LinearGradientBrush *p_lineGradientBrush;
	hResult = mp_renderTarget->CreateLinearGradientBrush(
		*a_gradientPositionData, p_gradientStop, &p_lineGradientBrush
	);
	if (S_OK != hResult) {
		p_lineGradientBrush = nullptr;
	}

	InterfaceRelease(&p_gradientStop);

	return p_lineGradientBrush;
}

ID2D1StrokeStyle *const Direct2D::CreateUserStrokeStyle(
	const D2D1_DASH_STYLE a_dashStyle, const D2D1_CAP_STYLE a_sideCap,
	const D2D1_CAP_STYLE a_dashCap, const D2D1_LINE_JOIN a_lineJoin,
	const float a_miterLimit, const float a_dashOffset
)
{
	D2D1_STROKE_STYLE_PROPERTIES storkeStypeProperties = {
		a_sideCap, a_sideCap, a_dashCap,
		a_lineJoin, a_miterLimit, a_dashStyle,
		a_dashOffset
	};

	ID2D1StrokeStyle *p_strokeStype;
	if (S_OK != gp_appCore->GetFactory()->CreateStrokeStyle(
		storkeStypeProperties, nullptr, 0, &p_strokeStype
	)) {
		return nullptr;
	}

	return p_strokeStype;
}

void Direct2D::SetBrushColor(const DColor &a_color)
{
	m_brushColor = a_color;
	static_cast<ID2D1SolidColorBrush *>(mp_brush)->SetColor(a_color);
}

void Direct2D::SetBackgroundColor(const DColor &a_backgroundColor)
{
	m_backgroundColor = a_backgroundColor;
}

// returns the previous stroke style. must be released from the user
ID2D1StrokeStyle *const Direct2D::SetStrokeStyle(ID2D1StrokeStyle *const ap_strokeStyle)
{
	ID2D1StrokeStyle *const prevFStrokeStyle = mp_strokeStyle;
	mp_strokeStyle = ap_strokeStyle;

	return prevFStrokeStyle;
}

void Direct2D::SetStrokeWidth(const float a_strokeWidth)
{
	m_strokeWidth = a_strokeWidth;
}

// returns the previous brush. must be released from the user
ID2D1Brush *Direct2D::SetBrush(ID2D1Brush *const ap_brush)
{
	ID2D1Brush *p_prevBrush = mp_brush;
	mp_brush = ap_brush;

	return p_prevBrush;
}

////////////////////////////////////
// drawing methode
////////////////////////////////////

void Direct2D::DrawLine(const DPoint &a_startPoint, const DPoint &a_endPoint)
{
	mp_renderTarget->DrawLine(a_startPoint, a_endPoint, mp_brush, m_strokeWidth, mp_strokeStyle);
}

void Direct2D::DrawRectangle(const DPoint &a_startPoint, const DPoint &a_endPoint)
{
	DrawRectangle(DRect({ a_startPoint.x, a_startPoint.y, a_endPoint.x, a_endPoint.y }));
}

void Direct2D::DrawRectangle(const DRect &a_rect)
{
	mp_renderTarget->DrawRectangle(a_rect, mp_brush, m_strokeWidth, mp_strokeStyle);
}

void Direct2D::DrawRoundedRectangle(const DPoint &a_startPoint, const DPoint &a_endPoint, const float radius)
{
	DrawRoundedRectangle(
		DRect({ a_startPoint.x, a_startPoint.y, a_endPoint.x, a_endPoint.y }),
		radius
	);
}

void Direct2D::DrawRoundedRectangle(const DRect &a_rect, const float radius)
{
	mp_renderTarget->DrawRoundedRectangle(
		D2D1_ROUNDED_RECT({ a_rect, radius, radius }), 
		mp_brush,
		m_strokeWidth,
		mp_strokeStyle
	);
}

void Direct2D::DrawEllipse(const DPoint &a_startPoint, const DPoint &a_endPoint)
{
	const float radiusX = (a_endPoint.x - a_startPoint.x) / 2;
	const float radiusY = (a_endPoint.y - a_startPoint.y) / 2;

	mp_renderTarget->DrawEllipse(
		D2D1_ELLIPSE({
			{ a_startPoint.x + radiusX, a_startPoint.y + radiusY },
			radiusX,
			radiusY
		}), 
		mp_brush,
		m_strokeWidth,
		mp_strokeStyle
	);
}

void Direct2D::DrawEllipse(const DRect &a_rect)
{
	const float radiusX = (a_rect.right - a_rect.left) / 2;
	const float radiusY = (a_rect.bottom - a_rect.top) / 2;

	mp_renderTarget->DrawEllipse(
		D2D1_ELLIPSE({
			{ a_rect.left + radiusX, a_rect.top + radiusY},
			radiusX,
			radiusY
		}),
		mp_brush,
		m_strokeWidth,
		mp_strokeStyle
	);
}

void Direct2D::DrawGeometry(ID2D1Geometry *const ap_geometry)
{
	mp_renderTarget->DrawGeometry(ap_geometry, mp_brush, m_strokeWidth, mp_strokeStyle);
}

void Direct2D::FillRectangle(const DRect &a_rect)
{
	mp_renderTarget->FillRectangle(a_rect, mp_brush);
}

void Direct2D::FillRectangle(const DPoint &a_startPoint, const DPoint &a_endPoint)
{
	FillRectangle(DRect({ a_startPoint.x, a_startPoint.y, a_endPoint.x, a_endPoint.y }));
}

void Direct2D::FillRoundedRectangle(const DRect &a_rect, const float radius)
{
	mp_renderTarget->FillRoundedRectangle(
		D2D1_ROUNDED_RECT({ a_rect, radius, radius }),
		mp_brush
	);
}

void Direct2D::FillRoundedRectangle(const DPoint &a_startPoint, const DPoint &a_endPoint, const float radius)
{
	mp_renderTarget->FillRoundedRectangle(
		D2D1_ROUNDED_RECT({
			{ a_startPoint.x, a_startPoint.y, a_endPoint.x, a_endPoint.y },
			radius,
			radius
		}), 
		mp_brush
	);
}

void Direct2D::FillEllipse(const DRect &a_rect)
{
	const float radiusX = (a_rect.right - a_rect.left) / 2;
	const float radiusY = (a_rect.bottom - a_rect.top) / 2;

	mp_renderTarget->FillEllipse(
		D2D1_ELLIPSE({
			{ a_rect.left + radiusX, a_rect.top + radiusY},
			radiusX,
			radiusY
		}),
		mp_brush
	);
}

void Direct2D::FillGeometry(ID2D1Geometry *const p_geometry)
{
	mp_renderTarget->FillGeometry(p_geometry, mp_brush);
}
