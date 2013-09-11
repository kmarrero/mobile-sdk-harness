#ifndef Mapply_AppOnMap_h
#define Mapply_AppOnMap_h

#include <vector>
#include <string>
#include "SphereMesh.h"
#include "Navigation.h"
#include "Traffic.h"
#include "CityThemes.h"
#include "IFileIO.h"
#include "ITextureFileLoader.h"
#include "IHttpCache.h"
#include "ITaskQueue.h"
#include "ChunkedBuildingBuilder.h"
#include "ChunkedRoadBuilder.h"
#include "Web.h"
#include "CustomTextures.h"
#include "Streaming.h"
#include "Weather.h"
#include "Lighting.h"
#include "EegeoSearch.h"
#include "UrlHelpers.h"
#include "WeatherUpdateModel.h"
#include "IExample.h"

namespace Eegeo
{
	class RenderCamera;
	class Blitter;
    class EegeoEnvironmentRendering;
    class WorldRenderHooks;
    
    namespace UI
    {
        class NativeUIFactories;
    }
    
    namespace Resources
    {
        class PooledRenderableItemFactory;
        class ResourceSpatialQueryService;
        
        template <typename T>
        class MeshPool;
        
        namespace Roads
        {
            class RoadStreaming;
            class RoadBuilder;
            class RoadResourceEvictObserver;
            class RoadNamesController;
        }
        
        namespace Buildings
        {
            class BuildingStreaming;
            class BuildingBuilder;
            class ChunkedBuildingBuilder;
        }
        
        namespace Terrain
        {
            class TerrainStreaming;
            
            namespace Placeholder
            {
                class TerrainPlaceholderBuilder;
            }
            
            namespace Heights
            {
                class TerrainHeightProvider;
                class TerrainHeightRepository;
            }
            
            namespace lcm
            {
                class LcmTerrainBuilder;
                class ChunkedLcmTerrainBuilder;
            }
            
            namespace Raster
            {
                class RasterTerrainBuilder;
            }
        }
        
        namespace PlaceNames
        {
            class PlaceNamesStreaming;
            class PlaceNamesBuilder;
            class PlaceNamesController;
            class PlaceNamesRepository;
            class PlaceNamesRenderer;
        }
    }
    
    namespace Streaming
    {
        class PayloadLoadRequestItemFactory;
        class MortonKeyLong;
        class ResourceStream;
        class IStreamingVolume;
        class StreamingController;
        class PayloadCache;
        class BuildPrioritiser;
        class PayloadBuildManager;
        class ResourceNodeCache;
        class StreamingVolumeController;
        class QuadTreeCube;
        class QuadTreeVisibilityUpdater;
        class LoadPrioritiser;
        class QuadTreeNodePool;
        class PayloadPool;
        class PayloadEvicter;
        class SceneGraphUpdater;
        class RedundantNodeEvicter;
        class PayloadPriorityComparator;
        class PayloadSlotStreamMap;
        class LodStreamMap;
        class RegionSelectedResourceStream;
        class NullResourceStream;
        class PayloadPlaceholderBuilder;
        class TerrainPlaceholderInserter;
        
        namespace CoverageTrees
        {
            class CoverageTreeWebRequestService;
        }
    }
    
    namespace Camera
    {
        class CameraModel;
        class ICamera;
        class GlobeCamera;
        class NewGlobeCamera;
    }
    
    namespace DataStructures
    {
        template <typename T>
        class Pool;
    }
    
    namespace DebugRendering
    {
        class DebugShader;
    }
    
    namespace Rendering
    {
        class ItemRenderer;
        class RenderableItem;
        class GLState;
        class RenderContext;
        class IMaterialFactory;
        class EnvironmentFlatteningService;
        class GlBufferPool;
        class WeatherEffectsOverlayShader;
        class WeatherEffectsOverlayRenderer;
        class RenderHooks;
        class LoadingScreen;
    }
    
    namespace Infrastructure
    {
        class ProcessUpdater;
    }
    
    namespace Location
    {
        class ILocationService;
        class IInterestPointProvider;
    }
    
    enum InitialisationState
    {
        InitialisingCoverageTrees = 0,
        DownloadTheme,
        BeginningStreaming,
        StreamingInitialScene,
        Completed
    };
}

