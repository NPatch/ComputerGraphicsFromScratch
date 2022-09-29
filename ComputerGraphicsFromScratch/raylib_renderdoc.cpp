#include "3rdparty/renderdoc_app.h"
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include "raylib_renderdoc.h"

pRENDERDOC_GetAPI RENDERDOC_GetAPI;
static RENDERDOC_API_1_1_2* rdoc_api = NULL; 
static void* renderdoc_dll = NULL;

void* LoadRenderDoc()
{
	if (NULL != rdoc_api)
	{
		return renderdoc_dll;
	}

	void* renderdoc_dll = (void*)LoadLibraryA("renderdoc.dll");

	if (NULL == renderdoc_dll)
	{
		printf("GetModuleHandle failed (%d)\n", GetLastError());
	}


	if(NULL != renderdoc_dll)
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI =
		(pRENDERDOC_GetAPI)GetProcAddress((HMODULE)renderdoc_dll, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
		assert(ret == 1);
	}
	else 
	{
		renderdoc_dll = NULL;
	}

	return renderdoc_dll;
}

void UnloadRenderDoc()
{
	if (NULL != renderdoc_dll)
	{
		// Once RenderDoc is loaded there shouldn't be calls
		// to Shutdown or unload RenderDoc DLL.
		// https://github.com/bkaradzic/bgfx/issues/1192
		//
		// rdoc_api->Shutdown();
	}
}

void RenderDocBeginFrameCapture() 
{
	// To start a frame capture, call StartFrameCapture.
	// You can specify NULL, NULL for the device to capture on if you have only one device and
	// either no windows at all or only one window, and it will capture from that device.
	// See the documentation below for a longer explanation
	if (NULL != rdoc_api)
	{
		rdoc_api->StartFrameCapture(NULL, NULL);
	}
}

void RenderDocEndFrameCapture()
{
	// stop the capture
	if (NULL != rdoc_api)
	{
		rdoc_api->EndFrameCapture(NULL, NULL);
	}
}


void RenderDocTriggerFrameCapture()
{
	if (NULL != rdoc_api)
	{
		rdoc_api->TriggerCapture();
	}
}