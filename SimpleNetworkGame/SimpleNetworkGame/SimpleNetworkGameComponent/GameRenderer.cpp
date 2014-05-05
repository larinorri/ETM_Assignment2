#include "pch.h"
#include "GameRenderer.h"
#include "NetworkEvents.h"
#include "DDSTextureLoader.h"
#include <ctime>

// Model Source code
#include "Models/Treasure.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace PhoneDirect3DXamlAppComponent;

GameRenderer::GameRenderer() :
	m_loadingComplete(false),
	m_connected(false),
	m_host(false),
	m_pause(true),
	m_indexCount(0)
{
	// Initialize non device dependent resources
	for (unsigned int i = 0; i < 2; ++i)
	{
		lightConstantData.lanterns[i].coneDir = XMFLOAT3(0, 0, 1);
		lightConstantData.lanterns[i].innerCone = 0.75f;
		lightConstantData.lanterns[i].lightPos = XMFLOAT3(0, 0, 0);
		lightConstantData.lanterns[i].outerCone = 0.7f;
		lightConstantData.lanterns[i].lightColor = XMFLOAT3(0.8, 1, 1);
		lightConstantData.lanterns[i].radius = 10;
	}
	// your opponents spotlight appears faintly red
	lightConstantData.lanterns[1].lightColor = XMFLOAT3(1, 0.8, 0.8);
	
	XMStoreFloat4x4(&playerLocal, XMMatrixIdentity());
	XMStoreFloat4x4(&theTreasure, XMMatrixTranslation(999,999,999));
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
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

		constantBufferDesc.ByteWidth = sizeof(LightDataConstantBuffer);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			&lightConstantBuffer
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
			{ XMFLOAT3(-DUNGEON_EDGE, 0, -DUNGEON_EDGE), XMFLOAT3(0.0f, DUNGEON_SIZE*2.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-DUNGEON_EDGE, 0, DUNGEON_EDGE),  XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 0, -DUNGEON_EDGE),  XMFLOAT3(DUNGEON_SIZE*2.0f, DUNGEON_SIZE*2.0f, 0.0f),	XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 0, DUNGEON_EDGE),   XMFLOAT3(DUNGEON_SIZE*2.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			// celing
			{ XMFLOAT3(-DUNGEON_EDGE, 1, -DUNGEON_EDGE), XMFLOAT3(0.0f, DUNGEON_SIZE, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 1, -DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, DUNGEON_SIZE, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
			{ XMFLOAT3(-DUNGEON_EDGE, 1, DUNGEON_EDGE), XMFLOAT3(0.0f, 0.0f, 0.0f),	XMFLOAT3(0.0f, -1.0f, 0.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 1, DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
			// outer walls
			{ XMFLOAT3(-DUNGEON_EDGE, 0, DUNGEON_EDGE), XMFLOAT3(0.0f, 1.0f, 0.0f),	XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(-DUNGEON_EDGE, 1, DUNGEON_EDGE), XMFLOAT3(0.0f, 0.0f, 0.0f),	XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 0, DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 1, DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

			{ XMFLOAT3(-DUNGEON_EDGE, 0, -DUNGEON_EDGE), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 0, -DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 1.0f, 0.0f),	XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-DUNGEON_EDGE, 1, -DUNGEON_EDGE), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 1, -DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 0.0f, 0.0f),	XMFLOAT3(0.0f, 0.0f, 1.0f) },

			{ XMFLOAT3(-DUNGEON_EDGE, 0, -DUNGEON_EDGE), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-DUNGEON_EDGE, 1, -DUNGEON_EDGE), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-DUNGEON_EDGE, 0, DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 1.0f, 0.0f),	XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-DUNGEON_EDGE, 1, DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 0.0f, 0.0f),	XMFLOAT3(1.0f, 0.0f, 0.0f) },

			{ XMFLOAT3(DUNGEON_EDGE, 0, DUNGEON_EDGE), XMFLOAT3(0.0f, 1.0f, 0.0f),	XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 1, DUNGEON_EDGE), XMFLOAT3(0.0f, 0.0f, 0.0f),	XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 0, -DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 1.0f, 0.0f),	XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(DUNGEON_EDGE, 1, -DUNGEON_EDGE), XMFLOAT3(DUNGEON_SIZE, 0.0f, 0.0f),	XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			// inner 4 sided cell
			{ XMFLOAT3(0, 0, 1), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(1, 0, 1), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(0, 1, 1), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },

			{ XMFLOAT3(0, 0, 0), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(0, 1, 0), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(1, 0, 0), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(1, 1, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

			{ XMFLOAT3(0, 0, 0), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0, 0, 1), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0, 1, 0), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0, 1, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

			{ XMFLOAT3(1, 0, 1), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1, 0, 0), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1, 1, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

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
		CreateDDSTextureFromFile(m_d3dDevice.Get(), L"Textures/GoldPile.dds", nullptr, &treasureSRV);

		// load treasure model
		D3D11_SUBRESOURCE_DATA bufferData = { 0,0,0 };
		bufferData.pSysMem = Treasure_data;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(Treasure_data), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed( m_d3dDevice->CreateBuffer( &vertexBufferDesc, &bufferData,	&treasureVB ));

		bufferData.pSysMem = Treasure_indicies;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(Treasure_indicies), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&indexBufferDesc, &bufferData, &treasureIB));

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
	if (isHost)
	{
		unsigned int dSeed = time(0);
		// transmit dungeon to opposing player (seed only)
		NetworkEvents::EVENT_DATA buildDungeon;
		buildDungeon.ID = GAME_EVENT::SEED_LEVEL;
		buildDungeon.integerUnsigned = dSeed;
		// transmit event
		NetworkEvents::GetInstance().PushOutgoingEvent(&buildDungeon);
		// make our own level
		RandomizeLevel(dSeed);
		// UnPause the game
		m_pause = false;
	}
	m_connected = true; // begin the gameplay
	m_host = isHost;
}