namespace ExampleTypes
{
    enum Examples
    {
        FIRST_EXAMPLE=0,
        DebugSphere=0,
        ScreenUnprojectTerrainHeightQuery,
        LoadModel,
        EnvironmentNotifier,
        WebRequest,
        FileIO,
        NavigationGraph,
        ModifiedRendering,
        ToggleTraffic,
        ResourceSpatialQuery,
        EnvironmentFlattening,
        Search,
        KeyboardInput,
        PODAnimation,
        
        LAST_EXAMPLE=PODAnimation
    };
}

struct AppConfiguration;

class AppOnMap
{
private:
    Examples::IExample* m_pExample;
    ExampleTypes::Examples m_selectedExample;
    
    Eegeo::InitialisationState m_initialisationState;
    
    Eegeo::Streaming::CoverageTrees::CoverageTreeWebRequestService* m_pCoverageTreeWebRequestService;
    
    std::vector<Eegeo::DebugRendering::SphereMesh*> m_spheres;
    
    Eegeo::Infrastructure::ProcessUpdater *m_pProcessUpdater;
    
    Eegeo::Camera::CameraModel *m_pCameraModel;
    Eegeo::Camera::ICamera* m_pCameraController;
    Eegeo::Lighting::GlobalLighting *m_pLighting;
    Eegeo::Lighting::GlobalFogging* m_pFogging;
    Eegeo::Rendering::IMaterialFactory *m_pMaterialFactory;
    
    Eegeo::Web::IWebLoadRequestFactory *m_pWebLoadRequestFactory;
    Eegeo::Web::WebLoadRequestQueue* m_pWebLoadRequestQueue;
    Eegeo::Web::WebLoadRequestQueue* m_pPrecacheWebLoadRequestQueue;
    
    Eegeo::Streaming::IStreamingVolume *m_pStreamingVolume;
    Eegeo::Streaming::StreamingController *m_pStreamingController;
    Eegeo::Streaming::BuildPrioritiser *m_pBuildPrioritiser;
    Eegeo::Streaming::ResourceNodeCache *m_pResourceNodeCache;
    Eegeo::Streaming::PayloadBuildManager *m_pBuildManager;
    Eegeo::Streaming::StreamingVolumeController *m_pStreamingVolumeController;
    Eegeo::Streaming::QuadTreeCube *m_pQuadTreeCube;
    Eegeo::Streaming::QuadTreeVisibilityUpdater *m_pQuadTreeVisibilityUpdater;
    Eegeo::Streaming::PayloadPriorityComparator *m_pPayloadPriorityComparator;
    std::vector<Eegeo::Streaming::LodStreamMap*> m_lodStreamMaps;
    Eegeo::Streaming::PayloadSlotStreamMap *m_pPayloadSlotStreamMap;
    Eegeo::Streaming::PayloadPlaceholderBuilder *m_pPayloadPlaceholderBuilder;
    Eegeo::Streaming::TerrainPlaceholderInserter *m_pTerrainPlaceholderInserter;
    
    Eegeo::Streaming::LoadPrioritiser *m_pLoadPrioritiser;
    Eegeo::Streaming::QuadTreeNodePool *m_pNodePool;
    Eegeo::Streaming::PayloadPool *m_pPayloadPool;
    Eegeo::Streaming::PayloadEvicter *m_pPayloadEvicter;
    Eegeo::Streaming::SceneGraphUpdater *m_pSceneGraphUpdater;
    Eegeo::Streaming::RedundantNodeEvicter *m_pNodeEvicter;
    Eegeo::Streaming::PayloadWebLoader* m_pPayloadWebLoader;
    
    Eegeo::Resources::Roads::RoadStreaming *m_pRoadStream;
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> *m_pRoadPool;
    Eegeo::Resources::Roads::Navigation::NavigationGraphRepository* m_railNavGraphRepository;
    Eegeo::Resources::Roads::Navigation::NavigationGraphBuilder* m_railNavGraphBuilder;
    Eegeo::Resources::Roads::Navigation::NavigationGraphLinkManager* m_railNavGraphLinkManager;
    
    Eegeo::Resources::Roads::Navigation::NavigationGraphRepository* m_tramNavGraphRepository;
    Eegeo::Resources::Roads::Navigation::NavigationGraphBuilder* m_tramNavGraphBuilder;
    Eegeo::Resources::Roads::Navigation::NavigationGraphLinkManager* m_tramNavGraphLinkManager;
    
    Eegeo::Resources::Roads::Navigation::NavigationGraphRepository* m_roadNavGraphRepository;
    Eegeo::Resources::Roads::Navigation::NavigationGraphBuilder* m_roadNavGraphBuilder;
    Eegeo::Resources::Roads::Navigation::NavigationGraphLinkManager* m_roadNavGraphLinkManager;
    
