#include <XPLMProcessing.h>
#include <XPLMUtilities.h>
#include <XPLMDataAccess.h>
#include <Eigen/Geometry>
#include <memory>
#include <format>
#include <cstdint>
#include <numbers>
#include "sim.hpp"
#include "ui.hpp"
#include "serial.hpp"

#define m_to_cm 100
#define deg_to_rad 3.1415f / 180.0f
#define inhg_to_pa 3386.38867
#define decimaldeg_to_deg 1.0e7

// Data structures expected by ArduPilot
namespace AP {
    typedef struct {
        uint8_t instance;
        float pressure_pa;
        float temperature;
    } baro_data_message_t;
    typedef struct {
        Eigen::Vector3f field;
    } mag_data_message_t;
    typedef struct {
        uint16_t gps_week;
        uint32_t ms_tow;
        uint8_t fix_type;
        uint8_t satellites_in_view;
        float horizontal_pos_accuracy;
        float vertical_pos_accuracy;
        float horizontal_vel_accuracy;
        float hdop;
        float vdop;
        int32_t longitude;
        int32_t latitude;
        int32_t msl_altitude;
        float ned_vel_north;
        float ned_vel_east;
        float ned_vel_down;
    } gps_data_message_t;
    typedef struct {
        Eigen::Vector3f accel;
        Eigen::Vector3f gyro;
        float temperature;
    } ins_data_message_t;
}

namespace Sim {
    namespace DataRef {
        XPLMDataRef accel_x = XPLMFindDataRef("sim/flightmodel/forces/g_axil");
        XPLMDataRef accel_y = XPLMFindDataRef("sim/flightmodel/forces/g_side");
        XPLMDataRef accel_z = XPLMFindDataRef("sim/flightmodel/forces/g_nrml");
        XPLMDataRef gyro_x = XPLMFindDataRef("sim/flightmodel/position/Prad");
        XPLMDataRef gyro_y = XPLMFindDataRef("sim/flightmodel/position/Qrad");
        XPLMDataRef gyro_z = XPLMFindDataRef("sim/flightmodel/position/Rrad");
        XPLMDataRef roll = XPLMFindDataRef("sim/flightmodel/position/phi");
        XPLMDataRef pitch = XPLMFindDataRef("sim/flightmodel/position/theta");
        XPLMDataRef yaw = XPLMFindDataRef("sim/flightmodel/position/psi");
        XPLMDataRef baro = XPLMFindDataRef("sim/weather/barometer_current_inhg");
        XPLMDataRef days = XPLMFindDataRef("sim/time/local_date_days");
        XPLMDataRef seconds = XPLMFindDataRef("sim/time/local_time_sec");
        XPLMDataRef latitude = XPLMFindDataRef("sim/flightmodel/position/latitude");
        XPLMDataRef longitude = XPLMFindDataRef("sim/flightmodel/position/longitude");
        XPLMDataRef elevation = XPLMFindDataRef("sim/flightmodel/position/elevation");
        XPLMDataRef local_vx = XPLMFindDataRef("sim/flightmodel/position/local_vx");
        XPLMDataRef local_vy = XPLMFindDataRef("sim/flightmodel/position/local_vy");
        XPLMDataRef local_vz = XPLMFindDataRef("sim/flightmodel/position/local_vz");
        XPLMDataRef speed = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
        XPLMDataRef airspeed = XPLMFindDataRef("sim/flightmodel/position/true_airspeed");
    }
    struct {
        Eigen::Vector3f accel;
        Eigen::Vector3f gyro;
        Eigen::Vector3f orientation;
        float pressure;
        int day;
        float seconds;
        double latitude;
        double longitude;
        double elevation;
        Eigen::Vector3f gps_vel;
    } state;
    float Loop(
        float inElapsedSinceLastCall,
        float inElapsedTimeSinceLastFlightLoop,
        int inCounter,
        void *inRefcon);
    void UpdateState();
    void ProcessState();
}

