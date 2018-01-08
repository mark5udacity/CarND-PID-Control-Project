#include <uWS/uWS.h>
#include <iostream>
#include "json.hpp"
#include "PID.h"
#include <math.h>

// for convenience
using json = nlohmann::json;

static bool justSwitchedToManual = true; // Just helpful printing when switching to manual

static const int WEBSOCKECT_OK_DISCONNECT_CODE = 1000;

static const std::string RESET_SIMULATOR_WS_MESSAGE = "42[\"reset\", {}]";

static const std::string MANUAL_WS_MESSAGE = "42[\"manual\",{}]";

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

int main() {
    uWS::Hub h;

    PID pid;
    const double p = 0.05;
    const double d = 0.6;
    const double i = 0.0001;
    pid.Init(p, i, d);

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
                    if (!justSwitchedToManual) {
                        std::cout << "Taking back control from manual!!" << std::endl;
                        justSwitchedToManual = true;
                    }

                    // j[1] is the data JSON object
                    const double cte = std::stod(j[1]["cte"].get<std::string>());
                    const double speed = std::stod(j[1]["speed"].get<std::string>());
                    const double angle = std::stod(j[1]["steering_angle"].get<std::string>());

                    pid.UpdateError(cte);

                    double steer_value = pid.SteerValue();

                    // DEBUG
                    std::cout << "CTE: " << cte
                              << ", Steering Value: " << steer_value << std::endl;

                    json msgJson;
                    msgJson["steering_angle"] = steer_value;
                    msgJson["throttle"] = 0.3;

                    auto msg = "42[\"steer\"," + msgJson.dump() + "]";
                    //std::cout << msg << std::endl;
                    sendMessage(ws, msg);
                } else {
                    std::cout << "What events do we receive that is non-telemetry? " << event << std::endl;
                }
            } else {
                if (justSwitchedToManual) {
                    justSwitchedToManual = false;
                    std::cout << "Switched to manual mode!!" << std::endl;
                }

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
        std::cout << "Connected!!!" << std::endl;
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
