//==================================================================
/// main.cpp
///
/// Created by Davide Pasca - 2023/04/27
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include "IncludeGL.h"
#include "DBase.h"
#include "MathBase.h"
#include "ImmGL.h"
#include "MinimalSDLApp.h"
#include "TA_SimpleNN.h"
#include "TA_Train.h"
#include "TA_Trainer.h"
#include "Simulation.h"

// speed of our simulation, as well as display
static constexpr auto FRAME_DT = 1.f / 60.f;

//==================================================================
static constexpr float DISP_CAM_NEAR    = 0.1f;     // near plane (meters)
static constexpr float DISP_CAM_FAR     = 1000.f;   // far plane (meters)

static constexpr IColor4 SKY_COL        = { 0.5f, 0.7f, 1.f, 1.f };

struct DemoParams
{
    float       DISP_CAM_FOV_DEG    = 50.f;       // field of view
    float       DISP_CAM_DIST       = 12;
    float       DISP_CAM_HEIGHT     = 5;
    Float2      DISP_CAM_PY_ANGS    = {10.f, 0.f};
} _sPar;

//==================================================================
class DemoMain
{
public:
    bool                            mShowDebugDraw = true;

    // training variables
    std::unique_ptr<Trainer>     moTrainer;
    size_t                          mCurModelIdx = 0;
    size_t                          mLastEpoch = 0;
    double                          mLastEpochTimeS = 0;
    double                          mLastEpochLenTimeS = 0;
    // periodically updated from the training
    std::vector<Tensor>          mBestPool;
    std::vector<ParamsInfo>      mBestPInfos;

    // simulation to play/test
    bool                            mPlayEnabled = true;
    uint32_t                        mPlaySeed = 0;
    std::unique_ptr<Simulation>     moPlaySim;
    std::unique_ptr<SimpleNN>       moPlayNet;

    DemoMain()
    {
        // start to train right away
        doStartTraining();
    }

    void AnimateDemo(float dt);
    void DrawDemo(ImmGL& immgl);
    Float3 GetOurVehiclePos() const;

    void HandleUI(size_t frameCnt);

private:
    void handleTrainUI();
    void handlePlayUI();

    void doStartTraining();
    void animateTrainer();
} _demoMain;

//==================================================================
inline double GetSteadyTimeS()
{
    return
        (double)std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch() ).count() / 1e6;
}

//==================================================================
static IColor4 hueToColor(float hue)
{
    // https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
    const auto C = 1.0f;
    const auto X = C * (1.0f - std::abs( std::fmod(hue / 60.0f, 2.0f) - 1.0f ));
    const auto m = 0.0f;

    if (hue >=   0.0f && hue <  60.0f) return {C, X, 0.0f, 1}; else
    if (hue >=  60.0f && hue < 120.0f) return {X, C, 0.0f, 1}; else
    if (hue >= 120.0f && hue < 180.0f) return {0.0f, C, X, 1}; else
    if (hue >= 180.0f && hue < 240.0f) return {0.0f, X, C, 1}; else
    if (hue >= 240.0f && hue < 300.0f) return {X, 0.0f, C, 1}; else
                                       return {C, 0.0f, X, 1};
}

//==================================================================
inline void debugDraw(auto &immgl, const Vehicle& vh)
{
    static constexpr auto PI2 = 2*glm::pi<float>();

    const auto probeAngLen = PI2 / Vehicle::PROBES_N;

    const auto fwdSca = Float3(1,1,-1);

    // draw the vehicle's probes
    for (size_t i=0; i < Vehicle::PROBES_N; ++i)
    {
        // calc probe min and max angle
        const auto probeAngMin = probeAngLen * ((float)i - 0.5f);
        const auto probeAngMax = probeAngMin + probeAngLen;

        const auto probeCol =
            hueToColor(360.f * (float)i / (float)Vehicle::PROBES_N) * IColor4(0.7f, 0.7f, 0.7f, 0.3f);

        const auto drawDist = vh.mSens[Vehicle::SENS_PROBE_FIRST_UNITDIST + i] * VH_PROBE_RADIUS;

        auto makeRotDist = [&](float ang)
        {
            return Float3{
                drawDist * sinf(ang),
                0,
                drawDist * cosf(ang),
            };
        };

        // slightly above the vehicle
        const auto basePos = vh.mPos + Float3(0, 0.3f, 0);

        const auto probePos    = basePos;
        const auto probePosMin = basePos + fwdSca * makeRotDist(probeAngMin);
        const auto probePosMax = basePos + fwdSca * makeRotDist(probeAngMax);
        immgl.DrawTri({probePos, probePosMin, probePosMax}, probeCol);
    }
}

