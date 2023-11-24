#pragma once
#include <Eigen/Geometry>

#define m_to_cm 100.0f
#define deg_to_rad 3.1415f / 180.0f
#define inhg_to_pa 3386.38867
#define decimaldeg_to_deg 1.0e7

Eigen::Vector3f angularVelocity(Eigen::Quaternionf q1, Eigen::Quaternionf q2, float dt);
Eigen::Quaternionf EulerToQuat(Eigen::Vector3f euler);
Eigen::Vector3f QuatToEuler(Eigen::Quaternionf quat);