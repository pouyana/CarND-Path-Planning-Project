#include <uWS/uWS.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "spline.h"
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "helpers.h"
#include "json.hpp"

// for convenience
using nlohmann::json;
using std::string;
using std::vector;

/**
 * Change the speed the vehicle according to your own 
 * and the target vehicle.
 * 
 **/
double changeSpeed(double v, double target_v)
{
  return v * 0.90 + target_v * 0.10;
}

/**
 * Cars behind are double close
 * 
 * @param double distance
 *
 * @return double
 */
double handleCarBehind(double distance)
{
  if (distance < 0)
  {
    distance = distance * -2;
  }
  return distance;
}

/**
 * Calculates the speed for the given car
 * 
 * 
 * @param double vx
 * @param double vy
 * 
 * 
 * @return double
 */
double calculateSpeed(double vx, double vy)
{
  return sqrt(vx * vx + vy * vy);
}

/**
 * Calculates the distance to the given car
 * 
 * @param double target_car_s
 * @param double car_s  
 * 
 * @return double
 */
double getCarDistance(double target_car_s,
                      double car_s)
{
  return target_car_s - car_s;
}

double getCarNewS(double time_step, double target_car_s, double target_car_speed, vector<double> previous_path_x)
{
  return target_car_s + ((double)previous_path_x.size() * time_step * target_car_speed);
}

/**
 * Calculate the car distance to the given lane
 * 
 * @param double car_d
 * @param int lane
 * 
 * @return double
 */
double getCarDistanceToLane(double car_d, int lane)
{
  return abs(car_d - (2 + 4 * lane));
}

/**
 * Changes the lane. This function does not check
 * the possibility.
 * 
 * @param double car_d
 * @param double lane
 * @param double left_car_distance
 * @param double right_car_distance
 * @param double maxRange
 * 
 * @return int
 */
int changeLane(
    double car_d,
    int lane,
    double left_car_distance,
    double right_car_distance,
    double maxRange)
{
  if ((35 < left_car_distance && left_car_distance < maxRange) || (35 < right_car_distance && right_car_distance < maxRange))
  {
    if (right_car_distance == maxRange)
    {
      if (lane != 0)
      {
        lane -= 1;
      }
    }
    else if (left_car_distance == maxRange)
    {
      if (lane != 2)
      {
        lane += 1;
      }
    }
    else if (left_car_distance > right_car_distance)
    {
      if (lane != 0)
      {
        lane -= 1;
      }
    }
    else
    {
      if (lane != 2)
      {
        lane += 1;
      }
    }
  }
  return lane;
}

/**
 *  Checks if the given car is in the same line as ours
 * 
 * @param int lane
 * @param double target_car_d
 * 
 * @return bool
 */
bool carInLane(int lane, double target_car_d)
{
  return target_car_d < (2 + 4 * lane + 2) && target_car_d > (2 + 4 * lane - 2);
}

/**
 * Checks if the given car is the left lane of us
 * 
 * @param int lane
 * @param double target_car_d
 * 
 * @return bool
 */
bool carInLeftLine(int lane, double target_car_d)
{
  return target_car_d < (2 + 4 * (lane - 1) + 2) && target_car_d > (2 + 4 * (lane - 1) - 2);
}

/**
 * Checks if the given car is to close to our car
 * 
 * @param double car_s 
 * @param int lane
 * @param double target_car_s
 * @param double target_car_d
 * 
 * @return bool
 */
bool carToClose(double car_s, int lane, double target_car_s, double target_car_d)
{
  if (carInLane(lane, target_car_d) == true)
  {
    if ((target_car_s > car_s) && (target_car_s - car_s < 30))
    {
      return true;
    }
  }
  return false;
}

/**
 * Returns the distance to the car on the left lane
 * 
 * @param double target_car_d
 * @param int lane
 * @param double distance
 * @param double maxRange
 */
double getLeftLaneCarDistance(double target_car_d, int lane, double distance, double maxRange)
{
  if (carInLeftLine(lane, target_car_d) == true)
  {
    if (lane > 0 & distance < maxRange)
    {
      return distance;
    }
  }
  return maxRange;
}

/**
 * Returns the distance to the right lane car
 * 
 * @param double target_car_d
 * @param int lane 
 * @param double distance
 * @param double maxRange
 * 
 * @return double
 */