    Eegeo::Resources::Roads::Navigation::NavigationGraphDebuggingService* m_pNavigationGraphDebuggingService;
    
    Eegeo::Resources::Roads::ChunkedRoadBuilder *m_pRoadBuilder;
    Eegeo::Resources::Roads::RoadNamesController *m_pRoadNamesController;
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> *m_pRoadNamesPool;
    
    Eegeo::Resources::PlaceNames::PlaceNamesController *m_pPlaceNamesController;
    Eegeo::Resources::PlaceNames::PlaceNamesRenderer *m_pPlaceNamesRenderer;
    
    Eegeo::Font* m_pFont;
    Eegeo::Font* m_pFontOutline;
    
    Eegeo::Resources::ResourceSpatialQueryService *m_pResourceSpatialQueryService;
    
    Eegeo::Resources::Terrain::Placeholder::TerrainPlaceholderBuilder * m_pTerrainPlaceholderBuilder;
    Eegeo::Resources::Terrain::Heights::TerrainHeightRepository * m_pTerrainHeightRepository;
    Eegeo::Resources::Terrain::Heights::TerrainHeightProvider * m_pTerrainHeightProvider;
    Eegeo::Resources::Terrain::TerrainStreaming *m_pTerrainStream;
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> *m_pLcmTerrainPool;
    Eegeo::Resources::Terrain::lcm::ChunkedLcmTerrainBuilder *m_pChunkedLcmTerrainBuilder;
    Eegeo::Resources::Terrain::Raster::RasterTerrainBuilder *m_pRasterTerrainBuilder;
    
    Eegeo::Resources::Buildings::BuildingStreaming *m_pBuildingStream;
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> *m_pBuildingPool;
    Eegeo::Resources::Buildings::ChunkedBuildingBuilder *m_pBuildingBuilder;
    
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> *m_pShadowPool;
    
    Eegeo::Resources::PlaceNames::PlaceNamesStreaming *m_pPlaceNamesStream;
    Eegeo::Resources::PlaceNames::PlaceNamesRepository* m_pPlaceNamesRepository;
    Eegeo::Resources::PlaceNames::PlaceNamesBuilder *m_pPlaceNamesBuilder;
    
    Eegeo::Resources::CityThemes::CityThemeLoader* m_pCityThemesLoader;
    Eegeo::Resources::CityThemes::CityThemeJsonParser* m_pCityThemesJsonParser;
    Eegeo::Resources::CityThemes::CityThemeRepository* m_pCityThemesRepository;
    Eegeo::Resources::CityThemes::CityThemesTextureUpdater* m_pCityThemesTextureUpdater;
    
    Eegeo::Resources::CustomTextures::BuildingCustomTextureRepository *m_pBuildingCustomTextureRepository;
    
    Eegeo::Resources::PooledRenderableItemFactory* m_pGenericPooledRenderableItemFactory;
    Eegeo::Rendering::ItemRenderer *m_pItemRenderer;
    Eegeo::EegeoEnvironmentRendering *m_rendering;
    Eegeo::Rendering::RenderContext* m_renderContext;
    Eegeo::Rendering::EnvironmentFlatteningService* m_pEnvironmentFlatteningService;
    Eegeo::Rendering::GlBufferPool* m_pGlBufferPool;
    Eegeo::WorldRenderHooks* m_pWorldRenderHooks;
    
    Eegeo::Traffic::VehicleModelRepository *m_pVehicleModelRepository;
    Eegeo::Traffic::TrafficSimulation* m_pTrafficSimulation;
    Eegeo::Traffic::TrafficSimulationCellFactory* m_pTrafficSimulationCellFactory;
    Eegeo::Traffic::TrafficSimulationCellsModel* m_pCarCellsModel;
    Eegeo::Traffic::TrafficSimulationCellsModel* m_pRailCellsModel;
    Eegeo::Traffic::TrafficSimulationCellsModel* m_pTramCellsModel;
    Eegeo::Traffic::IVehicleFactory* m_pCarVehicleFactory;
    Eegeo::Traffic::IVehicleFactory* m_pTrainVehicleFactory;
    Eegeo::Traffic::IVehicleFactory* m_pTramVehicleFactory;
    Eegeo::Traffic::VehicleRenderablesModel* m_pTrafficVehicleRenderablesModel;
    Eegeo::Traffic::VehicleRenderer* m_pTrafficVehicleRenderer;
    Eegeo::Traffic::PlaneSimulation *m_pPlaneSimulation;
    
