//
//  AppOnMap.cpp
//  Mapply
//
//  Created by eeGeo on 06/08/2012.
//  Copyright (c) 2012 eeGeo. All rights reserved.
//

//#define ART_PREVIEW_STORE "art_edits/local/mike/2013_06_18_5100"

#include <vector>

#include "CameraModel.h"
#include "AppOnMap.h"
#include "LatLongAltitude.h"
#include "EarthConstants.h"
#include "PooledRenderableItemFactory.h"
#include "PooledMesh.h"
#include "ChunkedRoadBuilder.h"
#include "RoadStreaming.h"
#include "TerrainPlaceholderBuilder.h"
#include "ChunkedLcmTerrainBuilder.h"
#include "RasterTerrainBuilder.h"
#include "TerrainStreaming.h"
#include "TerrainHeightProvider.h"
#include "ChunkedBuildingBuilder.h"
#include "BuildingStreaming.h"
#include "PlaceNamesBuilder.h"
#include "PlaceNamesStreaming.h"
#include "BuildingCustomTextureRepository.h"
#include "TerrainPlaceholderInserter.h"
#include "PayloadPlaceholderBuilder.h"
#include "StreamingVolume.h"
#include "StreamingController.h"
#include "BuildPrioritiser.h"
#include "PayloadBuildManager.h"
#include "ResourceNodeCache.h"
#include "PayloadPriorityComparator.h"
#include "StreamingVolumeController.h"
#include "QuadTreeCube.h"
#include "QuadTreeVisibilityUpdater.h"
#include "LoadPrioritiser.h"
#include "PayloadEvicter.h"
#include "SceneGraphUpdater.h"
#include "RedundantNodeEvicter.h"
#include "LodStreamMap.h"
#include "NewGlobeCamera.h"
#include "GlobalLighting.h"
#include "Pool.h"
#include "MortonKeyLongLoader.h"
#include "ItemRenderer.h"
#include "RenderableItem.h"
#include "EegeoEnvironmentRendering.h"
#include "PooledRenderableItemFactory.h"
#include "TrafficSimulation.h"
#include "TrafficSimulationCellFactory.h"
#include "CarVehicleFactory.h"
#include "TrainVehicleFactory.h"
#include "TrafficSimulationCellsModel.h"
#include "VehicleRenderablesModel.h"
#include "VehicleRenderer.h"
#include "VehicleModelRepository.h"
#include "NavigationGraphRepository.h"
#include "NavigationGraphBuilder.h"
#include "NavigationGraphLinkManager.h"
#include "NavigationGraphDebuggingService.h"
#include "RenderContext.h"
#include "GLState.h"
#include "CityThemeRepository.h"
#include "CityThemesTextureUpdater.h"
#include "DefaultMaterialFactory.h"
#include "MaterialNames.h"
#include "EnvironmentFlatteningService.h"
#include "EffectHandler.h"
#include "PrecacheService.h"
#include "DebugValues.h"
#include "Blitter.h"
#include "WebLoadRequestQueue.h"
#include "PayloadWebLoader.h"
#include "IWebLoadRequestFactory.h"
#include "ProcessUpdater.h"
#include "IScheduledUpdateCallback.h"
#include "ShortDiffuseTexturedMaterial.h"
#include "ResourceSpatialQueryService.h"
#include "DebugShader.h"
#include "WeatherController.h"
#include "WeatherEffectsOverlayRenderer.h"
#include "IteratorHelpers.h"
#include "GlobalFogging.h"
#include "ItemRenderer.h"
#include "CoverageTreeBuilder.h"
#include "CoverageTree.h"
#include "CoverageTreeWebRequestService.h"
#include "CityThemeData.h"
#include "RenderHooks.h"
#include "SearchService.h"
#include "LoadingScreen.h"
#include "GlobeCameraInterestPointProvider.h"
#include "Random.h"
#include "PlaneSimulation.h"
#include "NativeUIFactories.h"
#include "RoadNamesController.h"
#include "WorldRenderHooks.h"
#include "PlaceNamesController.h"
#include "CityThemeJsonParser.h"
#include "CityThemeLoader.h"
#include "PlaceNamesRenderer.h"
#include "PlaceNamesRepository.h"
#include "VehicleModelLoader.h"
#include "NativeUIFactories.h"
#include "ILocationService.h"

#include "DebugSphereExample.h"
#include "ScreenUnprojectExample.h"
#include "LoadModelExample.h"
#include "EnvironmentNotifierExample.h"
#include "WebRequestExample.h"
#include "FileIOExample.h"
#include "NavigationGraphExample.h"
#include "ModifiedRenderingExample.h"
#include "ToggleTrafficExample.h"
#include "ResourceSpatialQueryExample.h"
#include "EnvironmentFlatteningExample.h"
#include "SearchExample.h"
#include "KeyboardInputExample.h"
#include "PODAnimationExample.h"

#define TERRAIN_MIN_LEVEL 0
#define TERRAIN_MAX_LEVEL 13

