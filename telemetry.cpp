#include <XPLMProcessing.h>
#include <XPLMUtilities.h>
#include <XPLMDataAccess.h>
#include <Eigen/Geometry>
#include <memory>
#include <format>
#include <cmath>
#include <numbers>
#include "main.hpp"
#include "telemetry.hpp"
#include "calibration.hpp"
#include "ui.hpp"
#include "serial.hpp"
#include "util.hpp"

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
    typedef struct {
        float differential_pressure; // Pa
        float temperature; // degC
    } airspeed_data_message_t;
}

namespace Telemetry {
    namespace DataRef {
        XPLMDataRef accel_x = XPLMFindDataRef("sim/flightmodel/forces/g_axil");
        XPLMDataRef accel_y = XPLMFindDataRef("sim/flightmodel/forces/g_side");
        XPLMDataRef accel_z = XPLMFindDataRef("sim/flightmodel/forces/g_nrml");
        XPLMDataRef gyro_x = XPLMFindDataRef("sim/flightmodel/position/P");
        XPLMDataRef gyro_y = XPLMFindDataRef("sim/flightmodel/position/Q");
        XPLMDataRef gyro_z = XPLMFindDataRef("sim/flightmodel/position/R");
        XPLMDataRef quat = XPLMFindDataRef("sim/flightmodel/position/q");
        XPLMDataRef baro = XPLMFindDataRef("sim/weather/barometer_current_inhg");
        XPLMDataRef temperature = XPLMFindDataRef("sim/weather/temperature_ambient_c");
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
        XPLMDataRef density = XPLMFindDataRef("sim/weather/rho");
        XPLMDataRef engine_running = XPLMFindDataRef("sim/flightmodel/engine/ENGN_running");
        XPLMDataRef engine_rads = XPLMFindDataRef("sim/flightmodel/engine/ENGN_tacrad");
        XPLMDataRef engine_power = XPLMFindDataRef("sim/flightmodel/engine/ENGN_power");
        XPLMDataRef engine_max_power = XPLMFindDataRef("sim/aircraft/engine/acf_pmax_per_engine");
        XPLMDataRef throttle = XPLMFindDataRef("sim/flightmodel/engine/ENGN_thro_use");
    }
    struct {
        Eigen::Vector3f accel;
        Eigen::Vector3f gyro;
        Eigen::Quaternionf rot;
        float pressure;
        float temperature;
        int day;
        float seconds;
        double latitude;
        double longitude;
        double elevation;
        Eigen::Vector3f gps_vel;
        uint8_t gps_fix;
        float dynamic_pressure;
    } state;
    int reset = false;
    float reset_timer = 0;
    void UpdateState();
    void ProcessState();
}

void Telemetry::Send() {
    UpdateState();
    ProcessState();
}

// get raw data from xplane
void Telemetry::UpdateState() {
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
    XPLMGetDatavf(DataRef::quat, &state.rot.w(), 0, 1);
    XPLMGetDatavf(DataRef::quat, state.rot.vec().data(), 1, 3);
    state.pressure = XPLMGetDataf(DataRef::baro);
    state.temperature = XPLMGetDataf(DataRef::temperature);
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
    state.gps_fix = 3;
    state.dynamic_pressure = XPLMGetDataf(DataRef::density) * pow(XPLMGetDataf(DataRef::airspeed), 2) / 2;
}

