#include "PID.h"

/*
* TODO: Complete the PID class.
*/

PID::PID() {

}

PID::~PID() {

}

void PID::Init(double Kp, double Ki, double Kd) {
    this->Kp = Kp;
    this->Ki = Ki;
    this->Kd = Kd;
}

void PID::UpdateError(const double cte) {
    const double steer = -Kp * cte;
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