//==================================================================
static void drawVehicle(ImmGL& immgl, const Vehicle& vh)
{
    const auto x0 = vh.mPos[0] - VH_WIDTH  * 0.5f;
    const auto x1 = vh.mPos[0] + VH_WIDTH  * 0.5f;
    const auto z0 = vh.mPos[2] - VH_LENGTH * 0.5f;
    const auto z1 = vh.mPos[2] + VH_LENGTH * 0.5f;

    const std::array<IFloat3,4> vpos = {
        IFloat3{x0, vh.mPos[1], z0},
        IFloat3{x1, vh.mPos[1], z0},
        IFloat3{x0, vh.mPos[1], z1},
        IFloat3{x1, vh.mPos[1], z1},
    };

    static constexpr auto OWN_COL          = IColor4{1.0f,0.0f,0.0f,1.f};
    static constexpr auto NPC_COL          = IColor4{0.0f,0.0f,1.0f,1.f};
    static constexpr auto NPC_STRANDED_COL = IColor4{0.5f,0.0f,1.0f,1.f};

    auto isStranded = vh.mSpeed < 0.001f;

    const auto baseCol = vh.mIsNPC
        ? (isStranded ? NPC_STRANDED_COL : NPC_COL)
        : OWN_COL;

    const auto frontCol = IColor4{
        baseCol[0] * 0.7f,
        baseCol[1] * 0.7f,
        baseCol[2] * 0.7f,
        baseCol[3]};
    const auto backCol = IColor4{
        baseCol[0] * 0.9f,
        baseCol[1] * 0.9f,
        baseCol[2] * 0.9f,
        baseCol[3]};

    const std::array<IColor4,4> cols = {
        frontCol,
        frontCol,
        backCol,
        backCol,
    };

    immgl.DrawQuad(vpos, cols);
}

//==================================================================
static void drawRoad(
        ImmGL& immgl,
        size_t idxSta,
        size_t idxEnd)
{
    // two levels of gray for asphalt, float values
    static IColor4 baseCols[2] = {
        { 0.4f, 0.4f, 0.4f, 1.f },
        { 0.5f, 0.5f, 0.5f, 1.f },
    };
    static IColor4 staCol = { 0.2f, 0.8f, 0.2f, 1.f };
    static IColor4 endCol = { 0.8f, 0.2f, 0.2f, 1.f };

    static IColor4 ROAD_COL_LANEVSTRIP = { 0.9f, 0.9f, 0.9f, 1.f };
    static IColor4 ROAD_COL_OUTSIDE    = { 0.2f, 0.4f, 0.05f, 1.f };

    const auto laneW = SLAB_WIDTH / (float)ROAD_LANES_N;
    const auto vstripW = laneW * 0.1f;

    const auto ROAD_OUT_Y = -0.01f;
    const auto ROAD_Y = 0.0f;
    const auto VSTRIP_Y = 0.01f;

    for (size_t idx=idxSta; idx < idxEnd; ++idx)
    {
        const auto x0 = -SLAB_WIDTH * 0.5f;
        const auto x1 =  SLAB_WIDTH * 0.5f;
        const auto z0 = (float)(idx  ) * -SLAB_DEPTH;
        const auto z1 = (float)(idx+1) * -SLAB_DEPTH;

        {
            // draw the outside of the road
            const auto coe = idx & 1 ? 0.9f : 1.0f;
            const auto col = ROAD_COL_OUTSIDE * IColor4{coe,coe,coe,1.f};

            // quad from left edge of screen to x0 or road
            {
                const auto xl = -SLAB_DEPTH * 20 * (z0+1);

                const std::array<IFloat3,4> pos = {
                    IFloat3{x0, ROAD_OUT_Y, z0},
                    IFloat3{xl, ROAD_OUT_Y, z0},
                    IFloat3{x0, ROAD_OUT_Y, z1},
                    IFloat3{xl, ROAD_OUT_Y, z1},
                };
                immgl.DrawQuad(pos, col);
            }
            // quad from right edge of screen to x1 or road
            {
                const auto xr = SLAB_DEPTH * 20 * (z0+1);

                const std::array<IFloat3,4> pos = {
                    IFloat3{x1, ROAD_OUT_Y, z0},
                    IFloat3{xr, ROAD_OUT_Y, z0},
                    IFloat3{x1, ROAD_OUT_Y, z1},
                    IFloat3{xr, ROAD_OUT_Y, z1},
                };
                immgl.DrawQuad(pos, col);
            }

        }

        const std::array<IFloat3,4> vpos = {
            IFloat3{x0, ROAD_Y, z0},
            IFloat3{x1, ROAD_Y, z0},
            IFloat3{x0, ROAD_Y, z1},
            IFloat3{x1, ROAD_Y, z1},
        };

        const auto col =
            (idx == SLAB_STA_IDX)
                ? staCol
                : (idx == SLAB_END_IDX)
                    ? endCol
                    : baseCols[idx % std::size(baseCols)];

        immgl.DrawQuad(vpos, col);

        if (idx & 1)
        {
            // vstrips that divide the lanes
            for (size_t i=1; i < ROAD_LANES_N; ++i)
            {
                const auto vs_x0 = x0 + laneW * (float)i - vstripW * 0.5f;
                const auto vs_x1 = vs_x0 + vstripW;

                const std::array<IFloat3,4> vpos = {
                    IFloat3{vs_x0, VSTRIP_Y, z0},
                    IFloat3{vs_x1, VSTRIP_Y, z0},
                    IFloat3{vs_x0, VSTRIP_Y, z1},
                    IFloat3{vs_x1, VSTRIP_Y, z1},
                };

                immgl.DrawQuad(vpos, ROAD_COL_LANEVSTRIP);
            }
        }
    }
}

