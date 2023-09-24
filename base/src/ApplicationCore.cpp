#include "ApplicationCore.h"

// to use D2D functions
#pragma comment(lib, "D2D1.lib")	// to draw
#pragma comment(lib, "DWRITE.lib")	// to write

ApplicationCore *gp_appCore;

ApplicationCore::ApplicationCore(HINSTANCE ah_instance)
{
	mh_instance = ah_instance;
	gp_appCore = this;

	mp_factory = nullptr;
	mp_wirteFactory = nullptr;
	mp_wicFactory = nullptr;
}

ApplicationCore::~ApplicationCore()
{
	InterfaceRelease(&mp_factory);
	InterfaceRelease(&mp_wirteFactory);
	InterfaceRelease(&mp_wicFactory);

	CoUninitialize();
}

const int ApplicationCore::Create()
{
	// call a function to initialize COM	
	int hResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (S_OK != hResult) {
		return hResult;
	}

	// create a factory instance to use D2D
	hResult = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &mp_factory);
	if (S_OK != hResult) {
		CoUninitialize();

		return hResult;
	}

	// create a write factory instance for string output
	hResult = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(mp_wirteFactory),
		reinterpret_cast<IUnknown **>(&mp_wirteFactory)
	);
	if (S_OK != hResult) {
		mp_factory->Release();
		CoUninitialize();

		return hResult;
	}
	
	// an object that creates various window imaging components
	hResult = CoCreateInstance(
		CLSID_WICImagingFactory, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mp_wicFactory)
	);
	if (S_OK != hResult) {
		mp_factory->Release();
		mp_wirteFactory->Release();
		CoUninitialize();

		return hResult;
	}

	return S_OK;
}

ID2D1Factory *const ApplicationCore::GetFactory()
{
	return mp_factory;
}

IDWriteFactory *const ApplicationCore::GetWriteFactory()
{
	return mp_wirteFactory;
}

IWICImagingFactory *const ApplicationCore::GetWICFactory()
{
	return mp_wicFactory;
}

const HINSTANCE ApplicationCore::GetHandleInstance()
{
	return mh_instance;
}