//  Copyright (c) 2014 eeGeo. All rights reserved.

#ifndef __ExampleApp__CameraTransitionExample__
#define __ExampleApp__CameraTransitionExample__

#include <iostream>
#include "IExample.h"
#include "EegeoWorld.h"
#include "Location.h"
#include "GlobeCamera.h"
#include "VectorMath.h"

namespace Examples
{
    class CameraTransitioner
    {
    public:
        CameraTransitioner(Eegeo::Camera::GlobeCamera::GlobeCameraController& cameraController,
                           Eegeo::Location::IInterestPointProvider& interestPointProvider);
        
        void StartTransitionTo(Eegeo::dv3 newInterestPoint, double distanceFromInterest, bool jumpIfFarAway);
        void StartTransitionTo(Eegeo::dv3 newInterestPoint, double distanceFromInterest, float newHeading, bool jumpIfFarAway);
        void StopCurrentTransition();
        void Update(float dt);
        
        const bool IsTransitioning() const { return m_isTransitioning; }
        
    private:
        bool ShouldJumpTo(Eegeo::dv3 newInterestPoint);
        
        Eegeo::Camera::GlobeCamera::GlobeCameraController& m_cameraController;
        Eegeo::Location::IInterestPointProvider& m_interestPointProvider;
        Eegeo::dv3 m_startTransitionInterestPoint;
        Eegeo::dv3 m_endTransitionInterestPoint;
        double m_startInterestDistance;
        double m_endInterestDistance;
        float m_startTransitionHeading;
        float m_endTransitionHeading;
        float m_transitionTime;
        float m_transitionDuration;
        bool m_isTransitioning;
    };

    /*!
     *  CameraTransitionExample demonstrates the ability to ease the camera position from it's current location to a destination and back again
     */
    class CameraTransitionExample : public IExample
    {
    private:
        Eegeo::Camera::GlobeCamera::GlobeCameraController& m_cameraController;
        Eegeo::Location::IInterestPointProvider& m_interestPointProvider;
        CameraTransitioner m_transitioner;
        bool m_firstPoint;
        
        void Transition();

    public:
        CameraTransitionExample(Eegeo::Camera::GlobeCamera::GlobeCameraController& cameraController,
                                Eegeo::Location::IInterestPointProvider& interestPointProvider);
        
        void Start() {}
        void EarlyUpdate(float dt);
        void Update(float dt) { }
        void Draw() {}
        void Suspend() {}
        
        void UpdateCamera(Eegeo::Camera::GlobeCamera::GlobeCameraController* pGlobeCameraController,
                                  Eegeo::Camera::GlobeCamera::GlobeCameraTouchController* pCameraTouchController,
                                  float dt);
    };
}

#endif /* defined(__ExampleApp__CameraTransitionExample__) */