//==================================================================
static std::vector<size_t> makeLayerNs(size_t insN, size_t outsN)
{
    return std::vector<size_t>{
        insN,
        std::max((size_t)(insN * 1.25), outsN),
        std::max((size_t)(insN * 0.75), outsN),
        std::max((size_t)(insN * 0.25), outsN),
        outsN};
}

//==================================================================
void DemoMain::AnimateDemo(float dt)
{
    // animate the play/display simulation
    if (mPlayEnabled && (!moPlaySim || !moPlaySim->IsSimRunning()))
    {
        if (!mBestPool.empty())
        {
            moPlayNet = std::make_unique<SimpleNN>(
                mBestPool[0],
                makeLayerNs(Vehicle::SENS_N, Vehicle::CTRL_N));

            moPlaySim = std::make_unique<Simulation>(
                mPlaySeed,
                moPlayNet.get());
        }
    }
    if (moPlaySim)
        moPlaySim->AnimateSim(dt);

    // animate the trainer
    if (moTrainer)
        animateTrainer();
}

//==================================================================
void DemoMain::animateTrainer()
{
    if (!moTrainer)
        return;

    if (const auto curEpoch = moTrainer->GetCurEpochN(); curEpoch != mLastEpoch)
    {
        const auto curTimeS = GetSteadyTimeS();
        mLastEpochLenTimeS = curTimeS - mLastEpochTimeS;
        mLastEpoch = curEpoch;
        mLastEpochTimeS = curTimeS;
    }

    auto& fut = moTrainer->GetTrainerFuture();
    // if the future is valid and it's ready
    if (fut.valid() && fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        moTrainer->LockViewBestPool([&](const auto&, const auto& infos)
        {
            if (infos.empty())
                printf("Training ended.");
            else
                printf("Training ended. Best network: %s, fitness:%f",
                    infos.front().MakeStrID().c_str(),
                    infos.front().ci_fitness);
        });

        moTrainer.reset();
    }
}

//==================================================================
void DemoMain::DrawDemo(ImmGL& immgl)
{
    // draw the road
    drawRoad(immgl, 0, SLAB_MAX_N);

    if (moPlaySim)
    {
        // draw the vehicles
        for (auto& vh : moPlaySim->GetVehicles())
            drawVehicle(immgl, vh);

        // draw the debug stuff
        if (_demoMain.mShowDebugDraw)
            debugDraw(immgl, moPlaySim->GetVehicles()[0]);
    }
}