AppOnMap::AppOnMap(AppConfiguration& configuration)
{
    m_initialisationState = Eegeo::InitialisingCoverageTrees;
    
    m_renderContext = new Eegeo::Rendering::RenderContext();
    m_renderContext->SetScreenDimensions(configuration.width, configuration.height, configuration.pixelScale);

    ConfigurePlatformDependentServices(configuration);
    
    m_pTerrainHeightRepository = Eegeo_NEW(Eegeo::Resources::Terrain::Heights::TerrainHeightRepository);
    m_pTerrainHeightProvider = Eegeo_NEW(Eegeo::Resources::Terrain::Heights::TerrainHeightProvider)(m_pTerrainHeightRepository);

    const bool useLowDetailLods = !configuration.isMultiCore;
    Eegeo::EffectHandler::Initialise();
    Eegeo::RenderCamera* pRenderCamera = new Eegeo::RenderCamera();
    Eegeo::Camera::CameraModel*  pCameraModel = new Eegeo::Camera::CameraModel(pRenderCamera);
    m_pCameraModel = pCameraModel;
    
    Eegeo::Camera::NewGlobeCamera* cameraController = new Eegeo::Camera::NewGlobeCamera(pCameraModel, pRenderCamera, *m_pTerrainHeightProvider, useLowDetailLods);
    m_pCameraController = cameraController;
    
    pRenderCamera->SetViewport(0.f, 0.f, m_renderContext->GetScreenWidth(), m_renderContext->GetScreenHeight());

    m_pBlitter = new Eegeo::Blitter(1024 * 128, 1024 * 64, 1024 * 32, *m_renderContext);
    m_pBlitter->Initialise();
    
    m_pLighting = new Eegeo::Lighting::GlobalLighting();
    m_pFogging = new Eegeo::Lighting::GlobalFogging();

    Eegeo::Rendering::DefaultMaterialFactory* pMaterialFactory = new Eegeo::Rendering::DefaultMaterialFactory();
    pMaterialFactory->Initialise(&m_currentWeatherTypeModel,
                                 m_renderContext,
                                 m_pLighting,
                                 m_pFogging,
                                 m_pBlitter,
                                 m_pFileIO,
                                 m_pTextureLoader);
    m_pMaterialFactory = pMaterialFactory;

    m_pVehicleModelRepository = new Eegeo::Traffic::VehicleModelRepository;
    Eegeo::Traffic::VehicleModelLoader vehicleModelLoader(m_renderContext->GetGLState(),
                                                          *m_pTextureLoader,
                                                          *m_pFileIO);

    Eegeo::Traffic::VehicleModelLoaderHelper::LoadAllVehicleResourcesIntoRepository(vehicleModelLoader, *m_pVehicleModelRepository);

    m_pInterestPointProvider = Eegeo_NEW(Eegeo::Location::GlobeCameraInterestPointProvider(*cameraController));

    m_splashScreenFilename = configuration.splashScreenFileName;
    
#if defined EEGEO_NIGHTLY_BUILD_RESOURCES
    m_environmentResourceUrl = EEGEO_NIGHTLY_BUILD_RESOURCES;
    const bool REQUIRE_COVERAGE_TREES = false;
#else
    m_environmentResourceUrl = configuration.environmentResourcesUrl;
    const bool REQUIRE_COVERAGE_TREES = true;
#endif
    
    m_pCoverageTreeWebRequestService = Eegeo_NEW(Eegeo::Streaming::CoverageTrees::CoverageTreeWebRequestService)(*m_pWebLoadRequestFactory,
                                                                                                                 m_pNativeInputFactories->AlertBoxFactory(),
                                                                                                                 REQUIRE_COVERAGE_TREES);
    
    m_pProcessUpdater = Eegeo_NEW(Eegeo::Infrastructure::ProcessUpdater)(0.25);
    
    bool HACK_adjustTransportUvs = true;
    ConfigureEnvironmentResources(HACK_adjustTransportUvs, configuration.themeManifestUrl);
    ConfigureStreaming(useLowDetailLods, configuration.apiToken);
    bool drivesOnRight = true; //arbitrary until themes load
    ConfigureTraffic(drivesOnRight);
    RegisterUpdatableProcesses();
    
    m_pWorldRenderHooks = Eegeo_NEW(Eegeo::WorldRenderHooks(*m_pTrafficSimulation,
                                                            *m_pTrafficVehicleRenderer,
                                                            *m_renderContext,
                                                            *m_pStreamingVolume));
    
    m_pResourceSpatialQueryService = Eegeo_NEW(Eegeo::Resources::ResourceSpatialQueryService)(m_pBuildingPool,
                                                                                              m_pRoadPool,
                                                                                              m_pLcmTerrainPool,
                                                                                              m_pShadowPool,
                                                                                              m_pRoadNamesPool,
                                                                                              m_roadNavGraphRepository);
    //TODO: Also pass Rail nav graph and extend ResourceSpatialQueryService to support Rail nav graph queries.......
    //TODO: And PlaceNames!
    
    if(configuration.pSearchCredentials)
    {
        m_pSearchService = Eegeo_NEW(Eegeo::Search::Service::SearchService)(GetWebRequestFactory(), *m_pUrlEncoder, *configuration.pSearchCredentials);
    }
    else
    {
        m_pSearchService = NULL;
    }
    
    cameraController->SetFlatteningService(m_pEnvironmentFlatteningService);
    
    cameraController->SetInterestHeadingDistance(Eegeo::Space::LatLongAltitude(37.7858,-122.401, 0, Eegeo::Space::LatLongUnits::Degrees),
                                                 0.0f,
                                                 1781.0f);
    
    m_renderContext->GetGLState().InvalidateAll();
    
    m_pExample = NULL;
    m_selectedExample = ExampleTypes::DebugSphere;
    UpdateExample();
}

AppOnMap::~AppOnMap()
{
    Eegeo::EffectHandler::Reset();
    Eegeo::EffectHandler::Shutdown();
    m_pBlitter->Shutdown();
    
    Eegeo_DELETE m_pSearchService;
    
    for(std::vector<Eegeo::Streaming::LodStreamMap*>::iterator it = m_lodStreamMaps.begin(); it != m_lodStreamMaps.end(); ++it)
    {
        Eegeo_DELETE *it;
    }
    m_lodStreamMaps.clear();
    
    Eegeo_DELETE m_pTrafficSimulation;
    Eegeo_DELETE m_pTrafficSimulationCellFactory;
    Eegeo_DELETE m_pCarCellsModel;
    Eegeo_DELETE m_pRailCellsModel;
    Eegeo_DELETE m_pTramCellsModel;
    Eegeo_DELETE m_pCarVehicleFactory;
    Eegeo_DELETE m_pTrainVehicleFactory;
    Eegeo_DELETE m_pTramVehicleFactory;
    Eegeo_DELETE m_pTrafficVehicleRenderablesModel;
    Eegeo_DELETE m_pTrafficVehicleRenderer;
    Eegeo_DELETE m_pPlaneSimulation;
    
    Eegeo_DELETE m_pWeatherEffectsOverlayRenderer;
    
    Eegeo_DELETE m_pResourceSpatialQueryService;
    Eegeo_DELETE m_pProcessUpdater;
    
    Eegeo_DELETE m_pStreamingVolume;
    Eegeo_DELETE m_pStreamingController;
    Eegeo_DELETE m_pBuildPrioritiser;
    Eegeo_DELETE m_pBuildManager;
    Eegeo_DELETE m_pQuadTreeCube;
    Eegeo_DELETE m_pQuadTreeVisibilityUpdater;
    Eegeo_DELETE m_pLoadPrioritiser;
    Eegeo_DELETE m_pNodePool;
    Eegeo_DELETE m_pPayloadPool;
    Eegeo_DELETE m_pPayloadEvicter;
    Eegeo_DELETE m_pSceneGraphUpdater;
    Eegeo_DELETE m_pNodeEvicter;
    Eegeo_DELETE m_pPayloadSlotStreamMap;
    Eegeo_DELETE m_pPayloadPriorityComparator;
    Eegeo_DELETE m_pPrecachingService;
    
    Eegeo_DELETE m_pRoadStream;
    Eegeo_DELETE m_pRoadPool;
    Eegeo_DELETE m_pRoadBuilder;
    Eegeo_DELETE m_pRoadNamesController;
    Eegeo_DELETE m_roadNavGraphRepository;
    Eegeo_DELETE m_roadNavGraphBuilder;
    Eegeo_DELETE m_roadNavGraphLinkManager;
    Eegeo_DELETE m_railNavGraphRepository;
    Eegeo_DELETE m_railNavGraphBuilder;
    Eegeo_DELETE m_railNavGraphLinkManager;
    Eegeo_DELETE m_tramNavGraphRepository;
    Eegeo_DELETE m_tramNavGraphBuilder;
    Eegeo_DELETE m_tramNavGraphLinkManager;
    
    Eegeo_DELETE m_pPlaceNamesController;
    Eegeo_DELETE m_pPlaceNamesRenderer;
    Eegeo_DELETE m_pPlaceNamesRepository;
    
    Eegeo_DELETE m_pFont;
    Eegeo_DELETE m_pFontOutline;

    Eegeo_DELETE m_pBuildingStream;
    Eegeo_DELETE m_pBuildingPool;
    Eegeo_DELETE m_pBuildingBuilder;

    Eegeo_DELETE m_pTerrainStream;
    Eegeo_DELETE m_pLcmTerrainPool;
    Eegeo_DELETE m_pChunkedLcmTerrainBuilder;
    Eegeo_DELETE m_pRasterTerrainBuilder;
    Eegeo_DELETE m_pShadowPool;
    Eegeo_DELETE m_rendering;
    Eegeo_DELETE m_pItemRenderer;
    Eegeo_DELETE m_pEnvironmentFlatteningService;
    Eegeo_DELETE m_pGenericPooledRenderableItemFactory;

    Eegeo_DELETE m_pPlaceNamesStream;
    Eegeo_DELETE m_pRoadNamesPool;
    Eegeo_DELETE m_pPlaceNamesBuilder;

    for(std::vector<Eegeo::DebugRendering::SphereMesh*>::iterator it = m_spheres.begin(); it != m_spheres.end(); ++ it)
    {
        Eegeo_DELETE (*it);
    }
    
    Eegeo::DebugRendering::DebugRenderable::DestroyShaderInstance();
    
    Eegeo_DELETE m_pPrecacheWebLoadRequestQueue;
    Eegeo_DELETE m_pWebLoadRequestQueue;
    Eegeo_DELETE m_pGlBufferPool;
    Eegeo_DELETE m_pWorldRenderHooks;
    Eegeo_DELETE m_pWeatherController;
    
    Eegeo_DELETE m_pCoverageTreeWebRequestService;
    
    Eegeo_DELETE m_pLoadingScreen;
    
    Eegeo_DELETE m_pHttpCache;
    Eegeo_DELETE m_pFileIO;
    Eegeo_DELETE m_pTextureLoader;
    Eegeo_DELETE m_pWebLoadRequestFactory;
    Eegeo_DELETE m_pTaskQueue;
    Eegeo_DELETE m_renderContext;
    Eegeo_DELETE m_pMaterialFactory;
    Eegeo_DELETE m_pLighting;
    Eegeo_DELETE m_pFogging;
    Eegeo_DELETE m_pLocationService;
    Eegeo_DELETE m_pBlitter;
    Eegeo_DELETE m_pNativeInputFactories;
    Eegeo_DELETE m_pInterestPointProvider;
    Eegeo_DELETE m_pUrlEncoder;
    Eegeo_DELETE m_pCameraModel;
    Eegeo_DELETE m_pCameraController;
    Eegeo_DELETE m_pTerrainHeightRepository;
    Eegeo_DELETE m_pTerrainHeightProvider;
    Eegeo_DELETE m_pVehicleModelRepository;
    
    m_spheres.clear();
}

