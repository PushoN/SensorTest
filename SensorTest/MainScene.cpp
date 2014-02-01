/********************************************************************
 * File   : MainScene.cpp
 * Project: ToolsDemo
 *
 ********************************************************************
 * Created on 9/21/13 By Nonlinear Ideas Inc.
 * Copyright (c) 2013 Nonlinear Ideas Inc. All rights reserved.
 ********************************************************************
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software. If you use this
 *    software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and
 *    must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include "MainScene.h"
#include "Box2DDebugDrawLayer.h"
#include "GridLayer.h"
#include "DebugLinesLayer.h"
#include "TapDragPinchInput.h"
#include "Notifier.h"
#include "Viewport.h"
#include "Entity.h"
#include "EntityManager.h"
#include "GraphSensorGenerator.h"
#include "GraphSensorManager.h"
#include "SystemContactListener.h"
#include "GraphSensorContactLayer.h"
#include "Box2DShapeCache.h"
#include "SunBackgroundLayer.h"
#include "SpriteBatchLayer.h"
#include "Snake.h"

/* This class generates a graph sensor array on a rectangular
 * grid.
 *
 * Each sensor is a circle with a given radius and a separation
 * between the edges.
 *
 * The size of the world is gleaned from the Viewport.
 */
class GraphSensorGridSquareSensors : public GraphSensorGenerator
{
private:
   // Private constructor.
   GraphSensorGridSquareSensors()
   {
      
   
   }
   
   b2World* _world;
   float32 _diameter;
   float32 _separation;
   
   virtual void GenerateSensors()
   {
      Viewport& vp = Viewport::Instance();
      CCSize worldSize = vp.GetWorldSizeMeters();
      SENSORS_T& sensors = GetSensors();

      // NOTE:  These are REFERENCES to the underlying values.
      // THESE ARE NOT LOCAL VARIABLES.
      int32& cols = Cols();
      int32& rows = Rows();
      
      // Calculate the rows and columns so that there is at least one gap between each
      // shape.
      cols = (uint32)ceil(worldSize.width/_separation);
      rows = (uint32)ceil(worldSize.height/_separation);
      sensors.clear();
      
      if(cols%2 != 0)
         cols++;
      if(rows%2 != 0)
         rows++;
      
      b2BodyDef bodyDef;
      bodyDef.type = b2_staticBody;
      
      /*
      b2CircleShape shape;
      shape.m_radius = _diameter/2;
      shape.m_p = b2Vec2(0,0);
      */
      b2PolygonShape shape;
      shape.SetAsBox(_diameter/2, _diameter/2);
      
      
      b2FixtureDef fixtureDef;
      fixtureDef.shape = &shape;
      fixtureDef.isSensor = true;
      
      GraphSensor* sensor = NULL;
      b2Body* body = NULL;
      b2Fixture* fixture = NULL;
      
      //CCLOG("Rows = %d, Cols = %d",rows,cols);
      
      for(int32 row = 0; row <= rows; ++row)
      {
         for(int32 col = 0; col <= cols; ++col)
         {
            b2Vec2 pos((col-cols/2) * _separation, (row-rows/2) * _separation);
            bodyDef.position = pos;
            
            body = _world->CreateBody(&bodyDef);
            fixture = body->CreateFixture(&fixtureDef);
            sensor = new GraphSensor();
            sensor->SetBody(body);
            sensor->SetIndex(sensors.size());
            fixture->SetUserData(sensor);
            body->SetUserData(sensor);
            
            // We don't need to draw these in box2d debug
            body->SetDebugDraw(false);
            
            sensors.push_back(sensor);
            
            /* Just a little unit testing */
            /*
            int32 calcIdx, calcRow, calcCol;
            
            calcIdx = CalcIndex(row, col);
            CalcIndex(calcIdx,calcRow,calcCol);
            
             CCLOG("AIdx = %04ld, Index = %04d, agrid(%d,%d), grid(%d,%d)",
             sensors.size()-1,
             calcIdx,
             row,
             col,
             calcRow,
             calcCol
             );
            assert(calcIdx == sensors.size()-1);
            assert(row == calcRow);
            assert(col == calcCol);
             */
         }
      }
   }
   
   virtual void GenerateAdjacency()
   {
      SENSORS_ADJ_T& adj = GetAdjacentSensors();
      adj.resize(GetSensorsConst().size());
      const SENSORS_T& sensors = GetSensorsConst();
      int32 row = 0, col = 0;
      for(int idx = 0; idx < sensors.size(); idx++)
      {
         // Get the row, col for this index.
         CalcIndex(idx, row,col);
         // Since this is a "grid", we will look at the
         // 8 cardinal directions as adjacent.
         int32 calcIdx;
         
         calcIdx = CalcIndex(row+1, col+1);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
         
         calcIdx = CalcIndex(row+1, col);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
         
         calcIdx = CalcIndex(row+1, col-1);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
         
         calcIdx = CalcIndex(row, col+1);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
         
         calcIdx = CalcIndex(row, col-1);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
         
         calcIdx = CalcIndex(row-1, col+1);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
         
         calcIdx = CalcIndex(row-1, col);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
         
         calcIdx = CalcIndex(row-1, col-1);
         if(calcIdx >= 0 && calcIdx < sensors.size()) {  adj[idx].push_back(calcIdx); }
      }
   }
   
