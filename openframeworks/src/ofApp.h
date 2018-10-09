#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxSerial.h"
#include "ofxPS3EyeGrabber.h"
#include "ofEvents.h"
#include "ofxOsc.h"
#include "ofxFaceTracker.h"
#include <chrono>

class ofApp : public ofBaseApp{
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;

	void keyPressed(int k) override;

	bool start_button_pressed;

	// PS3 EYE CAMERA
	const int CAM_WIDTH = 500;
	const int CAM_HEIGHT = 450;
	// the video grabber for the PS3Eye Cam
	// a shared_ptr avoids manual allocation of memory (new/delete)
	// when the reference count of the pointed object reaches 0 memory is freed
	std::shared_ptr<ofVideoGrabber> video_grabber;

	ofFbo light_fbo;

	// FACE TRACKING
	ofxFaceTracker face_tracker;
	glm::vec2 tracked_face_position;
	bool face_detected, update_servo;
	const int FACE_DISTANCE_THRESHOLD = 70;
	ofRectangle face_tracking_rectangle;

	// TSP (cnc path optimization)
	// Nearest Neighbour approach for finding best path
	int solve_nn(const vector<glm::mediump_ivec2> & in_points, vector<glm::mediump_ivec2> & out_points);

	// OPENCV
	void run_coherent_line_drawing(const ofImage &in, ofImage &out, ofFbo &dots_fbo);
	void create_debugging_quad(vector<glm::mediump_ivec2> & dots, ofFbo & dots_fbo);

	ofImage input_image, output_image;
	ofFbo dots_fbo;
	// coherent line drawing parameters
	const int HALFW = 6;
	const int SMOOTH_PASSES = 1;
	const float SIGMA1 = 4.50; // degree of coherence
	const float SIGMA2 = 0.95905;
	const float TAU = 0.98;
	const int BLACK_LEVEL = -8;
	const int threshold = 100;
	// vector<glm::vec2> dots, sorted_dots;
	vector<glm::mediump_ivec2> dots, sorted_dots;
	int circle_size;

	// SERIAL
	const int BAUD_RATE = 9600;
	void init_serial_device(ofxIO::SLIPPacketSerialDevice &device1);
	void send_current_command(int i); // used to send commands to the paintball machine
	int current_command_index; // keeps track of the current command that we're sending
	const int SERIAL_INITIAL_DELAY_TIME = 1; // seconds

	int button_pressed_time;
	bool draw_dots;
	
	// CNC
	// cnc machine movement boundaries
	const int MACHINE_X_MIN_POS = 5;
	const int MACHINE_X_MAX_POS = (CAM_WIDTH) + MACHINE_X_MIN_POS;
	const int MACHINE_Y_MIN_POS = 0;
	const int MACHINE_Y_MAX_POS = (CAM_HEIGHT) + MACHINE_Y_MIN_POS;
	const int INTEREST_RADIUS = 180;
	const int STEPS_PER_MM = 5;
	const double SECONDS_BETWEEN_STEPS = 0.0005;
	const float MAGIC_NUMBER = 3.442;
	const float X_DISTORTION_CORRECTION = 1.25;
	string estimated_elapsed_time, real_elapsed_time;

	ofxIO::SLIPPacketSerialDevice cnc_device;

	// ofxSerial events
	void onSerialBuffer(const ofxIO::SerialBufferEventArgs &args);
	void onSerialError(const ofxIO::SerialBufferErrorEventArgs &args);

	// STATS
	std::chrono::steady_clock::time_point start_time;

private:
	// OSC STUFF
	 // add our osc message to the osc bundle
	void append_message(ofxOscMessage &message, osc::OutboundPacketStream &p);
	 // send the osc bundle via serial
	void send_osc_bundle(ofxOscMessage &m, ofxIO::SLIPPacketSerialDevice &device, int buffer_size);
};
