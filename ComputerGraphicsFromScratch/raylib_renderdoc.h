/**********************************************************************************************
*
*   RenderDoc integration
*
**********************************************************************************************/

#ifndef RAYLIB_RENDERDOC_H
#define RAYLIB_RENDERDOC_H

void* LoadRenderDoc();
void UnloadRenderDoc();

bool RenderDocIsFrameCapturing();
void RenderDocBeginFrameCapture();
void RenderDocEndFrameCapture();
#endif //RAYLIB_RENDERDOC_H