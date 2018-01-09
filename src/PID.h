#ifndef PID_H
#define PID_H

#include <math.h>

class PID {
private:
    /*
    * Errors
    */
    double p_error;
    double i_error;
    double d_error;

    /*
    * Coefficients
    */
    double Kp;
    double Ki;
    double Kd;

public:


    /*
    * Constructor
    */
    PID();

    /*
    * Destructor.
    */
    virtual ~PID();

    /*
    * Initialize PID.
    */
    void Init(double Kp, double Ki, double Kd);

    /*
    * Update the PID error variables given cross track error.
    */
    void UpdateError(double cte);

    /*
    * Return the total PID error.
    */
    double TotalError();

    /**
     * This may not belong here as it is specific to this particular project.
     *
     * @param debug print debug values if true
     * @return The steering error based on Total Error, adjusted for constraints.
     */
    double SteerValue(bool debug);

};

#endif /* PID_H */
