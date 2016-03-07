#include <cmath>
#include <cstdio>
#include <ctime>
#include <vector>

#include <SDL.h>

#include "bitmap.hh"
#include "gui.hh"
#include "star.hh"
#include "util.hh"
#include "vec2d.hh"

using namespace std;

// Screen size
#define WIDTH 640
#define HEIGHT 480

// Minimum time between clicks
#define CREATE_INTERVAL 1000

// Time step size
#define DT 0.04

// Gravitational constant
#define G 1

// Update all stars in the simulation
void updateStars();

// Draw a circle on a bitmap based on this star's position and radius
void drawStar(bitmap* bmp, star s);

// Add a "galaxy" of stars to the points list
void addRandomGalaxy(double center_x, double center_y);

// A list of stars being simulated
vector<star> stars;

// Offset of the current view
int x_offset = 0;
int y_offset = 0;

/**
 * Entry point for the program
 * \param argc  The number of command line arguments
 * \param argv  An array of command line arguments
 */
int main(int argc, char** argv) {
  // Seed the random number generator
  srand(time(NULL));
  
  // Create a GUI window
  gui ui("Galaxy Simulation", WIDTH, HEIGHT);
  
  // Start with the running flag set to true
  bool running = true;
  
  // Render everything using this bitmap
  bitmap bmp(WIDTH, HEIGHT);
  
  // Save the last time the mouse was clicked
  bool mouse_up = true;
  
  // Loop until we get a quit event
  while(running) {
    // Process events
    SDL_Event event;
    while(SDL_PollEvent(&event) == 1) {
      // If the event is a quit event, then leave the loop
      if(event.type == SDL_QUIT) running = false;
    }
    
    // Get the current mouse state
    int mouse_x, mouse_y;
    uint32_t mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
    
    // If the left mouse button is pressed, create a new random "galaxy"
    if(mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
      // Only create one if the mouse button has been released
      if(mouse_up) {
        addRandomGalaxy(mouse_x - x_offset, mouse_y - y_offset);
        
        // Don't create another one until the mouse button is released
        mouse_up = false;
      }
    } else {
      // The mouse button was released
      mouse_up = true;
    }
    
    // Get the keyboard state
    const uint8_t* keyboard = SDL_GetKeyboardState(NULL);
    
    // If the up key is pressed, shift up one pixel
    if(keyboard[SDL_SCANCODE_UP]) {
      y_offset++;
      bmp.shiftDown();  // Shift pixels so scrolling doesn't create trails
    }
    
    // If the down key is pressed, shift down one pixel
    if(keyboard[SDL_SCANCODE_DOWN]) {
      y_offset--;
      bmp.shiftUp();  // Shift pixels so scrolling doesn't create trails
    }
    
    // If the right key is pressed, shift right one pixel
    if(keyboard[SDL_SCANCODE_RIGHT]) {
      x_offset--;
      bmp.shiftLeft();  // Shift pixels so scrolling doesn't create trails
    }
    
    // If the left key is pressed, shift left one pixel
    if(keyboard[SDL_SCANCODE_LEFT]) {
      x_offset++;
      bmp.shiftRight(); // Shift pixels so scrolling doesn't create trails
    }
    
    // Remove stars that have NaN positions
    for(int i=0; i<stars.size(); i++) {
      // Remove this star if it is too far from zero or has NaN position
      if(stars[i].pos().x() != stars[i].pos().x() ||  // A NaN value does not equal itself
         stars[i].pos().y() != stars[i].pos().y()) {
        stars.erase(stars.begin()+i);
        i--;
        continue;
      }
    }
    
    // Compute forces on all stars and update
    updateStars();
    
    // Darken the bitmap instead of clearing it to leave trails
    bmp.darken(0.92);
    
    // Draw stars
    for(int i=0; i<stars.size(); i++) {
      drawStar(&bmp, stars[i]);
    }
    
    // Display the rendered frame
    ui.display(bmp);
  }
  
  return 0;
}

