#include "PID.h"

/*
* TODO: Complete the PID class.
*/

PID::PID() {
    tau_ = 0.1;
    p_error = 0.;
}

PID::~PID() {

}

void PID::Init(double Kp, double Ki, double Kd) {
}

void PID::UpdateError(const double cte) {
    const double steer = -tau_ * cte;
    p_error = steer;
}

double PID::TotalError() {
    return p_error;
}

double PID::SteerValue() {
    double steer_value = TotalError();
    if (steer_value > 1.) {
        steer_value = 1.;
    } else if (steer_value < -1.) {
        steer_value = -1.;
    }

    return steer_value;
}