    Eegeo::Weather::WeatherController* m_pWeatherController;
    Eegeo::Rendering::WeatherEffectsOverlayRenderer* m_pWeatherEffectsOverlayRenderer;
    
    Eegeo::Web::PrecacheService *m_pPrecachingService;
    
    Eegeo::Helpers::IFileIO* m_pFileIO;
    Eegeo::Helpers::ITextureFileLoader* m_pTextureLoader;
    Eegeo::Helpers::IHttpCache *m_pHttpCache;
    Eegeo::Helpers::ITaskQueue* m_pTaskQueue;
    
    Eegeo::Location::ILocationService* m_pLocationService;
    Eegeo::Location::IInterestPointProvider* m_pInterestPointProvider;
    
    Eegeo::Blitter* m_pBlitter;
    
    Eegeo::DebugRendering::DebugShader* m_pDebugShader;
    
    Eegeo::Web::WebRequestSet* m_pWebRequestSet;
    
    Eegeo::Helpers::UrlHelpers::IUrlEncoder* m_pUrlEncoder;
    Eegeo::Search::Service::SearchService* m_pSearchService;
    
    Eegeo::UI::NativeUIFactories* m_pNativeInputFactories;
    
    Eegeo::Rendering::LoadingScreen * m_pLoadingScreen;
    std::string m_splashScreenFilename;
    std::string m_environmentResourceUrl;
    
    Eegeo::Weather::CurrentWeatherModel m_currentWeatherTypeModel;
    
    void ConfigureLODStreams(bool enableBuildingLod0);
    void ConfigureEnvironmentResources(bool HACK_adjustTransportUvs, const std::string& themeManifestUrl);
    void ConfigureStreaming(bool isLowSpecDevice, const std::string& apiToken);
    void ConfigureTraffic(bool drivesOnRight);
    void RegisterUpdatableProcesses();
    
    void PreDraw(float dt);
    void DrawWorldPhases(float dt, Eegeo::Rendering::RenderHooks* pRenderHooks);
    void DrawAllPhases(float dt, Eegeo::Rendering::RenderHooks* pRenderHooks);
    void PostDraw(float dt);
    
    void ConfigurePlatformDependentServices();
    
public:
    AppOnMap(AppConfiguration& config);
    ~AppOnMap();
    
    std::string GetEnvironmentResourceURL() const { return m_environmentResourceUrl; }
    std::vector<Eegeo::DebugRendering::SphereMesh*> &GetDebugSpheres() { return m_spheres; }
    Eegeo::DebugRendering::DebugShader &GetDebugShader() { return *m_pDebugShader; }
    
    Eegeo::Camera::CameraModel &GetCameraModel() { return *m_pCameraModel; }
    Eegeo::Camera::ICamera &GetCameraController() { return *m_pCameraController; }
    Eegeo::Lighting::GlobalLighting &GetGlobalLighting() { return *m_pLighting; }
    Eegeo::Lighting::GlobalFogging &GetGlobalFogging() { return *m_pFogging; }
    Eegeo::Rendering::RenderContext& GetRenderContext() { return *m_renderContext; }
    
    Eegeo::Traffic::TrafficSimulation &GetTrafficSimulation() { return *m_pTrafficSimulation; }
    
    
    Eegeo::Streaming::IStreamingVolume &GetStreamingVolume() { return *m_pStreamingVolume; }
    Eegeo::Streaming::StreamingController &GetStreamingController() { return *m_pStreamingController; }
    Eegeo::Streaming::BuildPrioritiser &GetBuildPrioritiser() { return *m_pBuildPrioritiser; }
    Eegeo::Streaming::PayloadBuildManager &GetPayloadBuildManager() { return *m_pBuildManager; }
    Eegeo::Streaming::StreamingVolumeController &GetStreamingVolumeController() { return *m_pStreamingVolumeController; }
    Eegeo::Streaming::QuadTreeCube &GetQuadTreeCube() { return *m_pQuadTreeCube; }
    Eegeo::Streaming::QuadTreeVisibilityUpdater &GetQuadTreeVisibilityUpater() { return *m_pQuadTreeVisibilityUpdater; }
    Eegeo::Streaming::PayloadPriorityComparator &GetPayloadPriorityComparator() { return *m_pPayloadPriorityComparator; }
    std::vector<Eegeo::Streaming::LodStreamMap*> &GetLodStreams() { return m_lodStreamMaps; }
    Eegeo::Streaming::PayloadSlotStreamMap &GetPayloadStreams() { return *m_pPayloadSlotStreamMap; }
    Eegeo::Streaming::PayloadPlaceholderBuilder &GetPayloadPlaceholderBuilder() { return *m_pPayloadPlaceholderBuilder; }
    Eegeo::Streaming::TerrainPlaceholderInserter &GetTerrainPlaceholderInserter() { return *m_pTerrainPlaceholderInserter; }
    