// convert raw xplane data to ardupilot and send
void Telemetry::ProcessState() {
    struct {
        char header[4] = { 'H', 'I', 'T', 'L' };
        int type = 0;
    } header;
    struct {
        AP::baro_data_message_t baro;
        AP::mag_data_message_t mag;
        AP::gps_data_message_t gps;
        AP::ins_data_message_t ins;
        AP::airspeed_data_message_t aspd;
        float q1 = state.rot.w();
        float q2 = state.rot.x();
        float q3 = state.rot.y();
        float q4 = state.rot.z();
        EFI_State efi;
    } msg;
    // Inertial sensor
    if (!Calibration::IsEnabled()) {
        msg.ins.accel = -state.accel * GRAVITY_MSS;
    } else {
        // plane has no acceleration when frozen mid air during calibration,
        // so here it is faked
        Eigen::Vector3f down = { 0, 0, -GRAVITY_MSS };
        msg.ins.accel = state.rot.conjugate() * down;
    }
    msg.ins.gyro = state.gyro * deg_to_rad;
    msg.ins.temperature = 25;
    // Barometer
    msg.baro.instance = 0;
    msg.baro.pressure_pa = state.pressure * inhg_to_pa;
    msg.baro.temperature = state.temperature;
    // Compass
    Eigen::Vector3f north = { 1, 0, 0 };
    msg.mag.field = state.rot.conjugate() * north;
    // GPS
    msg.gps.gps_week = 0xFFFF;
    msg.gps.ms_tow = 0;
    msg.gps.fix_type = state.gps_fix;
    msg.gps.satellites_in_view = 10;
    msg.gps.horizontal_pos_accuracy = 1;
    msg.gps.vertical_pos_accuracy = 1;
    msg.gps.horizontal_vel_accuracy = 1;
    msg.gps.hdop = 1;
    msg.gps.vdop = 1;
    msg.gps.latitude = state.latitude * decimaldeg_to_deg;
    msg.gps.longitude = state.longitude * decimaldeg_to_deg;
    msg.gps.msl_altitude = state.elevation * m_to_cm;
    msg.gps.ned_vel_north = -state.gps_vel.z();
    msg.gps.ned_vel_east = state.gps_vel.x();
    msg.gps.ned_vel_down = -state.gps_vel.y();
    // Airspeed
    msg.aspd.differential_pressure = state.dynamic_pressure;
    msg.aspd.temperature = state.temperature;
    // EFI
    int engine_running;
    XPLMGetDatavi(DataRef::engine_running, &engine_running, 0, 1);
    msg.efi.engine_state = engine_running == 2 ? Engine_State::RUNNING : Engine_State::STOPPED;
    msg.efi.general_error = false;
    msg.efi.crankshaft_sensor_status = Crankshaft_Sensor_Status::NOT_SUPPORTED;
    msg.efi.temperature_status = Temperature_Status::NOT_SUPPORTED;
    msg.efi.fuel_pressure_status = Fuel_Pressure_Status::NOT_SUPPORTED;
    msg.efi.oil_pressure_status = Oil_Pressure_Status::NOT_SUPPORTED;
    msg.efi.detonation_status = Detonation_Status::NOT_SUPPORTED;
    msg.efi.misfire_status = Misfire_Status::NOT_SUPPORTED;
    msg.efi.debris_status = Debris_Status::NOT_SUPPORTED;
    float engine_power;
    float engine_max_power;
    XPLMGetDatavf(DataRef::engine_power, &engine_power, 0, 1);
    XPLMGetDatavf(DataRef::engine_max_power, &engine_max_power, 0, 1);
    msg.efi.engine_load_percent = engine_power / engine_max_power;
    float engine_rads;
    XPLMGetDatavf(DataRef::engine_rads, &engine_rads, 0, 1);
    msg.efi.engine_speed_rpm = static_cast<uint32_t>(engine_rads * 60.0f / (2 * std::numbers::pi));
    XPLMGetDatavf(DataRef::throttle, &msg.efi.throttle_out, 0, 1);
    msg.efi.throttle_position_percent = static_cast<uint8_t>(msg.efi.throttle_out * 100);
    msg.efi.ignition_voltage = -1;
    Serial::Send(&header, sizeof(header));
    Serial::Send(&msg, sizeof(msg));
}

void Telemetry::RestartArdupilot() {
    struct {
        char header[4] = { 'H', 'I', 'T', 'L' };
        int type = 1;
    } msg;
    Serial::Send(&msg, sizeof(msg));
}