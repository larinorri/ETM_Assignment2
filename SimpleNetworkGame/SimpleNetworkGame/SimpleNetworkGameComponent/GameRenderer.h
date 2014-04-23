#pragma once

#include "Direct3DBase.h"

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
	DirectX::XMFLOAT2	time;
	DirectX::XMFLOAT2	padding;
};

struct VertexPositionColor
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 color;
};

// This class renders all objects in the game
ref class GameRenderer sealed : public Direct3DBase
{
public:
	GameRenderer();

	// Direct3DBase methods.
	virtual void CreateDeviceResources() override;
	virtual void CreateWindowSizeDependentResources() override;
	virtual void Render() override;
	
	// Method for updating time-dependent objects.
	void Update(float timeTotal, float timeDelta);

	// Method for notifying current game has started
	void StartGame(bool isHost) { m_connected = true; m_host = isHost; }

private:
	bool m_loadingComplete, m_connected, m_host;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;

	// Load cool gunship model
	Microsoft::WRL::ComPtr<ID3D11Buffer> shipVBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> shipIBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> modelVS;
	
	// Fullscreen quad geo & shader
	Microsoft::WRL::ComPtr<ID3D11VertexShader> quadVS;

	// Uber pixel shader which handles various materials
	Microsoft::WRL::ComPtr<ID3D11PixelShader> uberPS;


	uint32 m_indexCount;
	ModelViewProjectionConstantBuffer m_constantBufferData;
};
