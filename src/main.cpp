#include <uWS/uWS.h>
#include "json.hpp"
#include "PID.h"

// for convenience
using json = nlohmann::json;

static bool justSwitchedToManual = true; // Just helpful printing when switching to manual
static bool FIRST_TIME = true;

static const bool USE_TWIDDLE = false;
static const int TWIDDLE_ITERATIONS = 5000;
static const int MIN_TWIDDLE_IT_TO_START_ERR = 100;
static const double DESIRED_ERROR = 0.5;  // Could be used to stop PID at the right time

static const int WEBSOCKECT_OK_DISCONNECT_CODE = 1000;

static const std::string RESET_SIMULATOR_WS_MESSAGE = "42[\"reset\", {}]";

static const std::string MANUAL_WS_MESSAGE = "42[\"manual\",{}]";
static const double MAX_ITERATION_ERROR = 15.;

// FIXME: This probably is NOT thread safe...just trying out the Twiddle code...
static double p_arr[3] = {.170359, .00000000190791, .1};
static double dp[3] = {.01, .000001, .01 }; // Anything above .1 is already known manually to be quite bad, so save some time up to .5


//Running Twiddle with
//static double p_arr[3] = {.35, .0009, .3};
//static double dp[3] = {.2, .001, .2 };

//Current best P: 0.35, 0.0009, 0.5,  and best error: 15.0018
//Current best P: 0.13, 0.00190791, 0.5,  and best error: 6.72912
// (1+ hour later)....
//Current best P: 0.170359, 0.00190791, 0.5,  and best error: 6.04466

// Ok, let's get rid of this 0.5, that's def too high, car is wild and dizzying!

// take 2, BLEcH
//static double p_arr[3] = {.070359, .00000000190791, .1};
//static double dp[3] = {.05, .000001, .05 };
// Current best P: 0.159359, 1.90791e-09, 0.341541,  and best error: 17.4547

static int curTwiddleIt;
static int curParamIdx;
static double bestError;
static int bestNumIterations;
static double curError;
static double best_p[3] = {0., 0., 0.};
static bool tryingAgain = false;

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
    auto found_null = s.find("null");
    auto b1 = s.find_first_of("[");
    auto b2 = s.find_last_of("]");
    if (found_null != std::string::npos) {
        return "";
    } else if (b1 != std::string::npos && b2 != std::string::npos) {
        return s.substr(b1, b2 - b1 + 1);
    }
    return "";
}

void sendMessage(uWS::WebSocket<uWS::SERVER> ws, std::string msg) {
    ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
}

void moveToNextParamToTweak() {
    if (curParamIdx == 2) {
        curParamIdx = 0;
        // In Sebastian's, there is a tolerance, instead here, I will manually quit when needed...
        std::cout << "=====================================================\n";
        std::cout << "| Did a full run of param tweaks, how's it looking? |\n";
        std::cout << "=====================================================\n";
    }// else if (curParamIdx == 0) {
     //   std::cout << "Skipping i-error for now, want to focus on p/d\n";
     //   curParamIdx = 2;
   // } else {
        curParamIdx++;
    //}

    //std::cout << "Moving on to tweak: " << curParamIdx << std::endl;
    p_arr[curParamIdx] += dp[curParamIdx]; // This is done at beginning of for loop in the Sebastian's Python code
}

void recordBest(const double endError) {
    bestError = endError;

    best_p[0] = p_arr[0];
    best_p[1] = p_arr[1];
    best_p[2] = p_arr[2];

    dp[curParamIdx] *= 1.1; // This is also done whenever we beat the bestError, whether on try-again or first-time
}

void resetPidAndRunTwiddleAgain(PID pid) {
    std::cout << "Current best P: ";
    for (const double curParam : best_p) {
        std::cout << curParam << ", ";
    }

    std::cout << " and best error: " << bestError << "\n\n\n";


    pid.Init(p_arr[0], p_arr[1], p_arr[2]);

    curTwiddleIt = 0;
    curError = 0.;
}