//==================================================================
Float3 DemoMain::GetOurVehiclePos() const
{
    if (moPlaySim)
        return moPlaySim->GetVehicles()[0].mPos;

    return Float3{0.f,0.f,0.f};
}

//==================================================================
void DemoMain::doStartTraining()
{
    Trainer::Params par;

    // Setup the layers structure
    par.layerNs = makeLayerNs(Vehicle::SENS_N, Vehicle::CTRL_N);

    // Maximum number of epochs for the training
    par.maxEpochsN = 10000;

    // Fitness calculation function (in our cases it runs and evaluates a simulation)
    par.calcFitnessFn = [](const auto &net, std::atomic<bool>& reqShutdown)
    {
        double totFitness = 0;
        // run a simulation for each variant
        for (size_t sidx=0; sidx < SIM_TRAIN_VARIANTS_N; ++sidx)
        {
            // We start with a random seed from a base that should not intersect with the validation set
            // e.g. Don't want to train on seed 0, 1 and then validate on 0, 1
            const auto seed = (uint32_t)(sidx + SIM_TRAIN_SEED_BASE);

            // create a simulation for the given scenario and neural net
            auto oSim = std::make_unique<Simulation>(seed, &net);

            // run to completion (includes timeout)
            while (oSim->IsSimRunning() && !reqShutdown)
                oSim->AnimateSim(FRAME_DT);

            totFitness += oSim->GetSimScore();
        }

        return totFitness / SIM_TRAIN_VARIANTS_N;
    };

    // Do create the trainer
    moTrainer = std::make_unique<Trainer>(par);

    // Reset our epoch tracking
    mLastEpoch = 0;
    mLastEpochTimeS = GetSteadyTimeS();
}

#ifdef ENABLE_IMGUI
//==================================================================
static inline auto guiHeader = []( const std::string &name, bool defOpen )
{
    return ImGui::CollapsingHeader(
                (name + "##head").c_str(),
                defOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
};

//==================================================================
void DemoMain::handleTrainUI()
{
    // handle training
    if (moTrainer)
    {
        if (ImGui::Button("Stop Training"))
        {
            moTrainer->ReqShutdown();
        }
        ImGui::SameLine();
        ImGui::Text("Variants:%zu", SIM_TRAIN_VARIANTS_N);
        ImGui::SameLine();
        ImGui::Text("Epoch:%zu...", moTrainer->GetCurEpochN());
    }
    else
    {
        if (ImGui::Button("Start Training"))
        {
            doStartTraining();
        }
        ImGui::SameLine();
        ImGui::Text("Model: Model 1");
    }

    if (moTrainer)
    {
        if (mLastEpochLenTimeS)
        {
            ImGui::Text("Epoch time: %.1fs", mLastEpochLenTimeS);
            ImGui::Text("Epochs per hour: %.1f", 60*60 / mLastEpochLenTimeS);
        }
        else
        {
            ImGui::Text("Epoch time: -");
            ImGui::Text("Epochs per hour: -");
        }
    }

    if (guiHeader("Brains", true))
    {
        static size_t SHOW_TOP_N = 5;
        if (ImGui::BeginTable("TopBrains", 5, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableHeadersRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s","");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Epoch");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Num");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("Fitness");

            // copy the latest best params
            if (moTrainer)
                moTrainer->LockViewBestPool([this](const auto& paramsPool, const auto& infos)
                {
                    // Copy the latest best params
                    mBestPool = paramsPool;
                    mBestPInfos = infos;
                });

            for (size_t i=0; i < std::min(SHOW_TOP_N, mBestPInfos.size()); ++i)
            {
                const auto& ci = mBestPInfos[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%zu", i);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", ci.ci_epochIdx);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", ci.ci_popIdx);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%f", ci.ci_fitness);
                ImGui::TableSetColumnIndex(4);
            }
            ImGui::EndTable();
        }
    }
}