    Eegeo::Streaming::LoadPrioritiser &GetLoadPrioritiser() { return *m_pLoadPrioritiser; }
    Eegeo::Streaming::QuadTreeNodePool &GetQuadTreeNodePool() { return *m_pNodePool; }
    Eegeo::Streaming::PayloadPool &GetPayloadPool() { return *m_pPayloadPool; }
    Eegeo::Streaming::PayloadEvicter &GetPayloadEvicter() { return *m_pPayloadEvicter; }
    Eegeo::Streaming::SceneGraphUpdater &GetSceneGraphUpdater() { return *m_pSceneGraphUpdater; }
    Eegeo::Streaming::RedundantNodeEvicter &GetRedundantNodeEvicter() { return *m_pNodeEvicter; }
    Eegeo::Streaming::ResourceNodeCache &GetResourceNodeCache() { return *m_pResourceNodeCache; }
    Eegeo::Helpers::ITaskQueue &GetTaskQueue() { return *m_pTaskQueue; }
    Eegeo::Rendering::GlBufferPool &GetGlBufferPool() { return *m_pGlBufferPool; }
    
    Eegeo::Resources::ResourceSpatialQueryService &GetResourceSpatialQueryService() { return *m_pResourceSpatialQueryService; }
    
    Eegeo::Resources::Roads::RoadStreaming &GetRoadStreaming() { return *m_pRoadStream; }
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> &GetRoadMeshPool() { return *m_pRoadPool; }
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> &GetRoadNamesMeshPool() { return *m_pRoadNamesPool; }
    Eegeo::Resources::Roads::ChunkedRoadBuilder &GetRoadBuilder() { return *m_pRoadBuilder; }
    Eegeo::Resources::Roads::Navigation::NavigationGraphRepository &GetNavigationGraphRepository() { return *m_roadNavGraphRepository; }
    Eegeo::Resources::Roads::Navigation::NavigationGraphDebuggingService &GetNavigationGraphDebuggingService() { return *m_pNavigationGraphDebuggingService; }
    
    Eegeo::Resources::Terrain::Heights::TerrainHeightProvider &GetTerrainHeightProvider() { return *m_pTerrainHeightProvider; }
    Eegeo::Resources::Terrain::Placeholder::TerrainPlaceholderBuilder &GetTerrainPlaceholderBuilder() { return *m_pTerrainPlaceholderBuilder; }
    Eegeo::Resources::Terrain::TerrainStreaming &GetTerrainStreaming() { return *m_pTerrainStream; }
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> &GetTerrainMeshPool() { return *m_pLcmTerrainPool; }
    Eegeo::Resources::Terrain::lcm::ChunkedLcmTerrainBuilder &GetLcmTerrainBuilder() { return *m_pChunkedLcmTerrainBuilder; }
    Eegeo::Resources::Terrain::Raster::RasterTerrainBuilder &GetRasterTerrainBuilder() { return *m_pRasterTerrainBuilder;}
    
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> &GetShadowMeshPool() { return *m_pShadowPool; }
    
    Eegeo::Resources::Buildings::BuildingStreaming &GetBuildingStreaming() { return *m_pBuildingStream; }
    Eegeo::Resources::MeshPool<Eegeo::Rendering::RenderableItem*> &GetBuildingMeshPool() { return *m_pBuildingPool; }
    Eegeo::Resources::Buildings::ChunkedBuildingBuilder &GetBuildingBuilder() { return *m_pBuildingBuilder; }
    
    Eegeo::Resources::PlaceNames::PlaceNamesStreaming &GetPlaceNamesStreaming() { return *m_pPlaceNamesStream; }
    Eegeo::Resources::PlaceNames::PlaceNamesRepository &GetPlaceNamesRepository() { return *m_pPlaceNamesRepository; }
    
    Eegeo::Resources::PlaceNames::PlaceNamesBuilder &GetPlaceNamesBuilder() { return *m_pPlaceNamesBuilder; }
    Eegeo::Resources::PlaceNames::PlaceNamesController &GetPlaceNamesController() { return *m_pPlaceNamesController; }
    
