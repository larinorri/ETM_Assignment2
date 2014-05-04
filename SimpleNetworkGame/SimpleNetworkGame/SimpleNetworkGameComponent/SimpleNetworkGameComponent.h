#pragma once

#include "pch.h"
#include "BasicTimer.h"
#include "GameRenderer.h"
#include <DrawingSurfaceNative.h>

namespace PhoneDirect3DXamlAppComponent
{
public delegate void RequestAdditionalFrameHandler();

[Windows::Foundation::Metadata::WebHostHidden]
public ref class Direct3DBackground sealed : public Windows::Phone::Input::Interop::IDrawingSurfaceManipulationHandler
{
public:
	Direct3DBackground();

	Windows::Phone::Graphics::Interop::IDrawingSurfaceBackgroundContentProvider^ CreateContentProvider();

	// IDrawingSurfaceManipulationHandler
	virtual void SetManipulationHost(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ manipulationHost);

	event RequestAdditionalFrameHandler^ RequestAdditionalFrame;

	property Windows::Foundation::Size	WindowBounds;
	property Windows::Foundation::Size	NativeResolution;
	property Windows::Foundation::Size	RenderResolution;
	// Sent to gameplay code for navigation
	property double						MagneticNorth;

	// notify internal objects to start the game
	void StartGame(bool isHost) { m_renderer->StartGame(isHost); }
	
	// Go back to the main menu
	void ResetGame() { m_renderer->ResetGame(); }

	// set callback routine for game over condition
	void SetGameOverCallback(IGameEndedCallback^ callbackObject)
	{
		m_renderer->SetCallBack(callbackObject);
	}

protected:
	// Event Handlers
	void OnPointerPressed(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerReleased(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerMoved(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);

internal:
	HRESULT Connect(_In_ IDrawingSurfaceRuntimeHostNative* host, _In_ ID3D11Device1* device);
	void Disconnect();

	HRESULT PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Inout_ DrawingSurfaceSizeF* desiredRenderTargetSize);
	HRESULT Draw(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView);

private:
	GameRenderer^ m_renderer;
	BasicTimer^ m_timer;
};

}