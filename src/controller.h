#pragma once

// Initialize Bluepad32 and register controller callbacks
void controllerInit();

// Call every loop iteration — polls controller and drives servos
void controllerUpdate();
