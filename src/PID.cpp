#include <iostream>
#include "PID.h"

PID::PID() {

}

PID::~PID() {

}

void PID::Init(double Kp, double Ki, double Kd) {
    std::cout << "~~~ Initializing PID with Kp=" << Kp << ", Ki=" << Ki << ", and Kd=" << Kd << " ~~~\n";
    this->Kp = Kp;
    this->Ki = Ki;
    this->Kd = Kd;

    p_error = 0.;
    i_error = 0.;
    d_error = INFINITY;
}

void PID::UpdateError(const double cte) {
    i_error += cte * .1;
    i_error *= .90; // Reduce by 10 percent!  Just like from Twiddle code

    double diff_cte;
    if (d_error == INFINITY) {
        diff_cte = 0.;
    } else {
        diff_cte = cte - d_error;
    }

    double steer = -Kp * cte; // proportional
    steer -= Kd * diff_cte;   // differential
    steer -= Ki * i_error;    // integral
    p_error = steer;
    d_error = p_error;
}

double PID::TotalError() {
    return p_error;
}

double PID::SteerValue(bool debug) {
    double steer_value = TotalError();

    if (debug) {
        std::cout << "total error: " << steer_value << " i_error: " << i_error << std::endl;
    }
    if (steer_value > 1.) {
        steer_value = 1.;
    } else if (steer_value < -1.) {
        steer_value = -1.;
    }

    return steer_value;
}