void AppOnMap::ConfigureEnvironmentResources(bool HACK_adjustTransportUvs, const std::string& themeManifestUrl)
{
    m_pGlBufferPool = Eegeo_NEW(Eegeo::Rendering::GlBufferPool)();
    m_rendering = Eegeo_NEW(Eegeo::EegeoEnvironmentRendering)(m_pTextureLoader, m_pMaterialFactory, *m_renderContext);
    m_pGenericPooledRenderableItemFactory = Eegeo_NEW(Eegeo::Resources::PooledRenderableItemFactory);
    
    m_pWebLoadRequestQueue = Eegeo_NEW(Eegeo::Web::WebLoadRequestQueue);
    m_pPrecacheWebLoadRequestQueue = Eegeo_NEW(Eegeo::Web::WebLoadRequestQueue);
    
    //theme loading
    m_pCityThemesRepository = Eegeo_NEW(Eegeo::Resources::CityThemes::CityThemeRepository);
    m_pCityThemesLoader = Eegeo_NEW(Eegeo::Resources::CityThemes::CityThemeLoader)(*m_pWebLoadRequestFactory,
                                                                                   *m_pCityThemesRepository,
                                                                                   *m_pCityThemesJsonParser);
    m_pCityThemesLoader->LoadThemes(themeManifestUrl);
    
    const float cameraDeltaThreshold = 10000.f;
    const float themeMaxAltitudeForUpdate = 20000.f;
    m_pCityThemesTextureUpdater = Eegeo_NEW(Eegeo::Resources::CityThemes::CityThemesTextureUpdater)(*m_pCityThemesRepository,
                                                                                                    cameraDeltaThreshold,
                                                                                                    themeMaxAltitudeForUpdate,
                                                                                                    *m_pCameraModel,
                                                                                                    *m_rendering,
                                                                                                    *m_pWebLoadRequestFactory,
                                                                                                    *m_pTextureLoader,
                                                                                                    *m_pHttpCache);
    //end theme loading
    
    
    m_roadNavGraphLinkManager = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphLinkManager);
    m_roadNavGraphRepository = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphRepository)(m_roadNavGraphLinkManager);
    m_roadNavGraphBuilder = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphBuilder)(m_roadNavGraphRepository);
    
    m_pNavigationGraphDebuggingService = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphDebuggingService)(*m_renderContext,
                                                                                                                         *m_pCameraModel,
                                                                                                                         *m_roadNavGraphRepository);
    
    m_railNavGraphLinkManager = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphLinkManager);
    m_railNavGraphRepository = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphRepository)(m_railNavGraphLinkManager);
    m_railNavGraphBuilder = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphBuilder)(m_railNavGraphRepository);
    
    m_tramNavGraphLinkManager = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphLinkManager);
    m_tramNavGraphRepository = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphRepository)(m_tramNavGraphLinkManager);
    m_tramNavGraphBuilder = Eegeo_NEW(Eegeo::Resources::Roads::Navigation::NavigationGraphBuilder)(m_tramNavGraphRepository);
    
    m_pFont = Eegeo_NEW(Eegeo::Font)(m_renderContext->GetGLState(), *m_pFileIO, *m_pTextureLoader);
    m_pFont->Load("FrankBold50.fnt");
    m_pFontOutline = Eegeo_NEW(Eegeo::Font)(m_renderContext->GetGLState(), *m_pFileIO, *m_pTextureLoader);
    m_pFontOutline->Load("FrankBold50_Outline.fnt");
    
    m_pPlaceNamesRepository = Eegeo_NEW(Eegeo::Resources::PlaceNames::PlaceNamesRepository)();
    m_pPlaceNamesController = Eegeo_NEW(Eegeo::Resources::PlaceNames::PlaceNamesController)(
                                                                                            *m_pCameraModel,
                                                                                            *(Eegeo::Rendering::PlaceNamesMaterial*)m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::PlaceNames),
                                                                                            *m_pPlaceNamesRepository
                                                                                            );
    m_pPlaceNamesBuilder = Eegeo_NEW(Eegeo::Resources::PlaceNames::PlaceNamesBuilder)(m_pGlBufferPool,
                                                                                      *m_pPlaceNamesController,
                                                                                      m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::PlaceNames),
                                                                                      m_pFontOutline);
    
    m_pShadowPool = Eegeo_NEW(Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*>)(1500, m_pGenericPooledRenderableItemFactory);
    
    m_pRoadPool = Eegeo_NEW(Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*>)(1500, m_pGenericPooledRenderableItemFactory);
    m_pRoadNamesPool = Eegeo_NEW(Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*>) (1500, m_pGenericPooledRenderableItemFactory);
    m_pRoadBuilder = Eegeo_NEW(Eegeo::Resources::Roads::ChunkedRoadBuilder)(m_pGlBufferPool,
                                                                            m_pRoadPool,
                                                                            m_pRoadNamesPool,
                                                                            m_pShadowPool,
                                                                            m_roadNavGraphBuilder,
                                                                            m_roadNavGraphRepository,
                                                                            m_railNavGraphBuilder,
                                                                            m_railNavGraphRepository,
                                                                            m_tramNavGraphBuilder,
                                                                            m_tramNavGraphRepository,
                                                                            m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::ThemedRoads),
                                                                            m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::ThemedRail),
                                                                            m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::RoadNames),
                                                                            m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::StencilShadow),
                                                                            m_pFont,
                                                                            HACK_adjustTransportUvs);
    
    m_pRoadNamesController = Eegeo_NEW(Eegeo::Resources::Roads::RoadNamesController)(*m_pInterestPointProvider, *m_pCameraModel, *(Eegeo::Rendering::TextMeshMaterial*)m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::RoadNames));
    
    m_pLcmTerrainPool = Eegeo_NEW(Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*>)(1500, m_pGenericPooledRenderableItemFactory);
    
    m_pChunkedLcmTerrainBuilder = Eegeo_NEW(Eegeo::Resources::Terrain::lcm::ChunkedLcmTerrainBuilder)(m_pGlBufferPool,
                                                                                                      m_rendering->Materials(),
                                                                                                      m_pLcmTerrainPool,
                                                                                                      m_pTerrainHeightRepository,
                                                                                                      m_rendering->MaterialFactory(),
                                                                                                      m_pTextureLoader);
    
    m_pRasterTerrainBuilder = Eegeo_NEW(Eegeo::Resources::Terrain::Raster::RasterTerrainBuilder)(
                                                                                                 m_pGlBufferPool,
                                                                                                 m_renderContext->GetGLState(),
                                                                                                 m_rendering->MaterialFactory(),
                                                                                                 m_pLcmTerrainPool,
                                                                                                 m_pTerrainHeightRepository);
    
    {
        Eegeo::EegeoEnvironmentRendering::TMaterialRepository& materialRepository = *m_rendering->Materials();
        
        Eegeo::Rendering::Material* placeholderMaterial = materialRepository.GetMaterial("pavement_01");

        Eegeo::Rendering::ShortDiffuseTexturedMaterial* shortDiffuseTexturedMaterial = static_cast<Eegeo::Rendering::ShortDiffuseTexturedMaterial*>(placeholderMaterial);
        
        uint placeholderTextureId = shortDiffuseTexturedMaterial->GetTextureId();

        m_pBuildingCustomTextureRepository =
        Eegeo_NEW(Eegeo::Resources::CustomTextures::BuildingCustomTextureRepository)(*m_pWebLoadRequestQueue,
                                                                                     *m_pWebLoadRequestFactory,
                                                                                     m_environmentResourceUrl,
                                                                                     NativeCompressedTextureExtension,
                                                                                     *m_rendering,
                                                                                     placeholderTextureId);
    }
    
    m_pBuildingPool = Eegeo_NEW(Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*>)(200, m_pGenericPooledRenderableItemFactory);
    
    m_pBuildingBuilder = Eegeo_NEW(Eegeo::Resources::Buildings::ChunkedBuildingBuilder)(
                                                                                        m_pGlBufferPool,
                                                                                        m_pBuildingPool,
                                                                                        m_pShadowPool,
                                                                                        m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::ThemedBuildings),
                                                                                        m_rendering->Materials()->GetMaterial(Eegeo::Rendering::MaterialNames::StencilShadow),
                                                                                        *m_pBuildingCustomTextureRepository
                                                                                        );
    
    m_pEnvironmentFlatteningService = Eegeo_NEW(Eegeo::Rendering::EnvironmentFlatteningService(*(m_rendering->Materials())));
    
    m_pPlaceNamesRenderer = Eegeo_NEW(Eegeo::Resources::PlaceNames::PlaceNamesRenderer(*m_pPlaceNamesRepository, *m_renderContext, *m_pEnvironmentFlatteningService));
    
    m_pItemRenderer = Eegeo_NEW(Eegeo::Rendering::ItemRenderer)(*m_renderContext, m_pCameraModel, m_pFogging);
    
    m_pItemRenderer->AddItemSourceToDrawPhase(m_pBuildingPool, Eegeo::Rendering::WorldOpaque);
    m_pItemRenderer->AddItemSourceToDrawPhase(m_pRoadPool, Eegeo::Rendering::WorldOpaque);
    m_pItemRenderer->AddItemSourceToDrawPhase(m_pLcmTerrainPool, Eegeo::Rendering::WorldOpaque);
    m_pItemRenderer->AddItemSourceToDrawPhase(m_pShadowPool, Eegeo::Rendering::WorldShadow);
    m_pItemRenderer->AddItemSourceToDrawPhase(m_pRoadNamesPool, Eegeo::Rendering::WorldTranslucency);
    m_pItemRenderer->AddRenderableItemSourceToDrawPhase(m_pPlaceNamesRenderer, Eegeo::Rendering::WorldTranslucency);
    
    m_pWeatherEffectsOverlayRenderer = Eegeo_NEW( Eegeo::Rendering::WeatherEffectsOverlayRenderer(*m_renderContext, m_rendering->GetWeatherEffectsOverlayMaterial()));
    
    m_pWeatherController = Eegeo_NEW( Eegeo::Weather::WeatherController(*m_rendering,
                                                                        *m_pCityThemesTextureUpdater,
                                                                        *m_pLighting,
                                                                        *m_pFogging,
                                                                        *m_pWeatherEffectsOverlayRenderer,
                                                                        *m_pInterestPointProvider
                                                                        ));
    m_pLoadingScreen = Eegeo_NEW(Eegeo::Rendering::LoadingScreen(m_pTextureLoader, m_splashScreenFilename));
}

