#include "pch.h"
#include "GameRenderer.h"
#include "NetworkEvents.h"
#include "DDSTextureLoader.h"
#include <ctime>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

GameRenderer::GameRenderer() :
	m_loadingComplete(false),
	m_connected(false),
	m_host(false),
	m_indexCount(0)
{
}

void GameRenderer::CreateDeviceResources()
{
	Direct3DBase::CreateDeviceResources();

	auto loadVSTask = DX::ReadDataAsync("SimpleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync("SimplePixelShader.cso");

	auto createVSTask = loadVSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreateVertexShader(
				fileData->Data,
				fileData->Length,
				nullptr,
				&m_vertexShader
				)
			);

		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_d3dDevice->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				fileData->Data,
				fileData->Length,
				&m_inputLayout
				)
			);
	});

	auto createPSTask = loadPSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreatePixelShader(
				fileData->Data,
				fileData->Length,
				nullptr,
				&m_pixelShader
				)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);
	});

	// create all geometry
	auto createQuadTask = (createPSTask && createVSTask).then([this] () {
		Vertex quadVertices[] = 
		{
			// Full screen quad
			{ XMFLOAT3(-1.0f, -1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 0.5f),	XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 0.5f),	XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.5f),	XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			// floor
			{ XMFLOAT3(-50, 0, -50), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 200.0f, 0.0f) },
			{ XMFLOAT3(-50, 0,  50), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 50, 0, -50), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(200.0f, 200.0f, 0.0f) },
			{ XMFLOAT3( 50, 0,  50), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(200.0f, 0.0f, 0.0f) },
			// celing
			{ XMFLOAT3(-50, 1, -50), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 100.0f, 0.0f) },
			{ XMFLOAT3( 50, 1, -50), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(100.0f, 100.0f, 0.0f) },
			{ XMFLOAT3(-50, 1,  50), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 50, 1,  50), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(100.0f, 0.0f, 0.0f) },
			// outer walls
			{ XMFLOAT3(-50, 0, 50), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-50, 1, 50), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 50, 0, 50), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(100.0f, 1.0f, 0.0f) },
			{ XMFLOAT3( 50, 1, 50), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(100.0f, 0.0f, 0.0f) },
			
			{ XMFLOAT3(-50, 0, -50), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3( 50, 0, -50), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(100.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-50, 1, -50), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 50, 1, -50), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(100.0f, 0.0f, 0.0f) },

			{ XMFLOAT3(-50, 0, -50), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-50, 1, -50), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-50, 0,  50), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(100.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-50, 1,  50), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(100.0f, 0.0f, 0.0f) },

			{ XMFLOAT3(50, 0,  50), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(50, 1,  50), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(50, 0, -50), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(100.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(50, 1, -50), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(100.0f, 0.0f, 0.0f) },
			// inner 4 sided cell
			{ XMFLOAT3(-0.5f, 0, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 0.5f, 0, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, 1, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 0.5f, 1, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

			{ XMFLOAT3(-0.5f, 0, -0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, 1, -0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 0.5f, 0, -0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3( 0.5f, 1, -0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

			{ XMFLOAT3(-0.5f, 0, -0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, 0,  0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, 1, -0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, 1,  0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

			{ XMFLOAT3( 0.5f, 0,  0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 0.5f, 0, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3( 0.5f, 1,  0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3( 0.5f, 1, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = quadVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);

	});

	// launch texture loading task
	createQuadTask.then([this] () {
		
		// make sampler object
		CD3D11_SAMPLER_DESC wrap(D3D11_DEFAULT);
		wrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		wrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		m_d3dDevice->CreateSamplerState(&wrap, &wrapSampler);
		
		// main loading complete	
		m_loadingComplete = true;

		// Load all needed DDS textures (don't have to wait)
		CreateDDSTextureFromFile(m_d3dDevice.Get(), L"Textures/DungeonFloor.dds", nullptr, &floorSRV);
		CreateDDSTextureFromFile(m_d3dDevice.Get(), L"Textures/DungeonWall.dds", nullptr, &wallSRV);
		CreateDDSTextureFromFile(m_d3dDevice.Get(), L"Textures/DungeonCeling.dds", nullptr, &celingSRV);
	});
}

void GameRenderer::CreateWindowSizeDependentResources()
{
	Direct3DBase::CreateWindowSizeDependentResources();

	float aspectRatio = m_windowBounds.Width / m_windowBounds.Height;
	float fovAngleY = 90.0f * XM_PI / 180.0f;

	XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(	
		XMMatrixPerspectiveFovLH(fovAngleY,	aspectRatio, 0.01f,	100.0f)	));
}

void GameRenderer::StartGame(bool isHost)
{
	m_connected = true; 
	RandomizeLevel();
	m_host = isHost;
}

void GameRenderer::RandomizeLevel()
{
	// used to randmoize the dungeon
	unsigned int dungeonSeed = time(0);
	srand(dungeonSeed); // specific random value

	for (unsigned int z = 0; z < 100; ++z)
		for (unsigned int x = 0; x < 100; ++x)
		{
			// only one in 5 cells is filled with brick
			theDungeon[z][x] = (rand() % 5) ? 0 : 1;
		}
	// transmit dungeon to opposing player (seed only)
}

void GameRenderer::Update(float timeTotal, float timeDelta)
{
	//(void) timeDelta; // Unused parameter.

	XMVECTOR eye = XMVectorSet(0.0f, 0.5f, -2.0f, 0.0f);
	XMVECTOR at = XMVectorSet(0.0f, 0.5f, 1.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));

	// update time variables
	m_constantBufferData.time.x = timeTotal;
	m_constantBufferData.time.y = timeDelta;
	
	// only the host should perform game logic..
	if (m_host)
	{
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
		XMStoreFloat4x4(&m_constantBufferData.view, 
			XMMatrixTranspose(XMMatrixInverse(0,XMMatrixRotationY(timeTotal) * XMMatrixTranslation(0, 0.5, -10))));
	}
	if (m_connected)
	{
		// If we are the server & we are connected... lets transmit game object orientation
		if (m_host)
		{
			// create event
			NetworkEvents::EVENT_DATA cubeLocation;
			cubeLocation.ID = GAME_EVENT::SYNC_CUBE;
			memcpy_s(cubeLocation.matrix, sizeof(XMFLOAT4X4), &m_constantBufferData.model, sizeof(XMFLOAT4X4));
			// transmit event
			NetworkEvents::GetInstance().PushOutgoingEvent(&cubeLocation);
		}
		else // if instead we are the client... receive the state of the game objects
		{
			NetworkEvents::EVENT_DATA what;
			ZeroMemory(&what, sizeof(NetworkEvents::EVENT_DATA));
			// if we have an event pending we must deal with it
			if (NetworkEvents::GetInstance().PopIncomingEvent(&what))
			{
				switch (what.ID)
				{
				case GAME_EVENT::SYNC_CUBE :
					// match server cube
					memcpy_s(&m_constantBufferData.model, sizeof(XMFLOAT4X4), what.matrix, sizeof(XMFLOAT4X4));
					//XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
					break;
				}// end switch
			}// end if
		}// end else
	}
}

void GameRenderer::Render()
{
	m_d3dContext->OMSetRenderTargets(1,	m_renderTargetView.GetAddressOf(),	m_depthStencilView.Get());

	const float midnightBlue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), midnightBlue);

	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Only draw the cube once it is loaded (loading is asynchronous).
	if (!m_loadingComplete)
	{
		return;
	}

	
	UINT offset = 0;
	UINT stride = sizeof(Vertex);
	// Set all common attributes
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_d3dContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	m_d3dContext->PSSetSamplers(0, 1, wrapSampler.GetAddressOf());

	// if we are not yet connected render the main menu
	if (!m_connected)
	{
		// copy over identity matrixies only
		ModelViewProjectionConstantBuffer doNothing;
		doNothing.shading = SHADING::MENU;
		doNothing.time = m_constantBufferData.time;
		XMStoreFloat4x4(&doNothing.model, XMMatrixIdentity());
		XMStoreFloat4x4(&doNothing.view, XMMatrixIdentity());
		XMStoreFloat4x4(&doNothing.projection, XMMatrixIdentity());

		m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &doNothing, 0, 0);

		// render FS Quad
		m_d3dContext->Draw(4, 0);
	}
	else // render the game scene
	{
		// send actual matricies to be used
		m_constantBufferData.shading = SHADING::TEXTURED;
		m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);
		
		// render Floor
		if (floorSRV)
			m_d3dContext->PSSetShaderResources(0, 1, floorSRV.GetAddressOf());
		m_d3dContext->Draw(4, 4);

		// render celing
		if (celingSRV)
			m_d3dContext->PSSetShaderResources(0, 1, celingSRV.GetAddressOf());
		m_d3dContext->Draw(4, 8);

		// render outer walls
		// draw each side on its own (don't mix normals)
		m_d3dContext->Draw(4, 12);
		m_d3dContext->Draw(4, 16);
		m_d3dContext->Draw(4, 20);
		m_d3dContext->Draw(4, 24);

		// render all wall sections (dungeon)
		if (wallSRV)
			m_d3dContext->PSSetShaderResources(0, 1, wallSRV.GetAddressOf());
		
		// grab the position of the camera in world space
		XMMATRIX cWorld = XMLoadFloat4x4(&m_constantBufferData.view);
		cWorld = XMMatrixTranspose(cWorld);
		cWorld = XMMatrixInverse(0,cWorld);

		// render the dungeon...
		for (int z = 0; z < 100; ++z)
			for (int x = 0; x < 100; ++x)
				if (theDungeon[z][x])
				{
					// render only items close to the camera...
					XMVECTOR diff = XMVectorSubtract(XMVectorSet(x - 50, 0, z - 50, 0), cWorld.r[3]);
					XMVECTOR lenR = XMVector3LengthSq(diff);
					XMVECTOR dotR = XMVector3Dot(XMVector3Normalize(diff), cWorld.r[2]);
					// culling
					if (XMVectorGetX(lenR) < 25 || (XMVectorGetX(dotR) > 0.75f && XMVectorGetX(lenR) < 25 * 25))
					{

						// place world matrix in proper location
						XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixTranslation(x - 50, 0, z - 50)));
						m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

						// draw a wall segment
						m_d3dContext->Draw(4, 28);
						m_d3dContext->Draw(4, 32);
						m_d3dContext->Draw(4, 36);
						m_d3dContext->Draw(4, 40);
					}
				}

	}
}