   virtual int32 CalcIndex(int32 row, int32 col)
   {
      return row * (Cols() + 1) + col;
   }
   
   virtual void CalcIndex(int32 idx, int32& outRow, int32& outCol)
   {
      outRow = idx / (Cols() + 1);
      outCol = idx % (Cols() + 1);
   }

   
public:
   GraphSensorGridSquareSensors(b2World* world, float32 diameter = 2.0f, float32 separation = 2.0f) :
   _world(world),
   _diameter(diameter),
   _separation(separation)
   {
      assert(_diameter > 0.001);
      assert(_separation > 0.001);
   }
   
};


MainScene::MainScene() :
   _entity(NULL)
{
}

MainScene::~MainScene()
{
}

void MainScene::CreateEntity()
{
   Vec2 position(0,0);
   _entity = new Snake();
   _entity->Create(*_world,position, 0);
}

void MainScene::InitSystem()
{
   // Set up the viewport
   static const float32 worldSizeMeters = 100.0;
   
   // Initialize the Viewport
   Viewport::Instance().Init(worldSizeMeters);
   Viewport::Instance().SetScale(1.4f);
   
   EntityManager::Instance().Init();
   GraphSensorManager::Instance().Init();
   SystemContactListener::Instance().Init();
   
}

void MainScene::CreatePhysics()
{   
   _world = new b2World(Vec2(0.0,0.0));
   // Do we want to let bodies sleep?
   // No for now...makes the debug layer blink
   // which is annoying.
   _world->SetAllowSleeping(false);
   _world->SetContinuousPhysics(true);
   
   // Set the contact callback.
   _world->SetContactListener(&SystemContactListener::Instance());
}

void MainScene::CreateSensors()
{
   GraphSensorGridSquareSensors sensorGrid(_world);
   sensorGrid.Create();
   
   GraphSensorManager::Instance().LoadSensors(sensorGrid);
}

bool MainScene::init()
{
   InitSystem();
   return true;
}

MainScene* MainScene::create()
{
   MainScene *pRet = new MainScene();
   if (pRet && pRet->init())
   {
      pRet->autorelease();
      return pRet;
   }
   else
   {
      CC_SAFE_DELETE(pRet);
      return NULL;
   }
}

void MainScene::onEnter()
{
   CCScene::onEnter();
   // Create physical world
   CreatePhysics();
   
   // Populate physical world
   CreateEntity();
   
   // Create the sensor grid
   CreateSensors();
   
   // Add a color background.  This will make it easier on the eyes.
   //addChild(CCLayerColor::create(ccc4(200, 200, 200, 255)));   
   addChild(SunBackgroundLayer::create());
   
   // Adding the debug lines so that we can draw the path followed.
   addChild(DebugLinesLayer::create());
   
   // Touch Input
   addChild(TapDragPinchInput::create(this));
   
   // Box2d Debug
   addChild(Box2DDebugDrawLayer::create(_world));
   
   // Grid
   addChild(GridLayer::create());
   
   // Contact Counts
   addChild(GraphSensorContactLayer::create());
}

void MainScene::onExit()
{
   CCScene::onExit();
}

void MainScene::onEnterTransitionDidFinish()
{
   // Schedule Updates
   scheduleUpdate();
}

void MainScene::onExitTransitionDidStart()
{
   CCScene::onExitTransitionDidStart();
   
   // Turn off updates
   unscheduleUpdate();
}


void MainScene::UpdateEntity()
{
   if(_entity != NULL)
   {
      _entity->Update();
   }
}

void MainScene::UpdatePhysics()
{
   const int velocityIterations = 8;
   const int positionIterations = 1;
   float32 fixedDT = SECONDS_PER_TICK;
   // Instruct the world to perform a single step of simulation. It is
   // generally best to keep the time step and iterations fixed.
   _world->Step(fixedDT, velocityIterations, positionIterations);
   
   SystemContactListener::Instance().NotifyContacts();
}

void MainScene::update(float dt)
{
   UpdateEntity();
   UpdatePhysics();
}




// Handler for Tap/Drag/Pinch Events
void MainScene::TapDragPinchInputTap(const TOUCH_DATA_T& point)
{
   _entity->CommandSeek(Viewport::Instance().Convert(point.pos));
}
void MainScene::TapDragPinchInputLongTap(const TOUCH_DATA_T& point)
{
}
void MainScene::TapDragPinchInputPinchBegin(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   _entity->CommandIdle();
   _camera.PinchBegin(point0, point1);
}
void MainScene::TapDragPinchInputPinchContinue(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   _camera.PinchContinue(point0, point1);
}
void MainScene::TapDragPinchInputPinchEnd(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   _camera.PinchEnd(point0, point1);
}
void MainScene::TapDragPinchInputDragBegin(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   _entity->CommandSeek(Viewport::Instance().Convert(point0.pos));
}
void MainScene::TapDragPinchInputDragContinue(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   _entity->CommandSeek(Viewport::Instance().Convert(point1.pos));
   Notifier::Instance().Notify(NE_RESET_DRAW_CYCLE);
}
void MainScene::TapDragPinchInputDragEnd(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   Notifier::Instance().Notify(NE_RESET_DRAW_CYCLE);
}


void MainScene::SetZoom(float scale)
{
   Viewport::Instance().SetScale(scale);
}