// Compute force on all stars and update their positions
void updateStars() {
  // Compute force on all stars
  for(int i=0; i<stars.size(); i++) {
    // Loop over all stars after this one
    for(int j=i+1; j<stars.size(); j++) {
      // Short names for important values
      double m1 = stars[i].mass();
      double m2 = stars[j].mass();
      double r1 = stars[i].radius();
      double r2 = stars[j].radius();
      
      // Compute a vector between the two points
      vec2d diff = stars[i].pos() - stars[j].pos();
      
      // Compute the distance between the two points
      double dist = diff.magnitude();
      
      // If the objects are too close, merge them
      if(dist < (r1 + r2) / 1.5) {
        // Replace the ith star with the merged one
        stars[i] = stars[i].merge(stars[j]);
        // Delete the jth star
        stars.erase(stars.begin() + j);
        j--;
        
      } else {
        // Normalize the difference vector to be a unit vector
        diff = diff.normalized();
      
        // Compute the force between these two stars
        vec2d force = -diff * G * m1 * m2 / pow(dist, 2);
      
        // Apply the force to both stars
        stars[i].addForce(force);
        stars[j].addForce(-force);
      }
    }
  }
  
  // Update positions and velocities given all accumulated forces
  for(int i=0; i<stars.size(); i++) {
    stars[i].update(DT);
  }
}

// Create a circle of stars moving in the same direction around the center of mass
void addRandomGalaxy(double center_x, double center_y) {
  // Random number of stars
  int count = rand() % 1000 + 1000;
  
  // Random radius
  double radius = drand(50, 200);
  
  // Create a vector for the center of the galaxy
  vec2d center = vec2d(center_x, center_y);
  
  // Clockwise or counter-clockwise?
  double direction = 1;
  if(rand() % 2 == 0) direction = -1;
  
  // Create `count` stars
  for(int i=0; i<count; i++) {
    // Generate a random angle
    double angle = drand(0, M_PI * 2);
    // Generate a random radius, biased toward the center
    double point_radius = drand(0, sqrt(radius)) * drand(0, sqrt(radius));
    // Compute X and Y coordinates
    double x = point_radius * sin(angle);
    double y = point_radius * cos(angle);
    
    // Create a vector to hold the position of this star (origin at center of the "galaxy")
    vec2d pos = vec2d(x, y);
    // Move the star in the appropriate direction around the center, with slightly-random velocity
    vec2d vel = vec2d(-cos(angle), sin(angle)) * sqrt(point_radius) * direction * drand(0.25, 1.25);
    
    // Create a new random color for the star
    rgb32 color = rgb32(rand() % 64 + 192, rand() % 64 + 192, 128);
    
    // Add the star with a mass dependent on distance from the center of the "galaxy"
    stars.push_back(star(10 / sqrt(pos.magnitude()), pos + center, vel, color));
  }
}

// Draw a circle at the given star's position
// Uses method from http://groups.csail.mit.edu/graphics/classes/6.837/F98/Lecture6/circle.html
void drawStar(bitmap* bmp, star s) {
  double center_x = s.pos().x();
  double center_y = s.pos().y();
  double radius = s.radius();
  
  // Loop over points in the upper-right quadrant of the circle
  for(double x = 0; x <= radius*1.1; x++) {
    for(double y = 0; y <= radius*1.1; y++) {
      // Is this point within the circle's radius?
      double dist = sqrt(pow(x, 2) + pow(y, 2));
      if(dist < radius) {
        // Set this point, along with the mirrored points in the other three quadrants
        bmp->set(center_x + x + x_offset, center_y + y + y_offset, s.color());
        bmp->set(center_x + x + x_offset, center_y - y + y_offset, s.color());
        bmp->set(center_x - x + x_offset, center_y - y + y_offset, s.color());
        bmp->set(center_x - x + x_offset, center_y + y + y_offset, s.color());
      }
    }
  }
}