void GameRenderer::ResetGame()
{
	// clear any cached events
	NetworkEvents::GetInstance().ClearIncomingEvents();
	NetworkEvents::GetInstance().ClearOutgoingEvents();
	// Send network kill event to ourselves
	NetworkEvents::EVENT_DATA killConnection;
	killConnection.ID = GAME_EVENT::NETWORK_KILL;
	NetworkEvents::GetInstance().PushOutgoingEvent(&killConnection);
	// show menu again
	m_connected = m_host = false;
	m_pause = true;
	// hide treasure for reset
	XMStoreFloat4x4(&theTreasure, XMMatrixTranslation(999, 999, 999));
}

void GameRenderer::RandomizeLevel(unsigned int dungeonSeed)
{
	// used to randmoize the dungeon
	srand(dungeonSeed); // specific random value

	for (unsigned int z = 0; z < DUNGEON_SIZE; ++z)
		for (unsigned int x = 0; x < DUNGEON_SIZE; ++x)
		{
			// only one in 5 cells is filled with brick
			theDungeon[z][x] = (rand() % 5) ? 0 : 1;
		}
	
	// find a random location for the treasure to spawn
	int startCellX, startCellZ;
	do
	{
		startCellX = rand() % DUNGEON_SIZE;
		startCellZ = rand() % DUNGEON_SIZE;
	} while (theDungeon[startCellZ][startCellX]);
	// move matrix to this location, offset of 0.5 to center model in cell
	XMMATRIX move = XMMatrixTranslation(startCellX - DUNGEON_EDGE + 0.5f, 0.0f, startCellZ - DUNGEON_EDGE + 0.5f);
	move = XMMatrixRotationY(XM_2PI * (rand() / float(RAND_MAX))) * move;
	XMStoreFloat4x4(&theTreasure, move);

	// re-randomize
	srand((unsigned int)time(nullptr));
	// find a random spot for the player to start
	do
	{
		startCellX = rand() % DUNGEON_SIZE;
		startCellZ = rand() % DUNGEON_SIZE;
	} while (theDungeon[startCellZ][startCellX]);
	// move matrix to this location
	move = XMMatrixTranslation(startCellX - DUNGEON_EDGE + 0.5f, 0.5f, startCellZ - DUNGEON_EDGE + 0.5f);
	move = XMMatrixRotationY(XM_2PI * (rand() / float(RAND_MAX))) * move;
	XMStoreFloat4x4(&playerLocal, move);
}