double getRightLaneCarDistance(double target_car_d, int lane, double distance, double maxRange)
{
  if (carInLane(lane, target_car_d) == false && carInLeftLine(lane, target_car_d) == false)
  {
    if (lane < 2 && distance < maxRange)
    {
      return distance;
    }
  }
  return maxRange;
}

/**
 * Calculate the target speed of the given car
 * 
 * @param double target_car_speed
 * @param double speed_const
 * 
 * @return double
 */
double calculateTargetSpeed(double target_car_speed, double speed_const)
{
  return target_car_speed * 2.24 - speed_const;
}

int main()
{
  uWS::Hub h;

  // Load up map values for waypoint's x,y,s and d normalized normal vectors
  vector<double> map_waypoints_x;
  vector<double> map_waypoints_y;
  vector<double> map_waypoints_s;
  vector<double> map_waypoints_dx;
  vector<double> map_waypoints_dy;

  // Waypoint map to read from
  string map_file_ = "../data/highway_map.csv";
  // The max s value before wrapping around the track back to 0
  double max_s = 6945.554;

  std::ifstream in_map_(map_file_.c_str(), std::ifstream::in);

  string line;
  while (getline(in_map_, line))
  {
    std::istringstream iss(line);
    double x;
    double y;
    float s;
    float d_x;
    float d_y;
    iss >> x;
    iss >> y;
    iss >> s;
    iss >> d_x;
    iss >> d_y;
    map_waypoints_x.push_back(x);
    map_waypoints_y.push_back(y);
    map_waypoints_s.push_back(s);
    map_waypoints_dx.push_back(d_x);
    map_waypoints_dy.push_back(d_y);
  }

  int lane = 1;
  double ref_vel = 49.5;

  h.onMessage([&map_waypoints_x, &map_waypoints_y, &map_waypoints_s,
               &map_waypoints_dx, &map_waypoints_dy, &lane, &ref_vel](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
                                                                      uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {

      auto s = hasData(data);

      if (s != "")
      {
        auto j = json::parse(s);

        string event = j[0].get<string>();

        if (event == "telemetry")
        {
          // j[1] is the data JSON object

          // Main car's localization Data
          double car_x = j[1]["x"];
          double car_y = j[1]["y"];
          double car_s = j[1]["s"];
          double car_d = j[1]["d"];
          double car_yaw = j[1]["yaw"];
          double car_speed = j[1]["speed"];

          // Previous path data given to the Planner
          auto previous_path_x = j[1]["previous_path_x"];
          auto previous_path_y = j[1]["previous_path_y"];
          // Previous path's end s and d values
          double end_path_s = j[1]["end_path_s"];
          double end_path_d = j[1]["end_path_d"];

          // Sensor Fusion Data, a list of all other cars on the same side
          //   of the road.
          auto sensor_fusion = j[1]["sensor_fusion"];

          int prev_size = previous_path_x.size();

          if (prev_size > 0)
          {
            car_s = end_path_s;
          }

          double time_step = 0.02;
          bool is_to_close = false;
          double target_speed = false;
          double max_range = 1000.0;
          double speed_const = 10.0;
          double right_car_distance = max_range;
          double left_car_distance = max_range;
          double target_car_speed = 0.0;

          for (int i = 0; i < sensor_fusion.size(); i++)
          {
            double target_car_d = sensor_fusion[i][6];
            double vx = sensor_fusion[i][3];
            double vy = sensor_fusion[i][4];
            double target_car_speed = calculateSpeed(vx, vy);
            double target_car_s = sensor_fusion[i][5];
            target_car_s = getCarNewS(time_step, target_car_s, target_car_speed, previous_path_x);
            double distance = getCarDistance(target_car_s, car_s);
            if (is_to_close == false)
            {
              is_to_close = carToClose(car_s, lane, target_car_s, target_car_d);
              target_speed = calculateTargetSpeed(target_car_speed, speed_const);
            }
            distance = handleCarBehind(distance);
            left_car_distance = getLeftLaneCarDistance(target_car_d, lane, distance, left_car_distance);
            right_car_distance = getRightLaneCarDistance(target_car_d, lane, distance, right_car_distance);
          }
          if (is_to_close)
          {
            lane = changeLane(car_d, lane, left_car_distance, right_car_distance, max_range);
            ref_vel = changeSpeed(car_speed, target_speed);
            is_to_close = false;
          }
          else
          {
            ref_vel = changeSpeed(car_speed, 49.5);
          }

          json msgJson;

          vector<double> ptsx;
          vector<double> ptsy;
          double ref_x = car_x;
          double ref_y = car_y;
          double ref_yaw = deg2rad(car_yaw);

          if (prev_size < 2)
          {
            double prev_car_x = car_x - cos(car_yaw);
            double prev_car_y = car_y - sin(car_yaw);
            ptsx.push_back(prev_car_x);
            ptsx.push_back(car_x);
            ptsy.push_back(prev_car_y);
            ptsy.push_back(car_y);
          }
          else
          {
            ref_x = previous_path_x[prev_size - 1];
            ref_y = previous_path_y[prev_size - 1];
            double ref_x_prev = previous_path_x[prev_size - 2];
            double ref_y_prev = previous_path_y[prev_size - 2];
            ref_yaw = atan2(ref_y - ref_y_prev, ref_x - ref_x_prev);
            ptsx.push_back(ref_x_prev);
            ptsx.push_back(ref_x);
            ptsy.push_back(ref_y_prev);
            ptsy.push_back(ref_y);
          }

          vector<double> next_wp0 = getXY(car_s + 30, (2 + 4 * lane), map_waypoints_s, map_waypoints_x, map_waypoints_y);
          vector<double> next_wp1 = getXY(car_s + 60, (2 + 4 * lane), map_waypoints_s, map_waypoints_x, map_waypoints_y);
          vector<double> next_wp2 = getXY(car_s + 90, (2 + 4 * lane), map_waypoints_s, map_waypoints_x, map_waypoints_y);

          ptsx.push_back(next_wp0[0]);
          ptsx.push_back(next_wp1[0]);
          ptsx.push_back(next_wp2[0]);

          ptsy.push_back(next_wp0[1]);
          ptsy.push_back(next_wp1[1]);
          ptsy.push_back(next_wp2[1]);

          // convert the map coordinates to car coordinates
          for (int i = 0; i < ptsx.size(); i += 1)
          {
            double shift_x = ptsx[i] - ref_x;
            double shift_y = ptsy[i] - ref_y;

            ptsx[i] = (shift_x * cos(0 - ref_yaw) - shift_y * sin(0 - ref_yaw));
            ptsy[i] = (shift_x * sin(0 - ref_yaw) + shift_y * cos(0 - ref_yaw));
          }

          tk::spline s;

          s.set_points(ptsx, ptsy);

          vector<double> next_x_vals;
          vector<double> next_y_vals;

          for (int i = 0; i < previous_path_x.size(); i += 1)
          {
            next_x_vals.push_back(previous_path_x[i]);
            next_y_vals.push_back(previous_path_y[i]);
          }

          double target_x = 30.0;
          double target_y = s(target_x);
          double target_dist = sqrt((target_x) * (target_x) + (target_y) * (target_y));
          double x_add_on = 0;

          for (int i = 1; i <= 50 - previous_path_x.size(); i += 1)
          {
            double N = (target_dist / (0.02 * ref_vel / 2.24));
            double x_point = x_add_on + (target_x) / N;
            double y_point = s(x_point);
            x_add_on = x_point;

            double x_ref = x_point;
            double y_ref = y_point;

            x_point = (x_ref * cos(ref_yaw) - y_ref * sin(ref_yaw));
            y_point = (x_ref * sin(ref_yaw) + y_ref * cos(ref_yaw));

            x_point += ref_x;
            y_point += ref_y;

            next_x_vals.push_back(x_point);
            next_y_vals.push_back(y_point);
          }

          msgJson["next_x"] = next_x_vals;
          msgJson["next_y"] = next_y_vals;

          auto msg = "42[\"control\"," + msgJson.dump() + "]";

          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        } // end "telemetry" if
      }
      else
      {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    } // end websocket if
  }); // end h.onMessage

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
                         char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port))
  {
    std::cout << "Listening to port " << port << std::endl;
  }
  else
  {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }

  h.run();
}