    Eegeo::Resources::CustomTextures::BuildingCustomTextureRepository& GetBuildingCustomTextureRepository() { return *m_pBuildingCustomTextureRepository; }
    
    Eegeo::Resources::PooledRenderableItemFactory &GetGenericPooledRenderabelItemFactory() { return *m_pGenericPooledRenderableItemFactory; }
    Eegeo::Rendering::ItemRenderer &GetItemRenderer() { return *m_pItemRenderer; }
    Eegeo::Rendering::EnvironmentFlatteningService &GetEnvironmentFlatteningService() { return *m_pEnvironmentFlatteningService; }
    Eegeo::EegeoEnvironmentRendering &GetEnvironmentRendering() { return *m_rendering; }
    
    Eegeo::Traffic::VehicleModelRepository &GetVehicleRepository() { return *m_pVehicleModelRepository; }
    
    Eegeo::UI::NativeUIFactories &GetNativeUIFactories() { return *m_pNativeInputFactories; }
    
    Eegeo::Web::PrecacheService &GetPrecachingService() { return *m_pPrecachingService; }
    Eegeo::Web::IWebLoadRequestFactory &GetWebRequestFactory() { return *m_pWebLoadRequestFactory; }
    
    Eegeo::Helpers::IFileIO &GetFileIO() { return *m_pFileIO; }
    Eegeo::Helpers::IHttpCache &GetHttpCache() { return *m_pHttpCache; }
    
    Eegeo::Helpers::ITextureFileLoader &GetTextureLoader() { return *m_pTextureLoader; }
    Eegeo::Location::ILocationService &GetLocationService() { return *m_pLocationService; }
    Eegeo::Location::IInterestPointProvider &GetInterestPointProvider() { return *m_pInterestPointProvider; }
    
    Eegeo::Weather::CurrentWeatherModel &GetCurrentWeatherTypeModel() { return m_currentWeatherTypeModel; }
    Eegeo::Weather::WeatherController &GetWeatherController() { return *m_pWeatherController; }
    
    Eegeo::Blitter &GetBlitter() { return *m_pBlitter; }
    Eegeo::Font &GetPlaceNameFont() { return *m_pFontOutline; }
    Eegeo::Rendering::LoadingScreen &GetLoadingScreen() { return *m_pLoadingScreen; }
    
    Eegeo::Web::WebLoadRequestQueue& GetWebLoadRequestQueue() { return * m_pWebLoadRequestQueue; }
    
    bool IsSearchServiceAvailable() { return m_pSearchService != NULL; }
    Eegeo::Search::Service::SearchService& GetSearchService()
    {
        Eegeo_ASSERT(m_pSearchService != NULL, "To use the SearchService, ensure you have provided credentials to the AppOnMap constructor!\n")
        return *m_pSearchService;
    }
    
    const bool Initialising() { return m_initialisationState != Eegeo::Completed; }
    void Update(float dt);
    void Draw(float dt, Eegeo::Rendering::RenderHooks* pRenderHooks);
    void Draw(float dt);
    
    void UpdateExample();
    
    //for purposes of example we know it's a globe camera
    Eegeo::Camera::NewGlobeCamera& GetGlobeCamera() { return (Eegeo::Camera::NewGlobeCamera&)*m_pCameraController; }
    
};


struct AppConfiguration
{
    float width;
    float height;
    float pixelScale;
    bool isMultiCore;
    std::string themeManifestUrl;
    std::string environmentResourcesUrl;
    std::string splashScreenFileName;
    std::string apiToken;
    Eegeo::Search::Service::SearchServiceCredentials* pSearchCredentials;
    
    inline static AppConfiguration CreateDefaultAppConfiguration(float width,
                                                                 float height,
                                                                 float pixelScale,
                                                                 bool isMultiCore,
                                                                 const std::string& apiToken)
    {
        AppConfiguration config;
        config.width = width;
        config.height = height;
        config.pixelScale = pixelScale;
        config.isMultiCore = isMultiCore;
        config.themeManifestUrl = "http://cdn1.eegeo.com/mobile-themes/v17/manifest.txt.gz";
        config.environmentResourcesUrl = "http://d2xvsc8j92rfya.cloudfront.net/";
        config.splashScreenFileName = "Default-Landscape@2x~ipad.png";
        config.apiToken = apiToken;
        config.pSearchCredentials = NULL;
        return config;
    }
};

#endif
