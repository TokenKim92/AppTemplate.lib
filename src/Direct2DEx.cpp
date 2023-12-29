#include "Direct2DEx.h"

extern ApplicationCore *gp_appCore;

Direct2DEx::Direct2DEx(const HWND ah_window, const RECT *const ap_viewRect) :
	Direct2D(ah_window, ap_viewRect)
{
	mp_textFormat = nullptr;
	mp_fontFace = nullptr;

	m_fontFormat.name = DEFAULT_FONT_NAME;
	m_fontFormat.size = 20.0f;
}

Direct2DEx::~Direct2DEx()
{
	DestroyDeviceResources();
}

HRESULT Direct2DEx::CreateDeviceResources()
{
	if (S_OK != Direct2D::CreateDeviceResources()) {
		return D2DERR_WIN32_ERROR;
	}

	// create a instance for a string output format 
	mp_textFormat = CreateTextFormat(
		m_fontFormat.name.c_str(), m_fontFormat.size, m_fontFormat.weight, m_fontFormat.style
	);
	if (nullptr == mp_textFormat) {
		return D2DERR_WIN32_ERROR;
	}

	mp_fontFace = CreateFontFace(
		m_fontFormat.name.c_str(), m_fontFormat.weight, m_fontFormat.style
	);
	if (nullptr == mp_fontFace) {
		InterfaceRelease(&mp_textFormat);

		return D2DERR_WIN32_ERROR;
	}

	return S_OK;
}

void Direct2DEx::DestroyDeviceResources()
{
	InterfaceRelease(&mp_textFormat);
	InterfaceRelease(&mp_fontFace);

	Direct2D::DestroyDeviceResources();
}

IDWriteTextFormat *Direct2DEx::CreateTextFormat(
	const wchar_t *ap_fontName, float a_fontSize, DWRITE_FONT_WEIGHT a_fontWeight,
	DWRITE_FONT_STYLE a_fontStyle, DWRITE_FONT_STRETCH a_fontStretch, const wchar_t *ap_localName
)
{
	IDWriteTextFormat *p_textFormat = nullptr;
	if (S_OK != gp_appCore->GetWriteFactory()->CreateTextFormat(
		ap_fontName, nullptr, a_fontWeight, a_fontStyle,
		a_fontStretch, a_fontSize, ap_localName, &p_textFormat
	)) {
		return nullptr;
	}

	return p_textFormat;
}

IDWriteFontFace *Direct2DEx::CreateFontFace(const wchar_t *const ap_name, const DWRITE_FONT_WEIGHT a_weight, const DWRITE_FONT_STYLE a_style)
{
	IDWriteGdiInterop *p_gdiInterop = nullptr;
	if (S_OK == gp_appCore->GetWriteFactory()->GetGdiInterop(&p_gdiInterop)) {
		LOGFONT logFont = {};
		wcscpy_s(logFont.lfFaceName, ap_name);
		logFont.lfWeight = static_cast<long>(a_weight);
		logFont.lfCharSet = DEFAULT_CHARSET;
		logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
		logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		logFont.lfQuality = ANTIALIASED_QUALITY;
		logFont.lfPitchAndFamily = VARIABLE_PITCH;
		logFont.lfItalic = 
			a_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC || 
			a_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE;

		IDWriteFont *font;
		if (S_OK == p_gdiInterop->CreateFontFromLOGFONT(&logFont, &font)) {
			IDWriteFontFace *p_fontFace;

			if (S_OK == font->CreateFontFace(&p_fontFace)) {
				InterfaceRelease(&font);
				InterfaceRelease(&p_gdiInterop);

				return p_fontFace;
			}

			InterfaceRelease(&font);
		}

		InterfaceRelease(&p_gdiInterop);
	}

	return nullptr;
}

ID2D1PathGeometry *Direct2DEx::CreateTextPathGeometry(const wchar_t *const ap_text, const float a_fontSize)
{
	const size_t textLength = wcslen(ap_text);
	unsigned int *p_codePoints = new unsigned int[textLength];
	unsigned short *p_glyphIndices = new unsigned short[textLength];

	for (size_t i = 0; i < textLength; i++) {
		p_codePoints[i] = static_cast<unsigned int>(ap_text[i]);
	}
	mp_fontFace->GetGlyphIndicesW(p_codePoints, textLength, p_glyphIndices);

	ID2D1PathGeometry *p_pathGeometry;
	bool result = false;

	if (S_OK == gp_appCore->GetFactory()->CreatePathGeometry(&p_pathGeometry)) {
		ID2D1GeometrySink *p_sink = nullptr;
		if (S_OK == p_pathGeometry->Open(&p_sink)) {
			if (S_OK == mp_fontFace->GetGlyphRunOutline(
				a_fontSize, p_glyphIndices,
				nullptr, nullptr,
				textLength, false,
				false, p_sink
			)) {
				result = true;
			}
			p_sink->Close();
			InterfaceRelease(&p_sink);
		}
	}

	if (!result) {
		InterfaceRelease(&p_pathGeometry);
	}
	delete[] p_codePoints;
	delete[] p_glyphIndices;

	return p_pathGeometry;
}

bool Direct2DEx::SetFontFormat(const FONT_FORMAT &a_fontFormat)
{
	IDWriteTextFormat *const p_textFormat = CreateTextFormat(
		a_fontFormat.name.c_str(), a_fontFormat.size, a_fontFormat.weight, a_fontFormat.style
	);
	IDWriteFontFace *const p_fontFace = CreateFontFace(
		a_fontFormat.name.c_str(), a_fontFormat.weight, a_fontFormat.style
	);

	if (nullptr != p_textFormat && nullptr != p_fontFace) {
		InterfaceRelease(&mp_textFormat);
		InterfaceRelease(&mp_fontFace);

		m_fontFormat = a_fontFormat;
		mp_textFormat = p_textFormat;
		mp_fontFace = p_fontFace;

		return true;
	}

	return false;
}