void AppOnMap::ConfigureStreaming(bool isLowSpecDevice, const std::string& apiToken)
{
    m_pPayloadSlotStreamMap = Eegeo_NEW(Eegeo::Streaming::PayloadSlotStreamMap());
    
    m_pPayloadWebLoader = Eegeo_NEW(Eegeo::Streaming::PayloadWebLoader)(m_pWebLoadRequestFactory, m_pHttpCache);
    
    m_pPayloadPriorityComparator = Eegeo_NEW(Eegeo::Streaming::PayloadPriorityComparator);
    m_pPayloadPool = Eegeo_NEW(Eegeo::Streaming::PayloadPool);
    m_pBuildPrioritiser = Eegeo_NEW(Eegeo::Streaming::BuildPrioritiser)(m_pPayloadPool, m_pPayloadPriorityComparator);
    m_pResourceNodeCache = Eegeo_NEW(Eegeo::Streaming::ResourceNodeCache)(30);
    m_pBuildManager = Eegeo_NEW(Eegeo::Streaming::PayloadBuildManager)(m_pBuildPrioritiser, m_pResourceNodeCache, m_pTaskQueue);
    m_pNodePool = Eegeo_NEW(Eegeo::Streaming::QuadTreeNodePool);
    m_pQuadTreeCube = Eegeo_NEW(Eegeo::Streaming::QuadTreeCube)(m_pNodePool);
    
    
    const bool enableBuildingLod0 = !isLowSpecDevice;
    int deepestLevelForAltitudeLodRefinement = isLowSpecDevice ? 14 : 12;
    
    const int lodAltitudeCount = 15;
    
    const double lowSpecLodRefinementAltitudes[lodAltitudeCount] =
    {
        5000000,
        2100000,
        1100000,
        520000,
        260000,
        130000,
        75000,
        60000,
        40000,
        20000,
        10000, // lcm
        4000,  // detail roads
        2600,  // lod2 buildings
        800,  // lod1 buildings
        500    // lod0 buildings
    };
    
    const double defaultLodRefinementAltitudes[lodAltitudeCount] =
    {
        5000000,
        2100000,
        1100000,
        520000,
        260000,
        130000,
        75000,
        60000,
        40000,
        25000,
        15000,
        10000,
        7500,
        0,
        0
    };
    
    const double* altitudes = isLowSpecDevice ? lowSpecLodRefinementAltitudes : defaultLodRefinementAltitudes;
    const std::vector<double> lodRefinementAltitudes(altitudes, &altitudes[lodAltitudeCount]);
    
    m_pQuadTreeVisibilityUpdater = Eegeo_NEW(Eegeo::Streaming::QuadTreeVisibilityUpdater)(m_pNodePool, m_pPayloadPool, m_pPayloadSlotStreamMap, lodRefinementAltitudes, deepestLevelForAltitudeLodRefinement);
    m_pLoadPrioritiser = Eegeo_NEW(Eegeo::Streaming::LoadPrioritiser)(m_pPayloadPool, m_pPayloadPriorityComparator);
    m_pPayloadEvicter = Eegeo_NEW(Eegeo::Streaming::PayloadEvicter)(m_pQuadTreeCube, m_pPayloadPool);
    m_pSceneGraphUpdater = Eegeo_NEW(Eegeo::Streaming::SceneGraphUpdater)(m_pQuadTreeCube, m_pPayloadPool);
    m_pNodeEvicter = Eegeo_NEW(Eegeo::Streaming::RedundantNodeEvicter)(m_pNodePool);
    m_pPayloadPlaceholderBuilder = Eegeo_NEW(Eegeo::Streaming::PayloadPlaceholderBuilder)(m_pPayloadPool);
    
    Eegeo::Streaming::StreamingVolume* pStreamingVolume = Eegeo_NEW(Eegeo::Streaming::StreamingVolume);
    m_pStreamingVolume = pStreamingVolume;
    m_pStreamingVolumeController = Eegeo_NEW(Eegeo::Streaming::StreamingVolumeController)(pStreamingVolume,
                                                                                          *m_renderContext,
                                                                                          m_pCameraModel);
    
    ConfigureLODStreams(enableBuildingLod0);
    
    m_pTerrainPlaceholderInserter = Eegeo_NEW(Eegeo::Streaming::TerrainPlaceholderInserter)(m_pPayloadPool,
                                                                                            m_pNodePool,
                                                                                            m_pTerrainStream,
                                                                                            m_pPayloadSlotStreamMap,
                                                                                            TERRAIN_MIN_LEVEL,
                                                                                            TERRAIN_MAX_LEVEL);
    
    m_pPrecachingService = Eegeo_NEW(Eegeo::Web::PrecacheService)(*m_pPrecacheWebLoadRequestQueue,
                                                                  m_pWebLoadRequestFactory,
                                                                  m_pPayloadSlotStreamMap,
                                                                  m_pHttpCache,
                                                                  m_pBuildingCustomTextureRepository);
    
    m_pStreamingController = Eegeo_NEW(Eegeo::Streaming::StreamingController)(apiToken,
                                                                              m_pPayloadWebLoader,
                                                                              m_pBuildManager,
                                                                              pStreamingVolume,
                                                                              m_pStreamingVolumeController,
                                                                              m_lodStreamMaps,
                                                                              m_pQuadTreeCube,
                                                                              m_pQuadTreeVisibilityUpdater,
                                                                              m_pLoadPrioritiser,
                                                                              m_pNodePool,
                                                                              m_pPayloadPool,
                                                                              m_pPayloadEvicter,
                                                                              m_pSceneGraphUpdater,
                                                                              m_pNodeEvicter,
                                                                              m_pPayloadSlotStreamMap,
                                                                              m_pPayloadPlaceholderBuilder,
                                                                              m_pTerrainPlaceholderInserter,
                                                                              m_pWebLoadRequestQueue,
                                                                              m_pResourceNodeCache,
                                                                              m_pPrecachingService);
}