void GameRenderer::Update(float timeTotal, float timeDelta)
{
	//(void) timeDelta; // Unused parameter.

	XMVECTOR eye = XMVectorSet(0.0f, 0.5f, -0.0f, 0.0f);
	XMVECTOR at = XMVectorSet(0.0f, 0.5f, 1.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));

	// update time variables
	m_constantBufferData.time.x = timeTotal;
	m_constantBufferData.time.y = timeDelta;

	// The host & client are responsible for their own game logic..
	// They transmit their positions to each other
	if (m_connected && !m_pause)
	{
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
		// amount of movement for testing penetration of walls
		const float penetration = 0.25f;

		// rotate player 1 based on compass
		XMMATRIX racer = XMLoadFloat4x4(&playerLocal);
		// rotate based on magnetic north
		XMVECTOR currPos = racer.r[3];
		racer = XMMatrixRotationY(XMConvertToRadians(magneticNorth));
		racer.r[3] = currPos;
		
		// TEMP CODE (emulator has no compass so we transfer our own)
		if (m_host == false)
		{
			NetworkEvents::EVENT_DATA remoteCompass;
			remoteCompass.ID = GAME_EVENT::COMPASS_SEND;
			remoteCompass.matrix[0] = magneticNorth;// our compass reading
			NetworkEvents::GetInstance().PushOutgoingEvent(&remoteCompass);
		}

		// create Z axis world space velocity vector
		XMVECTOR velocityW = racer.r[2];
		velocityW = XMVectorSetW(velocityW, 0); // trim any W velocity
		// convert current MOVED position into floating point grid location
		XMVECTOR newPosW = racer.r[3] + velocityW * penetration;
		
		// Preform X & Z colission probes, remove velocity component from vector on collission
		int currCellX = XMVectorGetX(racer.r[3]) + DUNGEON_EDGE;
		int currCellZ = XMVectorGetZ(racer.r[3]) + DUNGEON_EDGE;
		// use floor to force negative values to negative one
		int nextCellX = floor(XMVectorGetX(newPosW) + DUNGEON_EDGE);
		int nextCellZ = floor(XMVectorGetZ(newPosW) + DUNGEON_EDGE);
		// test X collission
		if (nextCellX >= DUNGEON_SIZE || nextCellX < 0 || theDungeon[currCellZ][nextCellX])
			velocityW = XMVectorSetX(velocityW, 0); // remove X velocity
		// test Z collission
		if (nextCellZ >= DUNGEON_SIZE || nextCellZ < 0 || theDungeon[nextCellZ][currCellX])
			velocityW = XMVectorSetZ(velocityW, 0); // remove Z velocity

		// Apply velocity to player
		racer.r[3] += velocityW * timeDelta * 0.5f;

		// preform sine bob
		XMVECTOR travel = XMVector3Length(velocityW * timeDelta);
		static float running = 0; running += XMVectorGetX(travel) * 15;
		racer.r[3] = XMVectorSetY(racer.r[3], 0.5f + cosf(running) * 0.02f);

		// attatch to camera
		XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, racer)) );
		// store back in player
		XMStoreFloat4x4(&playerLocal, racer);

		// transmit updated location to other players
		NetworkEvents::EVENT_DATA playerLocation;
		playerLocation.ID = GAME_EVENT::PLAYER_LOC;
		memcpy_s(playerLocation.matrix, sizeof(XMFLOAT4X4), &playerLocal, sizeof(XMFLOAT4X4));
		NetworkEvents::GetInstance().PushOutgoingEvent(&playerLocation);
	}

	// *********  NETWORK EVENTS *****************
	if (m_connected)
	{
		// If we are the server & we are connected... 
		if (m_host)
		{
			// Only the host decides if the game is won
			// detect if we or the remote player enters the same cell as the treasure
			unsigned int aWinnar = 0; // no winnar
			XMMATRIX pos = XMLoadFloat4x4(&theTreasure);
			int treasureX = XMVectorGetX(pos.r[3]) + DUNGEON_EDGE;
			int treasureZ = XMVectorGetZ(pos.r[3]) + DUNGEON_EDGE;
			pos = XMLoadFloat4x4(&playerLocal);
			int hostX = XMVectorGetX(pos.r[3]) + DUNGEON_EDGE;
			int hostZ = XMVectorGetZ(pos.r[3]) + DUNGEON_EDGE;
			pos = XMLoadFloat4x4(&playerRemote);
			int clientX = XMVectorGetX(pos.r[3]) + DUNGEON_EDGE;
			int clientZ = XMVectorGetZ(pos.r[3]) + DUNGEON_EDGE;
			
			// detrmine if anyone is the winnar
			if (treasureX == hostX && treasureZ == hostZ)
				aWinnar = 1; // we won!
			if (treasureX == clientX && treasureZ == clientZ)
				aWinnar = 2; // the client beat us to it!

			// in either case send a game over message
			if (aWinnar) // continuous broadcast (makes sure they get it!)
			{
				NetworkEvents::EVENT_DATA gameOver;
				gameOver.ID = GAME_EVENT::GAME_OVER;
				gameOver.integerUnsigned = aWinnar;
				// transmit event to BOTH parties
				NetworkEvents::GetInstance().PushOutgoingEvent(&gameOver);
				NetworkEvents::GetInstance().PushIncomingEvent(&gameOver);
			}
		}

		// Handle ALL network events
		NetworkEvents::EVENT_DATA what;
		ZeroMemory(&what, sizeof(NetworkEvents::EVENT_DATA));
		// if we have an event pending we must deal with it
		while (NetworkEvents::GetInstance().PopIncomingEvent(&what))
		{
			switch (what.ID)
			{
			case GAME_EVENT::COMPASS_SEND:

				// use the incoming compass value as our own
				magneticNorth = what.matrix[0];
				break;

			case GAME_EVENT::PLAYER_LOC :
				
				// Update remote player (used to visualize the remote spotlight)
				memcpy_s(&playerRemote, sizeof(XMFLOAT4X4), what.matrix, sizeof(XMFLOAT4X4));
		
				break;

			case GAME_EVENT::GAME_OVER :

				// Pause the game... dispatch message to prompt C# to show proper button
				if (!m_pause)
				{
					m_pause = true;
					Platform::String ^status = ref new Platform::String();
					if (m_host)
						status = (what.integerUnsigned == 1) ? "Win" : "Lose";
					else
						status = (what.integerUnsigned == 2) ? "Win" : "Lose";
					// Invoke callback
					_callbackObject->GameOver(status);
				}
				break;

			case GAME_EVENT::SEED_LEVEL:

				// build game level
				RandomizeLevel(what.integerUnsigned);
				// UnPause the game
				m_pause = false;
				break;	
			}// end switch
		}// end while
	}// end network events

	// after game logic is run, update ligthing variables
	XMMATRIX cWorld = XMLoadFloat4x4(&m_constantBufferData.view);
	cWorld = XMMatrixTranspose(cWorld);
	cWorld = XMMatrixInverse(0, cWorld);
	
	XMStoreFloat3(&lightConstantData.lanterns[0].coneDir, cWorld.r[2]);
	XMStoreFloat3(&lightConstantData.lanterns[0].lightPos, cWorld.r[3]);

	// remote player spotlight
	cWorld = XMLoadFloat4x4(&playerRemote);
	XMStoreFloat3(&lightConstantData.lanterns[1].coneDir, cWorld.r[2]);
	XMStoreFloat3(&lightConstantData.lanterns[1].lightPos, cWorld.r[3]);

}