bool Direct2DEx::SetFontName(const wchar_t *const ap_name)
{
	const FONT_FORMAT fontFormat({ ap_name, m_fontFormat.size, m_fontFormat.weight, m_fontFormat.style });
	return SetFontFormat(fontFormat);
}

bool Direct2DEx::SetFontSize(const float a_size)
{
	const FONT_FORMAT fontFormat({ m_fontFormat.name, a_size, m_fontFormat.weight, m_fontFormat.style });
	return SetFontFormat(fontFormat);
}

bool Direct2DEx::SetFontWeight(const DWRITE_FONT_WEIGHT a_weight)
{
	const FONT_FORMAT fontFormat({ m_fontFormat.name, m_fontFormat.size, a_weight, m_fontFormat.style });
	return SetFontFormat(fontFormat);
}

bool Direct2DEx::SetFontStyle(const DWRITE_FONT_STYLE a_style)
{
	const FONT_FORMAT fontFormat({ m_fontFormat.name, m_fontFormat.size, m_fontFormat.weight, a_style });
	return SetFontFormat(fontFormat);
}

void Direct2DEx::SetTextAlignment(const DWRITE_TEXT_ALIGNMENT a_hType, const DWRITE_PARAGRAPH_ALIGNMENT a_vType)
{
	mp_textFormat->SetTextAlignment(a_hType);
	mp_textFormat->SetParagraphAlignment(a_vType);
}

IDWriteTextFormat *const Direct2DEx::SetTextFormat(IDWriteTextFormat *const ap_textFormat)
{
	IDWriteTextFormat *const prevTextFormat = mp_textFormat;
	mp_textFormat = ap_textFormat;

	return prevTextFormat;
}

IDWriteFontFace *const Direct2DEx::SetFontFace(IDWriteFontFace *const ap_fontFace)
{
	IDWriteFontFace *const prevFontFace = mp_fontFace;
	mp_fontFace = ap_fontFace;

	return prevFontFace;
}

DSize Direct2DEx::GetTextExtent(const wchar_t *const ap_str, const float a_maxWidth, const float a_maxHeight)
{
	IDWriteTextLayout *p_textLayout;
	DWRITE_TEXT_METRICS textMetrics;
	DSize displaySize = { 0, 0 };
	const float maxWidth = a_maxWidth
		? a_maxWidth
		: static_cast<float>(mp_viewRect->right - mp_viewRect->left);
	const float maxHeight = a_maxHeight
		? a_maxHeight
		: static_cast<float>(mp_viewRect->bottom - mp_viewRect->top);
	const unsigned int strLength = static_cast<unsigned int>(wcslen(ap_str));

	if (S_OK == gp_appCore->GetWriteFactory()->CreateTextLayout(
		ap_str, strLength, mp_textFormat, maxWidth, maxHeight, &p_textLayout
	)) {
		if (S_OK == p_textLayout->GetMetrics(&textMetrics)) {
			displaySize.width = textMetrics.widthIncludingTrailingWhitespace;
			displaySize.height = textMetrics.height;
		}
		p_textLayout->Release();
	}

	if (!a_maxWidth) return displaySize;

	// if the text contains a space(-s), this should be replaces as a other letter and get the text extent again
	// to get a right text extent
	if (displaySize.width > a_maxWidth) {
		auto IsWhiteSpace = [](const wchar_t a_letter) {
			wchar_t whiteSpaceList[] = { L' ', L'\t', L'\n', L'\v', L'\f', L'\r' };
			for (auto whiteSpace : whiteSpaceList) {
				if (a_letter == whiteSpace) {
					return true;
				}
			}
			return false;
		};

		std::wstring text(ap_str);
		// the all space should be replaced a character to get the right size
		for (auto &letter : text) {
			if (!IsWhiteSpace(letter)) {
				break;
			}
			letter = L'1';
		}

		if (S_OK == gp_appCore->GetWriteFactory()->CreateTextLayout(
			text.c_str(), strLength, mp_textFormat, maxWidth, maxHeight, &p_textLayout
		)) {
			if (S_OK == p_textLayout->GetMetrics(&textMetrics)) {
				displaySize.width = textMetrics.widthIncludingTrailingWhitespace;
				displaySize.height = textMetrics.height;
			}
			p_textLayout->Release();
		}
	}

	return displaySize;
}

////////////////////////////////////
// drawing methode
////////////////////////////////////

void Direct2DEx::DrawUserText(const wchar_t *const ap_text, const DRect &ap_rect)
{
	mp_renderTarget->DrawText(ap_text, wcslen(ap_text), mp_textFormat, ap_rect, mp_brush);
}

DRect Direct2DEx::DrawTextOutline(const wchar_t *const ap_text, const DPoint &a_startPos, const float a_textHeight)
{
	ID2D1PathGeometry *p_textPathGeometry = CreateTextPathGeometry(ap_text, m_fontFormat.size);
	
	DRect rect;
	p_textPathGeometry->GetBounds(nullptr, &rect);
	rect.left = a_startPos.x;
	rect.right = rect.left + (rect.right - rect.left) + m_strokeWidth;

	const float textHeight = 0 == a_textHeight
		? m_fontFormat.size
		: a_textHeight;

	mp_renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(a_startPos.x, a_startPos.y + (textHeight + rect.bottom - rect.top) * 0.5f));
	mp_renderTarget->DrawGeometry(p_textPathGeometry, mp_brush, m_strokeWidth, mp_strokeStyle);
	mp_renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0.0f, 0.0f));

	InterfaceRelease(&p_textPathGeometry);

	return rect;
}