void AppOnMap::ConfigureLODStreams(bool enableBuildingLod0)
{
    const char* environmentResourcesBaseUrl = m_environmentResourceUrl.c_str();
    
    m_pTerrainPlaceholderBuilder = Eegeo_NEW(Eegeo::Resources::Terrain::Placeholder::TerrainPlaceholderBuilder)(m_pGlBufferPool,
                                                                                                                m_pLcmTerrainPool,
                                                                                                                m_rendering->Materials()->GetMaterial("Terrain_Placeholder"));
    
#ifdef ART_PREVIEW_STORE
    m_pRoadStream = Eegeo::Resources::Roads::RoadStreaming::CreateWithStore(
                                                                            m_pRoadBuilder,
                                                                            environmentResourcesBaseUrl,
                                                                            std::string(ART_PREVIEW_STORE));
    
    m_pBuildingStream = Eegeo::Resources::Buildings::BuildingStreaming::CreateWithStore(
                                                                                        m_pBuildingBuilder,
                                                                                        environmentResourcesBaseUrl,
                                                                                        std::string(ART_PREVIEW_STORE));
    
    m_pTerrainStream = Eegeo::Resources::Terrain::TerrainStreaming::CreateWithLcmStore(
                                                                                       m_pChunkedLcmTerrainBuilder,
                                                                                       m_pTerrainPlaceholderBuilder,
                                                                                       environmentResourcesBaseUrl,
                                                                                       std::string(ART_PREVIEW_STORE));
#else
    m_pRoadStream = Eegeo::Resources::Roads::RoadStreaming::CreateWithStoreOverrides(
                                                                                     m_pRoadBuilder,
                                                                                     environmentResourcesBaseUrl,
                                                                                     m_pFileIO,
                                                                                     *m_pCoverageTreeWebRequestService);
    
    m_pBuildingStream = Eegeo::Resources::Buildings::BuildingStreaming::CreateWithStoreOverrides(
                                                                                                 m_pBuildingBuilder,
                                                                                                 environmentResourcesBaseUrl,
                                                                                                 *m_pCoverageTreeWebRequestService);
    
    m_pTerrainStream = Eegeo::Resources::Terrain::TerrainStreaming::CreateWithStoreOverrides(
                                                                                             m_pChunkedLcmTerrainBuilder,
                                                                                             m_pRasterTerrainBuilder,
                                                                                             m_pTerrainPlaceholderBuilder,
                                                                                             environmentResourcesBaseUrl,                                                                                                                                         m_pFileIO,
                                                                                             *m_pCoverageTreeWebRequestService);
#endif
    
    m_pPlaceNamesStream = Eegeo_NEW(Eegeo::Resources::PlaceNames::PlaceNamesStreaming)(m_pPlaceNamesBuilder, environmentResourcesBaseUrl);
    
    Eegeo::Streaming::LodStreamMap* pRoadLodStreamMap = Eegeo_NEW(Eegeo::Streaming::LodStreamMap);
    pRoadLodStreamMap->addStream(m_pRoadStream, 11, 13);
    m_lodStreamMaps.push_back(pRoadLodStreamMap);
    
    Eegeo::Streaming::LodStreamMap* pPlaceNameStreamMap = Eegeo_NEW(Eegeo::Streaming::LodStreamMap);
    pPlaceNameStreamMap->addStream(m_pPlaceNamesStream, 0, 11);
    m_lodStreamMaps.push_back(pPlaceNameStreamMap);
    
    Eegeo::Streaming::LodStreamMap* pTerrainStreamMap = Eegeo_NEW(Eegeo::Streaming::LodStreamMap);
    pTerrainStreamMap->addStream(m_pTerrainStream, TERRAIN_MIN_LEVEL, TERRAIN_MAX_LEVEL);
    m_lodStreamMaps.push_back(pTerrainStreamMap);
    
    Eegeo::Streaming::LodStreamMap* pBuildingStreamMap = Eegeo_NEW(Eegeo::Streaming::LodStreamMap);
    
    const int maxBuildingLevel = enableBuildingLod0 ? 15 : 14;
    pBuildingStreamMap->addStream(m_pBuildingStream, 13, maxBuildingLevel);
    m_lodStreamMaps.push_back(pBuildingStreamMap);
}