void twiddleAfterAFullRun(const double endError) {// TODO: is it necessary to divide by n?  Maybe we could leave out??
    if (tryingAgain) {
        tryingAgain = false;
        if (endError < bestError) {
            std::cout << "Beat best error on try again!, *= 1.1 to dp" << std::endl;
            recordBest(endError);
        } else {
            std::cout << "Did NOT beat best error on try again!, *= 0.9 to dp" << std::endl;
            p_arr[curParamIdx] += dp[curParamIdx];
            dp[curParamIdx] *= 0.9;
        }

        std::cout << "moving to next paramIdx after tryingAgain" << std::endl;

        moveToNextParamToTweak();

    } else {
        if (endError < bestError) {
            std::cout << "Beat best error on first try with these params, moving along to next paramIdx!"
                           << std::endl;
            recordBest(endError);

            moveToNextParamToTweak();

        } else {
            tryingAgain = true;
            std::cout
                    << "Didn't beat the best error, so we're going to subtract twice dp and try again"
                    << std::endl;
            p_arr[curParamIdx] -= 2 * dp[curParamIdx]; // we subtract twice because we added before
        }
    }
}

int main() {
    uWS::Hub h;

    PID pid;
    const double p = .17; //0.070359; //.06;
    const double i = 0; //.000190791;
    const double d = .07; //.8; //19;

    if (!USE_TWIDDLE) {
        std::cout << "No Twiddle, just using supplied params!" << std::endl;
        pid.Init(p, i, d);
    } else {
        std::cout << "Using Twiddle! with params: max_iterations" << TWIDDLE_ITERATIONS
                  << " and min to record: " << MIN_TWIDDLE_IT_TO_START_ERR
                  << " and max allowed length: " <<  MAX_ITERATION_ERROR << std::endl;
        curTwiddleIt = 0;
        curParamIdx = 0;
        curError = 0.;
        bestError = INFINITY;
        pid.Init(p_arr[0], p_arr[1], p_arr[2]);
    }

    h.onMessage([&pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
        // "42" at the start of the message means there's a websocket message event.
        // The 4 signifies a websocket message
        // The 2 signifies a websocket event

        if (length && length > 2 && data[0] == '4' && data[1] == '2') {
            auto s = hasData(std::string(data).substr(0, length));
            if (s != "") {
                auto j = json::parse(s);
                std::string event = j[0].get<std::string>();
                if (event == "telemetry") {
                    //if (!justSwitchedToManual) {
                    //    std::cout << "Taking back control from manual!!" << std::endl;
                    //    justSwitchedToManual = true;
                    //}

                    // j[1] is the data JSON object
                    const double cte = std::stod(j[1]["cte"].get<std::string>());
                    const double speed = std::stod(j[1]["speed"].get<std::string>());
                    const double angle = std::stod(j[1]["steering_angle"].get<std::string>());

                    pid.UpdateError(cte);

                    double steer_value = pid.SteerValue(!USE_TWIDDLE);
                    double throttle_value = .1;
                    if (cte > 2.) { // Slow down when CTE gets too high
                        throttle_value = .01;
                    }

                    // DEBUG
                    if (!USE_TWIDDLE) {
                        std::cout << "CTE: " << cte
                                  << ", Steering Value: " << steer_value << std::endl;
                    }

                    json msgJson;
                    msgJson["steering_angle"] = steer_value;
                    msgJson["throttle"] = throttle_value;

                    auto msg = "42[\"steer\"," + msgJson.dump() + "]";

                    if (USE_TWIDDLE) {
                        double endError = 0.;
                        curTwiddleIt++;

                        if (curTwiddleIt > MIN_TWIDDLE_IT_TO_START_ERR) { // Why does Sebastian do this?
                            if (curTwiddleIt == MIN_TWIDDLE_IT_TO_START_ERR + 1) {
                                std::cout << "Reached min number of iterations to start recording CTE^2" << std::endl;
                            }
                            curError += cte * cte; // TODO: Is is necessary to square cte?

                            endError = curError / (curTwiddleIt - MIN_TWIDDLE_IT_TO_START_ERR);

                            if (speed < 0.02) {
                                std::cout << "Probably stuck!  Resetting!" << std::endl;
                                endError += MAX_ITERATION_ERROR;
                            }
                        }

                        if (endError > MAX_ITERATION_ERROR || curTwiddleIt > TWIDDLE_ITERATIONS) {

                            if (endError > MAX_ITERATION_ERROR) {
                                // early cut-off check for clearly wrong error values.  Having ran Twiddle for 10+minutes, best error
                                // found was 0.0457...so use this to avoid whack-o params
                                // so then we can fail quickly and try again
                                std::cout << "!!!These params are clearly terrible, restarting twiddle with new params!!!" << std::endl;

                                if (curTwiddleIt > bestNumIterations + 1) {
                                    std::cout << "Reached new best num its before error-ing out!  New best; " << curTwiddleIt << std::endl;
                                    bestNumIterations = curTwiddleIt;
                                    recordBest(endError); // Yah, so the error might go up, but that's OK, because we want to run longer!
                                }
                            } else {
                                std::cout << "Reached max twiddle iterations!!! found error: " << endError << std::endl;
                            }

                            twiddleAfterAFullRun(endError);

                            resetPidAndRunTwiddleAgain(pid);
                            std::cout << "==============Sending simulator reset message===========\n\n\n\n"
                                      << std::endl;
                            msg = RESET_SIMULATOR_WS_MESSAGE;
                        }
                    }

                    //std::cout << msg << std::endl;
                    sendMessage(ws, msg);
                } else {
                    std::cout << "What events do we receive that is non-telemetry? " << event << std::endl;
                }
            } else {
                //if (justSwitchedToManual) {
                //    justSwitchedToManual = false;
                //    std::cout << "Switched to manual mode!!" << std::endl;
                //}

                sendMessage(ws, MANUAL_WS_MESSAGE);
            }
        }
    });

    // We don't need this since we're not using HTTP but if it's removed the program
    // doesn't compile :-(
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
        const std::string s = "<h1>Hello world!</h1>";
        if (req.getUrl().valueLength == 1) {
            res->end(s.data(), s.length());
        } else {
            // i guess this should be done more gracefully?
            res->end(nullptr, 0);
        }
    });

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
        if (FIRST_TIME) {
            std::cout << "Connected for first time!!!  Restarting simulator!" << std::endl;
            FIRST_TIME = false;
            auto msg = RESET_SIMULATOR_WS_MESSAGE;
            sendMessage(ws, msg);
        } else {
            std::cout << "Reconnected to simulator, success!" << std::endl;
        }
    });

    h.onError([](void *user) {
        // Code copied from: https://github.com/uNetworking/uWebSockets/blob/master/tests/main.cpp
        switch ((long) user) {
            case 1:
                std::cout << "Client emitted error on invalid URI" << std::endl;
                break;
            case 2:
                std::cout << "Client emitted error on resolve failure" << std::endl;
                break;
            case 3:
                std::cout << "Client emitted error on connection timeout (non-SSL)" << std::endl;
                break;
            case 5:
                std::cout << "Client emitted error on connection timeout (SSL)" << std::endl;
                break;
            case 6:
                std::cout << "Client emitted error on HTTP response without upgrade (non-SSL)" << std::endl;
                break;
            case 7:
                std::cout << "Client emitted error on HTTP response without upgrade (SSL)" << std::endl;
                break;
            case 10:
                std::cout << "Client emitted error on poll error" << std::endl;
                break;
            case 11:
                static int protocolErrorCount = 0;
                protocolErrorCount++;
                std::cout << "Client emitted error on invalid protocol" << std::endl;
                if (protocolErrorCount > 1) {
                    std::cout << "FAILURE:  " << protocolErrorCount << " errors emitted for one connection!" << std::endl;
                    //exit(-1);
                }
                break;
            default:
                std::cout << "FAILURE: " << user << " should not emit error!" << std::endl;
                //exit(-1);
        }
    });

    h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
        if (code == WEBSOCKECT_OK_DISCONNECT_CODE) {
            std::cout << "Disconnected normally" << std::endl;
        } else {
            std::cout << "Unexpected Disconnect with code: " << code << std::endl;
        }

        std::cout << "WARN: Not closing WS because we get bad access exception! (Which one cannot catch in C++!?)"  << std::endl;
        // StackOverflow https://stackoverflow.com/q/19304157 suggested that error code 1006 means to check onError
        // But, but, but, adding onError here doesn't get called at all, everythign seems alright
        // (other than this ws.close() exc_bad_access)
        //ws.close(code, message, length); // Besides, if we're getting disconnection method, then the connection is already closed??
    });

    int port = 4567;
    if (h.listen(port)) {
        std::cout << "Listening to port " << port << std::endl;
    } else {
        std::cerr << "Failed to listen to port" << std::endl;
        return -1;
    }
    h.run();
}
