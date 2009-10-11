#include <stdlib.h>
#include <assert.h>
#include "GlideSolvers/MacCready.hpp"
#include <math.h>
#include <stdio.h>

int n_samples = 0;


extern long count_mc;
extern int count_distance;

void distance_counts() {
  printf("#     distance queries %d\n",count_distance/n_samples); 
  printf("#     mc calcs %d\n",count_mc/n_samples); 
}

#include "Tasks/TaskManager.h"

#include "BaseTask/ConvexHull/GrahamScan.hpp"

double small_rand() {
  return rand()*0.005/RAND_MAX;
}

////////////////////////////////////////////////

void test_mc()
{
  MacCready mc;

  AIRCRAFT_STATE ac;
  GLIDE_STATE gs;
  GLIDE_RESULT gr;

  mc.set_mc(1.0);

  ac.WindSpeed = 5.0;
  ac.WindDirection = 0;

  gs.Distance = 100;
  gs.Bearing = 0;
  gs.MinHeight = 2.0;

  ac.Altitude = 10;

  printf("AC alt %g\n", ac.Altitude);
  gr = mc.solve(ac,gs);
  gr.print(std::cout);

  ac.Altitude = 1;
  printf("AC alt %g\n", ac.Altitude);
  gr = mc.solve(ac,gs);
  gr.print(std::cout);

  ac.Altitude = 3;
  printf("AC alt %g\n", ac.Altitude);
  gr = mc.solve(ac,gs);
  gr.print(std::cout);

}

////////////////////////////////////////////////
/*
#include "ConvexHull/GrahamScan.hpp"
#include "ConvexHull/PolygonInterior.hpp"
#include <math.h>

std::vector<SearchPoint> sampled_points;


void test_polygon() 
{
  for (double t=0; t<1.0; t+= 0.1) {
    SearchPoint sp;
    double ang = t*2.0*3.1415926;
    sp.Location.Longitude = cos(ang);
    sp.Location.Latitude = sin(ang);
    sp.actual = true;
    sp.saved_rank = 0;
    sampled_points.push_back(sp);
  }

  GrahamScan gs(sampled_points);
  sampled_points = gs.prune_interior();

  GEOPOINT location;
  location.Longitude = 0.4;
  location.Latitude = 1.0;
  if (PolygonInterior(location, sampled_points)) {
    printf("inside\n");
  } else {
    printf("outside\n");
  }
  location.Longitude = 0.4;
  location.Latitude = 0.0;

  if (PolygonInterior(location, sampled_points)) {
    printf("inside\n");
  } else {
    printf("outside\n");
  }

}
*/
////////////////////////////////////////////////

int main() {

  test_mc();


  TaskManager test_task;

  AIRCRAFT_STATE state, state_last;
  state.Location.Longitude=8;
  state.Location.Latitude=11;  
  state.Altitude = 3.0;
  state.Time = 0.0;
  state.WindSpeed = 5.0;
  state.WindDirection = 0;

  test_task.setActiveTaskPoint(0);

#define  num_wp 5
  GEOPOINT w[num_wp];
  w[0].Longitude = -0.25; 
  w[0].Latitude = -1.25; 
  w[1].Longitude = -0.5; 
  w[1].Latitude = 10.5; 
  w[2].Longitude = 10.5; 
  w[2].Latitude = 10.5; 
  w[3].Longitude = 7.5; 
  w[3].Latitude = 4.5; 
  w[4].Longitude = 9; 
  w[4].Latitude = 1; 

  state_last.Location = w[0];

  for (int i=0; i<num_wp-1-1; i++) {
    for (double t=0; t<1.0; t+= 0.002) {
      state.Location.Latitude = w[i].Latitude*(1.0-t)+w[i+1].Latitude*t+small_rand();
      state.Location.Longitude = w[i].Longitude*(1.0-t)+w[i+1].Longitude*t+small_rand();

      double dx = state.Location.Longitude-state_last.Location.Longitude;
      double dy = state.Location.Latitude-state_last.Location.Latitude;
      double d = sqrt(dx*dx+dy*dy);
      double V = 7.0;
      test_task.update_sample(state, state_last);
      test_task.report(state);
      n_samples++;
      state_last = state;
      if (state.Location.Longitude>10.5) { 
        distance_counts();
        exit(0); 
      }
      state.Time += d/V;
    }
    printf("[enter to continue]\n");
    char c = getchar();
    (void)c;
  }
  distance_counts();

//  test_task.remove(2);
//  test_task.scan_distance(location);
  return 0;
}