float Sim::Loop(
    float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
    int inCounter, void *inRefcon) {
    if (!Serial::IsOpen()) {
        UI::OnSerialDisconnect();
        return 0.0;
    }
    UpdateState();
    ProcessState();
    Serial::Poll();
    return -1.0;
}

// get raw data from xplane
void Sim::UpdateState() {
    state.accel = {
        XPLMGetDataf(DataRef::accel_x),
        XPLMGetDataf(DataRef::accel_y),
        XPLMGetDataf(DataRef::accel_z)
    };
    state.gyro = {
        XPLMGetDataf(DataRef::gyro_x),
        XPLMGetDataf(DataRef::gyro_y),
        XPLMGetDataf(DataRef::gyro_z)
    };
    state.orientation = {
        XPLMGetDataf(DataRef::roll),
        XPLMGetDataf(DataRef::pitch),
        XPLMGetDataf(DataRef::yaw)
    };
    state.pressure = XPLMGetDataf(DataRef::baro);
    state.day = XPLMGetDatai(DataRef::days);
    state.seconds = XPLMGetDataf(DataRef::seconds);
    state.latitude = XPLMGetDatad(DataRef::latitude);
    state.longitude = XPLMGetDatad(DataRef::longitude);
    state.elevation = XPLMGetDatad(DataRef::elevation);
    state.gps_vel = {
        XPLMGetDataf(DataRef::local_vx),
        XPLMGetDataf(DataRef::local_vy),
        XPLMGetDataf(DataRef::local_vz)
    };
}

// convert raw xplane data to ardupilot and send
void Sim::ProcessState() {
    struct {
        char header[4] = { 'H', 'I', 'T', 'L' };
        AP::baro_data_message_t baro;
        AP::mag_data_message_t mag;
        AP::gps_data_message_t gps;
        AP::ins_data_message_t ins;
    } msg;
    // Inertial sensor
    msg.ins.accel = {
        state.accel.x() * GRAVITY_MSS,
        state.accel.y() * GRAVITY_MSS,
        -state.accel.z() * GRAVITY_MSS
    };
    msg.ins.gyro = {
        state.gyro.x(),
        state.gyro.y(),
        state.gyro.z()
    };
    msg.ins.temperature = 25;
    // Altimeter
    msg.baro.instance = 0;
    msg.baro.pressure_pa = state.pressure * inhg_to_pa;
    msg.baro.temperature = 25;
    // Compass
    Eigen::Vector3f orientation_rad = state.orientation;
    Eigen::Quaternionf orientation =
        Eigen::AngleAxisf(orientation_rad.x(), Eigen::Vector3f::UnitX())
        * Eigen::AngleAxisf(orientation_rad.y(), Eigen::Vector3f::UnitY())
        * Eigen::AngleAxisf(orientation_rad.z(), Eigen::Vector3f::UnitZ());
    Eigen::Vector3f north = { 1, 0, 0 };
    msg.mag.field = orientation * north;
    msg.mag.field = { 10.0f, 10.0f, 10.0f };
    msg.mag.field = msg.mag.field.normalized();
    // GPS
    msg.gps.gps_week = 0xFFFF;
    msg.gps.ms_tow = 0;
    msg.gps.fix_type = 3; //OK 3D
    msg.gps.satellites_in_view = 10;
    msg.gps.horizontal_pos_accuracy = 1;
    msg.gps.vertical_pos_accuracy = 1;
    msg.gps.horizontal_vel_accuracy = 1;
    msg.gps.hdop = 100;
    msg.gps.vdop = 100;
    msg.gps.latitude = state.latitude * decimaldeg_to_deg;
    msg.gps.longitude = state.longitude * decimaldeg_to_deg;
    msg.gps.msl_altitude = state.elevation;
    msg.gps.ned_vel_north = -state.gps_vel.z();
    msg.gps.ned_vel_east = state.gps_vel.x();
    msg.gps.ned_vel_down = -state.gps_vel.y();
    Serial::Send(&msg, sizeof(msg));
}