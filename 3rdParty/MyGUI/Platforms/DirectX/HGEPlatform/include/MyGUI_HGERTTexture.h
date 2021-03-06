/*!
	@file
	@author		Albert Semenov
	@date		12/2009
*/

#ifndef __MYGUI_HGE_RTTEXTURE_H__
#define __MYGUI_HGE_RTTEXTURE_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ITexture.h"
#include "MyGUI_RenderFormat.h"
#include "MyGUI_IRenderTarget.h"

#include "hge.h"
/*
struct IDirect3DDevice9;
struct IDirect3DTexture9;
struct IDirect3DSurface9;
*/
namespace MyGUI
{

	class HGERTTexture :
		public IRenderTarget
	{
	public:
		HGERTTexture(HGE* _hge, HTEXTURE _texture);
		virtual ~HGERTTexture();

		virtual void begin();
		virtual void end();

		virtual void doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count);

		virtual const RenderTargetInfo& getInfo()
		{
			return mRenderTargetInfo;
		}

	private:
		HGE* mpHGE;
		HTEXTURE mpTexture;
		/*
		IDirect3DDevice9* mpD3DDevice;
		IDirect3DTexture9* mpTexture;
		IDirect3DSurface9* mpRenderSurface;
		IDirect3DSurface9* mpBackBuffer;
		*/
		RenderTargetInfo mRenderTargetInfo;		
	};

} // namespace MyGUI

#endif // __MYGUI_HGE_RTTEXTURE_H__
