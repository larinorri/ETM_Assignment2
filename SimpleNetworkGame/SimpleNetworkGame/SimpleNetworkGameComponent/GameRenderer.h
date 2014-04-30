#pragma once

#include "Direct3DBase.h"

// 32 diffrent shading options
enum SHADING 
{
	MENU = 0,
	TEXTURED = 1,
	LIGHTING = 2,
};

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
	DirectX::XMFLOAT2	time;
	unsigned int		shading;
	unsigned int		padding;
};

struct SpotLight
{
	DirectX::XMFLOAT3	coneDir;
	float				innerCone;
	DirectX::XMFLOAT3	lightPos;
	float				outerCone;
	DirectX::XMFLOAT3	lightColor;
	float				radius;
};

struct PointLight
{
	DirectX::XMFLOAT3	lightPos;
	float				radius;
	DirectX::XMFLOAT4	lightColor;
};

struct LightDataConstantBuffer
{
	SpotLight	lanterns[2];
	PointLight	ambiance[2];
};

struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 texcoord;
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

	// Provides compass setting
	void UpdateCompass(double _magneticNorth) 
	{	magneticNorth = _magneticNorth;	}

	// Method for notifying current game has started
	void StartGame(bool isHost);

	// re-seeds the level, transmits it to the other player
	void RandomizeLevel();

private:
	bool m_loadingComplete, m_connected, m_host;
	// which way is north (in degrees)
	double magneticNorth;

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

	// various textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorSRV, wallSRV, celingSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> wrapSampler;

	uint32 m_indexCount;
	ModelViewProjectionConstantBuffer m_constantBufferData;

	// light information for rendering
	LightDataConstantBuffer	lightConstantData;
	Microsoft::WRL::ComPtr<ID3D11Buffer> lightConstantBuffer;

	// the dungeon is a 100x100 cell level
	unsigned char theDungeon[100][100];

	// The players
	DirectX::XMFLOAT4X4	playerLocal;
	DirectX::XMFLOAT4X4	playerRemote;

};


// treasure racer plan:

//done
// allocate grid, each cell is empty or filled  
// render floor & celing of dungeon using texture repeat.(if time, clip to frustum corners)
// loop through cells, non-empty cells have 4 walls, test vs frustum & draw.

// next
// player is the camera in world space, attatch flash light (spotlight)
// Use the compass to control the player. (don't walk through walls...) 

// last
// draw a compass 2D graphic. (rotate based on sensor)
// Pick a random location for the treasure...
// When you shine the light on the treasure you win! send lose event to opponent.
// 
//