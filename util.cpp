#include <Eigen/Geometry>
#include "util.hpp"

// https://mariogc.com/post/angular-velocity-quaternions/
Eigen::Vector3f angularVelocity(Eigen::Quaternionf q1, Eigen::Quaternionf q2, float dt) {
    return Eigen::Vector3f({
        q1.w() * q2.x() - q1.x() * q2.w() - q1.y() * q2.z() + q1.z() * q2.y(),
        q1.w() * q2.y() + q1.x() * q2.z() - q1.y() * q2.w() - q1.z() * q2.x(),
        q1.w() * q2.z() - q1.x() * q2.y() + q1.y() * q2.x() - q1.z() * q2.w()
        }) * (2 / dt) / deg_to_rad;
}

Eigen::Quaternionf EulerToQuat(Eigen::Vector3f euler) {
    float phi = euler.x() * deg_to_rad / 2;
    float theta = euler.y() * deg_to_rad / 2;
    float psi = (-euler.z() - 180) * deg_to_rad / 2;
    return {
        cos(phi) * cos(theta) * sin(psi) - sin(phi) * sin(theta) * cos(psi),
        cos(phi) * sin(theta) * cos(psi) + sin(phi) * cos(theta) * sin(psi),
        -cos(phi) * sin(theta) * sin(psi) + sin(phi) * cos(theta) * cos(psi),
        cos(phi) * cos(theta) * cos(psi) + sin(phi) * sin(theta) * sin(psi)
    };
}

Eigen::Vector3f QuatToEuler(Eigen::Quaternionf quat) {
    return quat.toRotationMatrix().eulerAngles(0, 1, 2) / deg_to_rad;
}