void GameRenderer::Render()
{
	m_d3dContext->OMSetRenderTargets(1,	m_renderTargetView.GetAddressOf(),	m_depthStencilView.Get());

	//const float midnightBlue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
	const float midnightBlue[] = { 1, 1, 1, 1.000f };
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), midnightBlue);

	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	
	// Only draw the main menu once it is loaded (loading is asynchronous).
	if (!m_loadingComplete)
		return;

	UINT offset = 0;
	UINT stride = sizeof(Vertex);
	// Set all common attributes
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_d3dContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_d3dContext->PSSetConstantBuffers(1, 1, lightConstantBuffer.GetAddressOf());
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	m_d3dContext->PSSetSamplers(0, 1, wrapSampler.GetAddressOf());

	// copy light data to GPU
	m_d3dContext->UpdateSubresource(lightConstantBuffer.Get(), 0, NULL, &lightConstantData, 0, 0);


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
		m_constantBufferData.shading = SHADING::LIGHTING;
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
		for (int z = 0; z < DUNGEON_SIZE; ++z)
			for (int x = 0; x < DUNGEON_SIZE; ++x)
				if (theDungeon[z][x])
				{
					// render only items close to the camera...
					XMVECTOR diff = XMVectorSubtract(XMVectorSet(x - DUNGEON_EDGE, 0, z - DUNGEON_EDGE, 0), cWorld.r[3]);
					XMVECTOR lenR = XMVector3LengthSq(diff);
					XMVECTOR dotR = XMVector3Dot(XMVector3Normalize(diff), cWorld.r[2]);
					// culling
					if (XMVectorGetX(lenR) < 25 || (XMVectorGetX(dotR) > 0.75f && XMVectorGetX(lenR) < 25 * 25))
					{
						// place world matrix in proper location
						XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixTranslation(x - DUNGEON_EDGE, 0, z - DUNGEON_EDGE)));
						m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

						// draw a wall segment
						m_d3dContext->Draw(4, 28);
						m_d3dContext->Draw(4, 32);
						m_d3dContext->Draw(4, 36);
						m_d3dContext->Draw(4, 40);
					}
				}
		// render the treasure (if in view)
		XMVECTOR diff = XMVectorSubtract(XMLoadFloat4x4(&theTreasure).r[3], cWorld.r[3]);
		XMVECTOR lenR = XMVector3LengthSq(diff);
		XMVECTOR dotR = XMVector3Dot(XMVector3Normalize(diff), cWorld.r[2]);
		if (XMVectorGetX(lenR) < 25 || (XMVectorGetX(dotR) > 0.75f && XMVectorGetX(lenR) < 25 * 25))
		{
			if (treasureSRV && treasureIB)
			{
				// place world matrix in proper location
				XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMLoadFloat4x4(&theTreasure)));
				m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

				m_d3dContext->PSSetShaderResources(0, 1, treasureSRV.GetAddressOf());
				m_d3dContext->IASetVertexBuffers(0, 1, treasureVB.GetAddressOf(), &stride, &offset);
				m_d3dContext->IASetIndexBuffer(treasureIB.Get(), DXGI_FORMAT_R32_UINT, 0);
				m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				m_d3dContext->DrawIndexed(ARRAYSIZE(Treasure_indicies), 0, 0);
			}
		}
		// done rendering main game
	}
}