void AppOnMap::ConfigureTraffic(bool drivesOnRight)
{
    Eegeo_ASSERT(m_roadNavGraphRepository != NULL);
    Eegeo_ASSERT(m_railNavGraphRepository != NULL);
    Eegeo_ASSERT(m_tramNavGraphRepository != NULL);
    
    Eegeo::Rendering::GLState& state = m_renderContext->GetGLState();
    
    m_pTrafficSimulationCellFactory = Eegeo_NEW(Eegeo::Traffic::TrafficSimulationCellFactory);
    const bool managePopulation = true;
    m_pCarCellsModel = Eegeo_NEW(Eegeo::Traffic::TrafficSimulationCellsModel)(Eegeo::Traffic::TrafficSimulationCell_Car, managePopulation);
    m_pRailCellsModel = Eegeo_NEW(Eegeo::Traffic::TrafficSimulationCellsModel)(Eegeo::Traffic::TrafficSimulationCell_Rail, !managePopulation);
    m_pTramCellsModel = Eegeo_NEW(Eegeo::Traffic::TrafficSimulationCellsModel)(Eegeo::Traffic::TrafficSimulationCell_Tram, !managePopulation);
    m_pTrafficVehicleRenderablesModel = Eegeo_NEW(Eegeo::Traffic::VehicleRenderablesModel);
    
    m_pCarVehicleFactory = Eegeo_NEW(Eegeo::Traffic::CarVehicleFactory)(*m_pInterestPointProvider,
                                                                        *m_pCameraModel,
                                                                        m_pCarCellsModel,
                                                                        m_pTrafficVehicleRenderablesModel,
                                                                        *m_pVehicleModelRepository);
    
    m_pTrainVehicleFactory = Eegeo_NEW(Eegeo::Traffic::TrainVehicleFactory)(*m_pInterestPointProvider,
                                                                            *m_pCameraModel,
                                                                            m_pRailCellsModel,
                                                                            m_pTrafficVehicleRenderablesModel,
                                                                            *m_pVehicleModelRepository,
                                                                            false);
    
    m_pTramVehicleFactory = Eegeo_NEW(Eegeo::Traffic::TrainVehicleFactory)(*m_pInterestPointProvider,
                                                                           *m_pCameraModel,
                                                                           m_pTramCellsModel,
                                                                           m_pTrafficVehicleRenderablesModel,
                                                                           *m_pVehicleModelRepository,
                                                                           true);
    
    m_pTrafficVehicleRenderer = Eegeo_NEW(Eegeo::Traffic::VehicleRenderer)(m_pTrafficVehicleRenderablesModel,
                                                                           m_pCameraModel,
                                                                           state,
                                                                           m_pVehicleModelRepository,
                                                                           *m_pEnvironmentFlatteningService,
                                                                           *m_pLighting,
                                                                           *m_pFogging);
    const u32 randomSeed = 1;
    Eegeo::Random * pRandom  = Eegeo_NEW(Eegeo::Random)(randomSeed);
    m_pPlaneSimulation = Eegeo_NEW(Eegeo::Traffic::PlaneSimulation)(m_pTrafficVehicleRenderablesModel,
                                                                    *m_pVehicleModelRepository,
                                                                    *pRandom,
                                                                    m_pCameraModel,
                                                                    m_pStreamingVolume,
                                                                    m_pTerrainHeightProvider,
                                                                    m_pCameraController);
    
    m_pTrafficSimulation = Eegeo_NEW(Eegeo::Traffic::TrafficSimulation)(*m_roadNavGraphRepository,
                                                                        *m_railNavGraphRepository,
                                                                        *m_tramNavGraphRepository,
                                                                        m_pCarCellsModel,
                                                                        m_pRailCellsModel,
                                                                        m_pTramCellsModel,
                                                                        *m_pTrafficSimulationCellFactory,
                                                                        *m_pCarVehicleFactory,
                                                                        *m_pTrainVehicleFactory, // TODO: Train factory.
                                                                        *m_pTramVehicleFactory,
                                                                        pRandom,
                                                                        m_pCameraModel,
                                                                        *m_pCityThemesTextureUpdater,
                                                                        m_pTrafficVehicleRenderablesModel,
                                                                        drivesOnRight,
                                                                        m_pPlaneSimulation);
    
}

void AppOnMap::RegisterUpdatableProcesses()
{
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Rendering::EnvironmentFlatteningService>)(m_pEnvironmentFlatteningService, &Eegeo::Rendering::EnvironmentFlatteningService::Update), 0);
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Camera::ICamera>)(m_pCameraController, &Eegeo::Camera::ICamera::Update), 1);
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Weather::WeatherController>)(m_pWeatherController, &Eegeo::Weather::WeatherController::Update), 2);
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Resources::CityThemes::CityThemesTextureUpdater>)(m_pCityThemesTextureUpdater, &Eegeo::Resources::CityThemes::CityThemesTextureUpdater::Update), 3);
    
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::EegeoEnvironmentRendering>)(m_rendering, &Eegeo::EegeoEnvironmentRendering::Update), 4);
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Streaming::StreamingController>)(m_pStreamingController, &Eegeo::Streaming::StreamingController::Update), 5);
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Traffic::TrafficSimulation>)(m_pTrafficSimulation, &Eegeo::Traffic::TrafficSimulation::Update), 6);
    m_pProcessUpdater->AddFixedTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Rendering::WeatherEffectsOverlayRenderer>)(m_pWeatherEffectsOverlayRenderer, &Eegeo::Rendering::WeatherEffectsOverlayRenderer::Update), 1/30.0, 7);
    
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Resources::Roads::RoadNamesController>)(m_pRoadNamesController, &Eegeo::Resources::Roads::RoadNamesController::Update), 7);
    m_pProcessUpdater->AddVariableTimestepScheduledUpdate(Eegeo_NEW(Eegeo::Infrastructure::TScheduledUpdateCallback<Eegeo::Resources::PlaceNames::PlaceNamesController>)(m_pPlaceNamesController, &Eegeo::Resources::PlaceNames::PlaceNamesController::Update), 7);
}

void AppOnMap::Update (float dt)
{
    m_pTaskQueue->Update();
    
    if(m_pLoadingScreen->IsVisible())
    {
        m_pLoadingScreen->Update(dt);
    }
    
    switch(m_initialisationState)
    {
        case Eegeo::InitialisingCoverageTrees:
        {
            if(m_pCoverageTreeWebRequestService->IsComplete())
            {
                m_initialisationState = Eegeo::DownloadTheme;
                m_pCameraController->Update(dt); // Needed to ensure camera is in the correct place when theme load begins.
            }
            return;
        }
        case Eegeo::DownloadTheme:
        {
            m_pCityThemesTextureUpdater->Update(dt);
            
            Eegeo::Resources::CityThemes::CityThemeType themeType = m_pCityThemesTextureUpdater->GetCurrentTheme();
            
            if(themeType != Eegeo::Resources::CityThemes::CityTheme_Invalid)
            {
                m_initialisationState = Eegeo::StreamingInitialScene;
                
                const Eegeo::Resources::CityThemes::CityThemeData& cityThemeData = m_pCityThemesRepository->GetThemeData(themeType);
                
                m_pWeatherController->SetFromCityThemeData(cityThemeData);
                m_pWeatherController->SetWeather(Eegeo::Weather::Sunny, 1.0f);
                m_pTrafficSimulation->SetDrivesOnRight(cityThemeData.DriveOnRight);
            }
            return;
        }
        case Eegeo::StreamingInitialScene:
        {
            m_pStreamingController->Update(dt);
            m_pEnvironmentFlatteningService->Update(dt);
            if(m_pStreamingController->numPayloadsWaitingToLoad() == 0 && m_pStreamingController->numPayloadsWaitingToBuild() == 0)
            {
                m_initialisationState = Eegeo::Completed;
                m_pLoadingScreen->BeginFadeOut();
            }
            return;
        }
        default:
            break;
    }
    
    m_renderContext->BeginUpdatePhase();
    m_pProcessUpdater->Update(dt);
    
    m_pExample->Update();
}

void AppOnMap::PreDraw(float dt)
{
    Eegeo::Debug::DebugValues::totalPolysRendered = 0;
    Eegeo::Debug::DebugValues::shadowPolysRendered = 0;
    Eegeo::Debug::DebugValues::totalShadowDrawCalls = 0;
    Eegeo::Debug::DebugValues::totalDrawCalls = 0;
    Eegeo::Camera::ICamera& cameraSelector = GetCameraController();
    Eegeo::RenderCamera* currentCamera = cameraSelector.GetCamera();
    Eegeo::EffectHandler::Reset();
    m_renderContext->BeginDrawPhase(*currentCamera, m_pCameraModel->GetWorldPosition());
    
    Eegeo::EffectHandler::Temp_UpdateFromRenderContext(*m_renderContext);
    m_pBlitter->Reset();
    
    m_pEnvironmentFlatteningService->SetDrawState();
    m_pLighting->SetRenderState(GetCameraModel().GetWorldPosition());
}

