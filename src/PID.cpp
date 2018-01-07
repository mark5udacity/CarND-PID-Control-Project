#include "PID.h"

/*
* TODO: Complete the PID class.
*/

PID::PID() {
    this->d_error = INFINITY;
    this->i_error = 0.;
}

PID::~PID() {

}

void PID::Init(double Kp, double Ki, double Kd) {
    this->Kp = Kp;
    this->Ki = Ki;
    this->Kd = Kd;
}

void PID::UpdateError(const double cte) {
    i_error += cte;

    double diff_cte;
    if (d_error == INFINITY) {
        diff_cte = 0;
    } else {
        diff_cte = cte - d_error;
    }

    double steer = -Kp * cte; // proportional
    steer -= Kd * diff_cte;   // differential
    steer -= Ki * i_error;    // integral
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