//==================================================================
void DemoMain::handlePlayUI()
{
    // edit field for seed
    ImGui::Checkbox("Auto Play", &mPlayEnabled);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputScalar("Seed", ImGuiDataType_U32, &mPlaySeed);

    if (moPlaySim)
    {
        ImGui::BeginTable("##simParams", 2,
                ImGuiTableFlags_RowBg
                | ImGuiTableFlags_BordersInner
                | ImGuiTableFlags_SizingStretchSame);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        auto nameAndValue = [](const char* name, const char* fmt, ...)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            char buf[256];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);
            ImGui::Text("%s", buf);
        };

        // simulation parameters
        nameAndValue("Run Time", "%.1f s", moPlaySim->GetRunTimeS());
        nameAndValue("Score", "%.1f", moPlaySim->GetSimScore());
        nameAndValue("Hit Vehicle", "%s", moPlaySim->HasHitVehicle() ? "yes" : "no");
        nameAndValue("Hit Curb", "%s", moPlaySim->HasHitCurb() ? "yes" : "no");
        nameAndValue("Arrived", "%s", moPlaySim->HasArrived() ? "yes" : "no");

        ImGui::EndTable();
    }
}

//==================================================================
void DemoMain::HandleUI(size_t frameCnt)
{
    if ( guiHeader( "Display", true ) )
    {
        ImGui::SliderFloat( "Camera FOV", &_sPar.DISP_CAM_FOV_DEG, 10.f, 120.f );
        ImGui::SliderFloat( "Camera Dist", &_sPar.DISP_CAM_DIST, 0.f, DISP_CAM_FAR/10 );
        ImGui::SliderFloat2( "Camera Pitch/Yaw", &_sPar.DISP_CAM_PY_ANGS[0], -180, 180 );
        ImGui::NewLine();
        ImGui::Checkbox("Debug Draw", &mShowDebugDraw);
    }

    if (guiHeader("Train", true))
        handleTrainUI();

    if (guiHeader("Play Simulation", true))
        handlePlayUI();
}
#endif

//==================================================================
int main( int argc, char *argv[] )
{
    MinimalSDLApp app( argc, argv, 1200, 750, 0
                    | MinimalSDLApp::FLAG_OPENGL
                    | MinimalSDLApp::FLAG_RESIZABLE
                    );

    ImmGL immgl;

    // begin the main/rendering loop
    for (size_t frameCnt=0; ; ++frameCnt)
    {
        // begin the frame (or get out)
        if ( !app.BeginFrame() )
            break;

#ifdef ENABLE_IMGUI
        app.DrawMainUIWin( [&]() { _demoMain.HandleUI(frameCnt); } );
#endif
        glViewport(0, 0, app.GetDispSize()[0], app.GetDispSize()[1]);
        glClearColor(SKY_COL[0], SKY_COL[1], SKY_COL[2], 1.0f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glEnable( GL_DEPTH_TEST );

        immgl.ResetStates();
        immgl.SetBlendAlpha();

        // obj -> world matrix
        auto world_obj = Matrix44( 1.f );

        // camera -> world matrix
        auto cam_world = [&]()
        {
            auto m = Matrix44( 1.f );
            m = glm::translate( m, Float3(0.0f, -_sPar.DISP_CAM_HEIGHT, -_sPar.DISP_CAM_DIST) );
            m = glm::rotate( m, DEG2RAD(_sPar.DISP_CAM_PY_ANGS[0]), Float3(1, 0, 0) );
            m = glm::rotate( m, DEG2RAD(_sPar.DISP_CAM_PY_ANGS[1]), Float3(0, 1, 0) );

            // follow our vehicle
            m = glm::translate(m, Float3(
                        0.0f,
                        0.0f,
                        -_demoMain.GetOurVehiclePos()[2]));
            return m;
        }();

        c_auto [curW, curH] = app.GetDispSize();

        // camera -> projection matrix
        c_auto proj_camera = glm::perspective(
                                DEG2RAD( _sPar.DISP_CAM_FOV_DEG ),  // FOV
                                (float)curW / curH,                       // aspect ratio
                                DISP_CAM_NEAR,
                                DISP_CAM_FAR );

        // obj -> proj matrix
        c_auto proj_obj = proj_camera * cam_world * world_obj;

        // set the obj -> proj matrix
        immgl.SetMtxPS( proj_obj );

        _demoMain.AnimateDemo(FRAME_DT);

        _demoMain.DrawDemo(immgl);

        immgl.FlushStdList();

        // end of the frame (will present)
        app.EndFrame();
    }

    return 0;
}