void AppOnMap::DrawWorldPhases(float dt, Eegeo::Rendering::RenderHooks* pRenderHooks)
{
    m_pStreamingController->Render(dt);
    Eegeo::RenderCamera& camera = *m_pCameraController->GetCamera();
    
    for(std::vector<Eegeo::DebugRendering::SphereMesh*>::iterator it = m_spheres.begin(); it != m_spheres.end(); ++ it)
    {
        (*it)->Draw(*m_pCameraController->GetCamera());
    }
    
    m_pItemRenderer->Draw(dt, *m_pCameraController->GetCamera(), *m_pStreamingVolume, pRenderHooks);
    
    m_pNavigationGraphDebuggingService->Draw(camera, *m_pCameraModel);
}

void AppOnMap::DrawAllPhases(float dt, Eegeo::Rendering::RenderHooks* pRenderHooks)
{
    Eegeo::Rendering::GLState& glState = m_renderContext->GetGLState();
    
    pRenderHooks->BeforeAllDrawPhases(dt, glState);
    
    pRenderHooks->BeforeAllWorldPhases(dt, glState);
    DrawWorldPhases(dt, pRenderHooks);
    pRenderHooks->AfterAllWorldPhases(dt, glState);
    
    pRenderHooks->BeforeWeatherOverlayPhase(dt, glState);
    m_pWeatherEffectsOverlayRenderer->Draw(m_pCameraModel->GetWorldPosition());
    pRenderHooks->AfterWeatherOverlayPhase(dt, glState);
    
    if(m_pLoadingScreen->IsVisible())
    {
        m_pLoadingScreen->Draw(*m_renderContext,m_pBlitter);
    }
    
    pRenderHooks->AfterAllDrawPhases(dt, glState);
}

void AppOnMap::PostDraw(float dt)
{
    m_rendering->EndFrame();
    
    Eegeo::EffectHandler::Reset();
    Eegeo::EffectHandler::Temp_UpdateFromRenderContext(*m_renderContext);
    m_pBlitter->Reset();
    
    m_pBlitter->SetDepthWrite(true);
    m_pBlitter->Draw(m_renderContext->GetViewProjectionMatrix(), m_renderContext->GetCameraWorldMatrix());
}

void AppOnMap::Draw (float dt, Eegeo::Rendering::RenderHooks* pRenderHooks)
{
    m_pWorldRenderHooks->SetChildRenderHooks(pRenderHooks);
    
    PreDraw(dt);
    
    DrawAllPhases(dt, m_pWorldRenderHooks);
    
    PostDraw(dt);
    
    if(m_initialisationState == Eegeo::Completed) {
        m_pExample->Draw();
    }
}

void AppOnMap::Draw (float dt)
{
    Draw(dt, NULL);
}

namespace {
    Examples::IExample* CreateExample(ExampleTypes::Examples example,
                                      Eegeo::Rendering::RenderContext& renderContext,
                                      Eegeo::Space::LatLongAltitude interestLocation,
                                      Eegeo::Camera::CameraModel& cameraModel,
                                      Eegeo::Camera::NewGlobeCamera& globeCamera,
                                      Eegeo::RenderCamera& renderCamera,
                                      Eegeo::Resources::Terrain::Heights::TerrainHeightProvider& terrainHeightProvider,
                                      Eegeo::Helpers::ITextureFileLoader& textureLoader,
                                      Eegeo::Helpers::IFileIO& fileIO,
                                      Eegeo::Resources::Terrain::TerrainStreaming& terrainStreaming,
                                      Eegeo::Web::IWebLoadRequestFactory& webRequestFactory,
                                      Eegeo::Resources::Roads::Navigation::NavigationGraphRepository& navigationGraphs,
                                      Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*>& buildingPool,
                                      Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*>& shadowPool,
                                      Eegeo::Streaming::IStreamingVolume& visibleVolume,
                                      Eegeo::Lighting::GlobalLighting& lighting,
                                      Eegeo::Lighting::GlobalFogging& fogging,
                                      Eegeo::Traffic::TrafficSimulation& trafficSimulation,
                                      Eegeo::Resources::ResourceSpatialQueryService& resourceSpatialQueryService,
                                      Eegeo::Rendering::EnvironmentFlatteningService& environmentFlatteningService,
                                      Eegeo::Search::Service::SearchService* searchService,
                                      Eegeo::UI::NativeUIFactories& nativeInputFactories)
    {
        switch(example)
        {
            case ExampleTypes::LoadModel:
                return new Examples::LoadModelExample(renderContext,
                                                      interestLocation,
                                                      cameraModel,
                                                      renderCamera,
                                                      fileIO,
                                                      textureLoader,
                                                      fogging);
                
            case ExampleTypes::ScreenUnprojectTerrainHeightQuery:
                return new Examples::ScreenUnprojectExample(renderContext,
                                                            cameraModel,
                                                            renderCamera,
                                                            terrainHeightProvider);
            case ExampleTypes::DebugSphere:
                return new Examples::DebugSphereExample(renderContext,
                                                        interestLocation,
                                                        cameraModel,
                                                        renderCamera);
            case ExampleTypes::EnvironmentNotifier:
                return new Examples::EnvironmentNotifierExample(renderContext,
                                                                cameraModel,
                                                                renderCamera,
                                                                terrainStreaming);
            case ExampleTypes::WebRequest:
                return new Examples::WebRequestExample(webRequestFactory);
                
            case ExampleTypes::FileIO:
                return new Examples::FileIOExample(fileIO);
                
            case ExampleTypes::NavigationGraph:
                return new Examples::NavigationGraphExample(renderContext,
                                                            renderCamera,
                                                            cameraModel,
                                                            navigationGraphs);
                
            case ExampleTypes::ModifiedRendering:
                return new Examples::ModifiedRenderingExample(renderContext,
                                                              renderCamera,
                                                              cameraModel,
                                                              globeCamera,
                                                              visibleVolume,
                                                              lighting,
                                                              buildingPool,
                                                              shadowPool);
                
            case ExampleTypes::ToggleTraffic:
                return new Examples::ToggleTrafficExample(trafficSimulation);
                
            case ExampleTypes::ResourceSpatialQuery:
                return new Examples::ResourceSpatialQueryExample(resourceSpatialQueryService,
                                                                 globeCamera);
                
            case ExampleTypes::EnvironmentFlattening:
                return new Examples::EnvironmentFlatteningExample(environmentFlatteningService);
                
            case ExampleTypes::Search:
                Eegeo_ASSERT(searchService != NULL, "Cannot run Search example, you must set up here.com Credentials in ViewController.mm");
                return new Examples::SearchExample(*searchService, globeCamera);
                
            case ExampleTypes::KeyboardInput:
                return new Examples::KeyboardInputExample(nativeInputFactories.IKeyboardInputFactory());
                
            case ExampleTypes::PODAnimation:
                return new Examples::PODAnimationExample(renderContext,
                                                         cameraModel,
                                                         fileIO,
                                                         textureLoader,
                                                         fogging);
                
        }
    }
}

