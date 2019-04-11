# CarND-Path-Planning-Project

Self-Driving Car Engineer Nano degree program.

## Goals

- Navigate in the virtual highway and drive inside the lanes
- Driving under speed limit of 50MPH But always close to that
- Pass the slower cars when possible with lane changes
- Don't collide with other cars
- Dont create accelerations over 10m/s^2 and jerks higher than 10 m/s^3 
- Make a complete loop of the highway 6946m

## Addressing the goals

### Navigate in the virtual highway and drive inside the lanes

As described in the help video by Udacity, the two vectors ptsy and ptsx that contains the points are created. (Line 375-276) At the start there are not enough points created for the car to follow. To do so, the car current position and previous position calculated with the help of heading, are added to the list of pints. (Line 379-389)

If there are enough points available in the list of previous points they will be pushed in our point vectors. (Line 394-401) With the help of helper function getXY() points in next 30, 60, 90 meters will be calculated. These will be added to the list of points in every axis. (Line 404-406) 

Still the xy coordinate used here are given in map coordinates, they should be transformed to car coordinates. Spline function needs these points in car coordinates. (Line 417-424) Using the spline function from `spline.h`, the new car coordinates are calculated and converted back to map coordinates. These are added to the list of points for the car to follow as trajectory.(Line 440-462)

### Pass the slower cars when possible with lane changes

If the car a head is slower and there is a line empty on the left / right the car tries to change lane. It is checked
if the distances in the other lanes are more than 35m to avoid collisions with the cars on the other lanes.

### Driving under speed limit of 50MPH But always close to that

The default speed is always is lower than 50MPH mainly 49.5. This is changed when the target a head has smaller target speed. This code assumes that all cars abide by this low so the calculate target speed is always less than 50MPH.

### Dont create accelerations over 10m/s^2 and jerks higher than 10 m/s^3

To make the car ride as smooth as possible without high accelerations and jerks, the car target speed is calculated. Then it it is added/subtracted gradually with the cars own speed. The function used here is a simple weighted average.
It can be found at lines: 22-25.

### Don't collide with other cars

This is done by checking for the cars in the same lane as car, if they have lower speed than ours, we will check the cars on the left/right lane, if it is possible to change lane, the car will change lane. If not possible the car will try to lower its speed to the target car speed.

### Make a complete loop of the highway 6946m

This works and the longest time I tested the car travelled more than 10 miles.

## Reflection

The help video and the discussions about the `spline` helped me a lot to create an acceptable trajectory with smooth lane changes, smooth transition between hight and low speed (small jerks).