void AppOnMap::UpdateExample()
{
    if(m_pExample != NULL) {
        m_pExample->Suspend();
    }
    
    Eegeo_DELETE m_pExample;
    
    float interestPointLatitudeDegrees = 37.7858f;
    float interestPointLongitudeDegrees = -122.401f;
    float interestPointAltitudeMeters = 2.7;
    
    Eegeo::Space::LatLongAltitude location = Eegeo::Space::LatLongAltitude(interestPointLatitudeDegrees,
                                                                           interestPointLongitudeDegrees,
                                                                           interestPointAltitudeMeters,
                                                                           Eegeo::Space::LatLongUnits::Degrees);
    
    GetCameraModel().SetWorldPosition(location.ToECEF());
    
    float cameraControllerOrientationDegrees = 0.0f;
    float cameraControllerDistanceFromInterestPointMeters = 1781.0f;
    
    GetGlobeCamera().SetInterestHeadingDistance(location,
                                             cameraControllerOrientationDegrees,
                                             cameraControllerDistanceFromInterestPointMeters);
    
    GetWeatherController().SetWeather(Eegeo::Weather::Sunny, 1.0f);
    
    Eegeo::Search::Service::SearchService* searchService = NULL;
    
    if (IsSearchServiceAvailable()) {
        searchService = &GetSearchService();
    }
    
    m_pExample = CreateExample(m_selectedExample,
                             GetRenderContext(),
                             location,
                             GetCameraModel(),
                             GetGlobeCamera(),
                             *GetGlobeCamera().GetCamera(),
                             GetTerrainHeightProvider(),
                             GetTextureLoader(),
                             GetFileIO(),
                             GetTerrainStreaming(),
                             GetWebRequestFactory(),
                             GetNavigationGraphRepository(),
                             GetBuildingMeshPool(),
                             GetShadowMeshPool(),
                             GetStreamingVolume(),
                             GetGlobalLighting(),
                             GetGlobalFogging(),
                             GetTrafficSimulation(),
                             GetResourceSpatialQueryService(),
                             GetEnvironmentFlatteningService(),
                             searchService,
                             GetNativeUIFactories());
    
    m_pExample->Start();
    
    if(m_selectedExample == ExampleTypes::LAST_EXAMPLE) {
        m_selectedExample = ExampleTypes::FIRST_EXAMPLE;
    }
    else {
        m_selectedExample ++;
    }
    
    if (m_selectedExample == ExampleTypes::Search && !IsSearchServiceAvailable()) {
        m_selectedExample ++;
    }
    
}

#if defined(EEGEO_IOS)

#include "iOSUrlEncoder.h"
#include "iOSTaskQueue.h"
#include "iOSFileIO.h"
#include "iOSHttpCache.h"
#include "iOSTextureFileLoader.h"
#include "iOSWebLoadRequestFactory.h"
#include "iOSLocationService.h"
#include "iOSInputBoxFactory.h"
#include "iOSKeyboardInputFactory.h"
#include "iOSAlertBoxFactory.h"

void AppOnMap::ConfigurePlatformDependentServices(AppConfiguration& configuration)
{
    m_pLocationService = Eegeo_NEW(Eegeo::iOS::iOSLocationService);
    Eegeo::iOS::iOSFileIO* p_iOSFileIO = Eegeo_NEW(Eegeo::iOS::iOSFileIO);
    m_pFileIO = p_iOSFileIO;
    Eegeo::Helpers::ITextureFileLoader* pTextureFileLoader = Eegeo_NEW(Eegeo::iOS::iOSTextureFileLoader)(p_iOSFileIO, m_renderContext->GetGLState());
    m_pTextureLoader = pTextureFileLoader;
    iOSHttpCache* pHttpCache = Eegeo_NEW(iOSHttpCache)(*p_iOSFileIO);
    m_pHttpCache = pHttpCache;
    Eegeo::Web::IWebLoadRequestFactory* pWebLoadRequestFactory = Eegeo_NEW(Eegeo::Web::iOSWebLoadRequestFactory)(*pHttpCache);
    m_pWebLoadRequestFactory = pWebLoadRequestFactory;
    Eegeo::Helpers::ITaskQueue* pTaskQueue = Eegeo_NEW(iOSTaskQueue)(10);
    m_pTaskQueue = pTaskQueue;
    m_pUrlEncoder = Eegeo_NEW(iOSUrlEncoder);
    
    Eegeo::UI::NativeInput::iOS::iOSInputBoxFactory* inputBoxFactory = Eegeo_NEW(Eegeo::UI::NativeInput::iOS::iOSInputBoxFactory);
    Eegeo::UI::NativeInput::iOS::iOSKeyboardInputFactory* keyboardInputFactory = Eegeo_NEW(Eegeo::UI::NativeInput::iOS::iOSKeyboardInputFactory);
    Eegeo::UI::NativeAlerts::iOS::iOSAlertBoxFactory* alertBoxFactory = Eegeo_NEW(Eegeo::UI::NativeAlerts::iOS::iOSAlertBoxFactory);
    m_pNativeInputFactories = Eegeo_NEW(Eegeo::UI::NativeUIFactories)(*alertBoxFactory, *inputBoxFactory, *keyboardInputFactory);
}

#elif defined(EEGEO_DROID)

#include "AndroidUrlEncoder.h"
#include "AndroidTaskQueue.h"
#include "AndroidFileIO.h"
#include "AndroidHttpCache.h"
#include "AndroidTextureFileLoader.h"
#include "AndroidWebLoadRequestFactory.h"
#include "AndroidLocationService.h"
#include "AndroidWebRequestService.hpp"
#include "AndroidInputBoxFactory.h"
#include "AndroidKeyboardInputFactory.h"
#include "AndroidAlertBoxFactory.h"

using namespace Eegeo::Android;
using namespace Eegeo::UI::NativeInput::Android;
void AppOnMap::ConfigurePlatformDependentServices(AppConfiguration& configuration)
{
	AndroidLocationService* pAndroidLocationService = new AndroidLocationService(configuration.pState);
	m_pLocationService = pAndroidLocationService;

	AndroidFileIO* pFileIO = new AndroidFileIO(configuration.pState);
	m_pFileIO = pFileIO;

	AndroidHttpCache* pHttpCache = new AndroidHttpCache(pFileIO, configuration.environmentResourcesUrl);  \
	m_pHttpCache = pHttpCache;
	AndroidTextureFileLoader* pTextureLoader = new AndroidTextureFileLoader(pFileIO, m_renderContext->GetGLState());
	m_pTextureLoader = pTextureLoader;

	AndroidTaskQueue* pTaskQueue = new AndroidTaskQueue(
			10,
			configuration.resourceBuildShareContext,
			configuration.shareSurface,
			configuration.display);
	m_pTaskQueue = pTaskQueue;

	m_pUrlEncoder = Eegeo_NEW(AndroidUrlEncoder)(configuration.pState);

	Eegeo::Android::AndroidWebRequestService* pAndroidWebRequestService = new AndroidWebRequestService(*pFileIO, pHttpCache, pTaskQueue, 50);
	configuration.pAndroidWebRequestService = pAndroidWebRequestService;

	AndroidWebLoadRequestFactory* pAndroidWebLoadRequestFactory = new AndroidWebLoadRequestFactory(
			pAndroidWebRequestService,
			pHttpCache);
	m_pWebLoadRequestFactory = pAndroidWebLoadRequestFactory;

    Eegeo::UI::NativeInput::Android::AndroidInputBoxFactory* inputBoxFactory = Eegeo_NEW(Eegeo::UI::NativeInput::Android::AndroidInputBoxFactory)(configuration.pState);
    Eegeo::Android::Input::IAndroidInputHandler& handler = (Eegeo::Android::Input::IAndroidInputHandler&)(*configuration.pInputHandler);
    Eegeo::UI::NativeInput::Android::AndroidKeyboardInputFactory* keyboardInputFactory = Eegeo_NEW(Eegeo::UI::NativeInput::Android::AndroidKeyboardInputFactory)(configuration.pState, handler);
    Eegeo::UI::NativeAlerts::Android::AndroidAlertBoxFactory* alertBoxFactory = Eegeo_NEW(Eegeo::UI::NativeAlerts::Android::AndroidAlertBoxFactory)(configuration.pState);
    m_pNativeInputFactories = Eegeo_NEW(Eegeo::UI::NativeUIFactories)(*alertBoxFactory, *inputBoxFactory, *keyboardInputFactory);
}

#endif
