#include "ofApp.h"

// using namespace ofxCv;
using namespace cv;

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetLogLevel(OF_LOG_VERBOSE);

	face_tracking_rectangle.set(glm::mediump_ivec2(CAM_WIDTH/4, CAM_HEIGHT/4), CAM_WIDTH - CAM_WIDTH/2, CAM_HEIGHT - CAM_HEIGHT/2);

	// if needed, set the logging to a file instead of stdout
	// ofLogToFile("paintball.log");

	// CAMERA
	// Load the JSON with the video settings from a configuration file.
    ofJson config = ofLoadJson("settings.json");
	// Create a grabber from the JSON
    video_grabber = ofxPS3EyeGrabber::fromJSON(config);

	ofSetVerticalSync(true);
	
	// FACE TRACKING
    face_tracker.setup();

	// DOTS
	circle_size = ofMap(16, MACHINE_X_MIN_POS, MACHINE_X_MAX_POS, 0, CAM_WIDTH);

	// save start time
	start_time = std::chrono::steady_clock::now();

	// init vars
	draw_dots = false;
	start_button_pressed = false;
	button_pressed_time = 0;
	current_command_index = 0;
	estimated_elapsed_time = "";
	real_elapsed_time = "";

	// connect to the arduino mega
	init_serial_device(cnc_device);

	dots_fbo.allocate(CAM_WIDTH, CAM_HEIGHT, GL_RGBA, 8);
	input_image.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
}

//--------------------------------------------------------------
void ofApp::update(){
	
	// update PS3 eye camera
    video_grabber->update();
	if (video_grabber->isFrameNew() && !draw_dots){

		// get the pixels from the cam and put them into the input_image
		ofPixels & grabber_pixels = video_grabber->getGrabber<ofxPS3EyeGrabber>()->getPixels();
		input_image.setFromPixels(grabber_pixels);
		input_image.setImageType(OF_IMAGE_GRAYSCALE);

		// track face
        face_tracker.update(ofxCv::toCv(input_image));

        // update the tracked face position
        tracked_face_position = face_tracker.getPosition();
	}

	// Save the time when the button is pressed
	if (start_button_pressed){

		button_pressed_time = (int) ofGetElapsedTimef();
		ofLogNotice() << "button pressed at: " << button_pressed_time << " seconds";
	
		int elapsed_seconds = (int) ofGetElapsedTimef();

		// wait some time before sending stuff to serial
		while (elapsed_seconds < button_pressed_time + SERIAL_INITIAL_DELAY_TIME){
			elapsed_seconds = (int) ofGetElapsedTimef();
		}

		ofLogNotice() << "sending home";
			
		ofxOscMessage osc_message;
		osc_message.setAddress("/home");
		osc_message.addIntArg(1);

		send_osc_bundle(osc_message, cnc_device, 1024);

		draw_dots = true;
	}

	start_button_pressed = false;
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(ofColor::white);

	// if the user hasn't pressed the space bar,
	// show the webcam live feed
	if (!draw_dots){
		ofxCv::threshold(input_image, 120);
		input_image.draw(0,0);

		ofPushStyle();
		ofNoFill();
		if (ofDist(tracked_face_position.x, tracked_face_position.y, ofGetWidth()/2, ofGetHeight()/2) > FACE_DISTANCE_THRESHOLD){
			ofDrawBitmapStringHighlight("Please put your face inside the rectangle", 50, 35);
			ofSetColor(ofColor::red);
		}
		else {
			ofDrawBitmapStringHighlight("Press the space bar to take a machine portrait!", 34, 35);
			ofSetColor(ofColor::green);
		}
		ofDrawRectangle(face_tracking_rectangle.x, face_tracking_rectangle.y, face_tracking_rectangle.width, face_tracking_rectangle.height);
		ofPopStyle();
	}
	else {
		output_image.draw(0, 0);
		//dots_fbo.draw(0, 0);
		ofPushStyle();
		// draw in green the current shot
		glm::mediump_ivec2 current_pos = sorted_dots.at(current_command_index);
		ofFill();
		ofSetColor(ofColor::green);
		ofDrawCircle(current_pos.x, current_pos.y, circle_size/2);
		ofPopStyle();

		ofDrawBitmapStringHighlight("Coherent line drawing", 10, 20);
		ofDrawBitmapStringHighlight("Number of dots: " + ofToString(sorted_dots.size()), 10, 35);
	}

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if (key == ' '){
		start_button_pressed = true;
		run_coherent_line_drawing(input_image, output_image, dots_fbo);
	}
}

//--------------------------------------------------------------
// SERIAL
//--------------------------------------------------------------
// initialise the communication settings with the arduino mega
// @args:	&cnc 		--> the required serial slip device (ofxIO::SLIPPacketSerialDevice)
//--------------------------------------------------------------
void ofApp::init_serial_device(ofxIO::SLIPPacketSerialDevice &cnc){

	std::vector<ofxIO::SerialDeviceInfo> devices_info = ofxIO::SerialDeviceUtils::listDevices();

	// log connected devices
    ofLogNotice("ofApp::setup") << "Connected devices: ";
    for (std::size_t i = 0; i < devices_info.size(); ++i){
		ofLogNotice("ofApp::setup") << "\t" << devices_info[i];
	}

	if (!devices_info.empty()){

        // Connect to the device
        bool success = cnc.setup(devices_info[0], BAUD_RATE);

        if (success){
            cnc.registerAllEvents(this);
            ofLogNotice("ofApp::setup") << "Successfully setup " << devices_info[0];
        }
        else ofLogError("ofApp::setup") << "Unable to setup " << devices_info[0];
    }
    else ofLogNotice("ofApp::setup") << "No devices connected.";
}

//--------------------------------------------------------------
void ofApp::send_current_command(int i){

    glm::mediump_ivec2 pos = sorted_dots.at(i);
	// map the position from pixels to mm
    glm::mediump_ivec2 mapped_pos(
		ofMap(pos.x, face_tracking_rectangle.x, face_tracking_rectangle.x + face_tracking_rectangle.width, MACHINE_X_MIN_POS, MACHINE_X_MAX_POS, true),
		ofMap(pos.y, face_tracking_rectangle.y, face_tracking_rectangle.y + face_tracking_rectangle.height, MACHINE_Y_MIN_POS, MACHINE_Y_MAX_POS, true)
	);

    ofxOscMessage osc_message;
	osc_message.setAddress("/stepper");
	osc_message.addIntArg((int) mapped_pos.x * X_DISTORTION_CORRECTION);
	osc_message.addIntArg(mapped_pos.y);

	ofLogNotice("send_current_command") << osc_message; 
    ofLogNotice("send_current_command") << current_command_index+1 << "/" << ofToString(sorted_dots.size());

    // check onSerialBuffer() to see what happens after we send a command
	send_osc_bundle(osc_message, cnc_device, 1024);
}

//--------------------------------------------------------------
// OPENCV
//--------------------------------------------------------------
// run the coherent line algorithm on a given image
// @args:	in			--> the input image
// @args:	out			--> the output image (b & w) obtained from CLD
// @args:	dots_fbo	--> an fbo on top of which we will draw the dots
//--------------------------------------------------------------
void ofApp::run_coherent_line_drawing(const ofImage &in, ofImage &out, ofFbo &dots_fbo){
	
	// Reset vector
	dots.clear();
	sorted_dots.clear();

	// Do the coherent line drawing magic
	ofxCv::CLD(input_image, output_image, HALFW, SMOOTH_PASSES, SIGMA1, SIGMA2, TAU, BLACK_LEVEL);
	ofxCv::invert(output_image);
	ofxCv::threshold(output_image, threshold);
	output_image.update();
	
	// create_debugging_quad(dots, dots_fbo); // draw a nice quad, useful for calibrating the machine

	// Draw the dots on their fbo
	ofLogNotice() << "circle size: " << circle_size;
	int sampling_size = circle_size;

	dots_fbo.begin();

	int max_dots = 300;
	int ending_point_x = face_tracking_rectangle.x + face_tracking_rectangle.width;
	int ending_point_y = face_tracking_rectangle.y + face_tracking_rectangle.height;

	// Sample the pixels from the coherent line image
	// and add dots if we find a white pixel
	for (int x = face_tracking_rectangle.x; x < ending_point_x; x+= sampling_size){
		for (int y = face_tracking_rectangle.y; y < ending_point_y; y+= sampling_size){

			if (dots.size() < max_dots){
				
				if (ofDist(x, y, ofGetWidth()/2, ofGetHeight()/2) < INTEREST_RADIUS){
					ofColor c = output_image.getColor(x, y);

					if (c.r == 255){
						ofSetColor(ofColor::orange);
						ofDrawCircle((int) x, (int) y, circle_size/2);
						// ofDrawRectangle(x, y, circle_size, circle_size);
						dots.push_back(glm::mediump_ivec2(x, y));
					}
				}
			}
			else {
				break;
			}
		}
	}

	dots_fbo.end();	

	// Optimize the path using nearest neighbour
	ofLogNotice("run_coherent_line_drawing()") << "optimizing path";
	int overall_path_length = solve_nn(dots, sorted_dots);
	ofLogNotice("run_coherent_line_drawing") << "overall length of the portrait: " << overall_path_length / 1000 << "m";
	int estimated_seconds = (overall_path_length * STEPS_PER_MM * SECONDS_BETWEEN_STEPS * MAGIC_NUMBER);
	int estimated_minutes = estimated_seconds / 60;
	estimated_elapsed_time = ofToString(estimated_minutes) + ":" + ofToString(estimated_seconds % 60);
	ofLogNotice("run_coherent_line_drawing") << "estimated time (m:s) --> " << estimated_elapsed_time;

	// for debug, save the points to a csv file
	ofFile sorted_dots_file("sorted_dots.csv", ofFile::WriteOnly);
	for (auto d : sorted_dots){
		sorted_dots_file << d.x << ',' << d.y << endl;
	}
	ofFile dots_file("dots.csv", ofFile::WriteOnly);
	for (auto d : dots){
		dots_file << d.x << ',' << d.y << endl;
	}

	// just add a final dot on the bottom left corner - the artist signature!
	dots.push_back(glm::mediump_ivec2(0, ending_point_y));

	ofLogNotice("run_coherent_line_drawing()") << "sorted dots size: " << sorted_dots.size();
	ofLogNotice("run_coherent_line_drawing()") << "completed";
}

//--------------------------------------------------------------
// draws a nice rectangle made of points
// @args:	&dots		--> the vector where it will push the dots
// @args:	dots_fbo	--> an fbo on top of which we will draw the dots
//--------------------------------------------------------------
void ofApp::create_debugging_quad(vector<glm::mediump_ivec2> & dots, ofFbo & dots_fbo){

	dots_fbo.begin();

	int quad_width = face_tracking_rectangle.width;
	int quad_resolution = 4;
	int sampling_size = quad_width / quad_resolution;

	// top X line
	for (int x = face_tracking_rectangle.x; x < face_tracking_rectangle.x + quad_width; x+=sampling_size){
		dots.push_back(glm::mediump_ivec2(x, face_tracking_rectangle.y));
		ofSetColor(ofColor::orange);
		ofDrawCircle(x, face_tracking_rectangle.y, circle_size);
	}
	// right Y line
	for (int y = face_tracking_rectangle.y + sampling_size; y < face_tracking_rectangle.y + quad_width; y+=sampling_size){
		dots.push_back(glm::mediump_ivec2(face_tracking_rectangle.x + quad_width, y));
		ofSetColor(ofColor::orange);
		ofDrawCircle(face_tracking_rectangle.x + quad_width, y, circle_size);
	}
	// bottom X line
	for (int x = face_tracking_rectangle.x + quad_width - sampling_size; x > face_tracking_rectangle.x; x-=sampling_size){
		dots.push_back(glm::mediump_ivec2(x, face_tracking_rectangle.y + quad_width));
		ofSetColor(ofColor::orange);
		ofDrawCircle(x, face_tracking_rectangle.y + quad_width, circle_size);
	}
	// left Y line
	for (int y = face_tracking_rectangle.y + quad_width; y >= face_tracking_rectangle.y; y-=sampling_size){
		dots.push_back(glm::mediump_ivec2(face_tracking_rectangle.x, y));
		ofSetColor(ofColor::orange);
		ofDrawCircle(face_tracking_rectangle.x, y, circle_size);
	}

	sorted_dots = dots;

	dots_fbo.end();
}

//--------------------------------------------------------------
// TSP
//--------------------------------------------------------------
// use nearest neighbours algorithm to find a good path for the cnc
// @args: 	in_points  --> the points used to compute the path optimization
// 	  		out_points --> a vector that will be filled with the sorted points
//--------------------------------------------------------------
int ofApp::solve_nn(const vector<glm::mediump_ivec2> & in_points, vector<glm::mediump_ivec2> & out_points){

    // 1. Start on an arbitrary vertex as current vertex
    int closest_p_index = 0;
	float nn_distance = 0.0f;

    // continue while there are still points to visit
    while (out_points.size() < in_points.size()-1){

        glm::mediump_ivec2 p = in_points.at(closest_p_index);

        // ofLogNotice() << " out_points: " << out_points.size() << ", in_points: " << in_points.size();

        float min_distance = FLT_MAX;

        for (int j = 0; j < in_points.size(); j++){

            glm::mediump_ivec2 other_p = in_points.at(j);

            // check if we already have visited the other p, if so, just skip it
			// NB using this technique, duplicate points will be removed.
            if(std::find(out_points.begin(), out_points.end(), other_p) == out_points.end()) {
                
                // 2. Find out the shortest edge connecting current vertex and an unvisited vertex V
                float current_distance = ofDist(p.x, p.y, other_p.x, other_p.y);
                if (current_distance < min_distance && current_distance > 0){

                    min_distance = current_distance;
                    // 3. make this point the next point
                    closest_p_index = j;
					// but don't add it to the list until we've checked all the points against the first!
                }
            }
        }
		// now we can add it to the list of added points:
		glm::mediump_ivec2 other_p = in_points.at(closest_p_index);
		out_points.push_back(other_p);
    }

	// compute distance of nn
    for (int i = 0; i < out_points.size()-1; i++){
        auto p = out_points.at(i);
        auto next_p = out_points.at(i+1);
        nn_distance += ofDist(p.x, p.y, next_p.x, next_p.y);
    }

	return nn_distance;
}

//--------------------------------------------------------------
// OSC
//--------------------------------------------------------------
void ofApp::append_message(ofxOscMessage &message, osc::OutboundPacketStream &p){

    p << osc::BeginMessage(message.getAddress().data());

    for (int i = 0; i < message.getNumArgs(); ++i){

        if ( message.getArgType(i) == OFXOSC_TYPE_INT32)
            p << message.getArgAsInt32(i);

        else if ( message.getArgType(i) == OFXOSC_TYPE_INT64 )
            p << (osc::int64)message.getArgAsInt64( i );

        else if ( message.getArgType(i) == OFXOSC_TYPE_FLOAT )
            p << message.getArgAsFloat(i);

        else if ( message.getArgType(i) == OFXOSC_TYPE_DOUBLE )
            p << message.getArgAsDouble( i );

        else if ( message.getArgType( i ) == OFXOSC_TYPE_STRING || message.getArgType( i ) == OFXOSC_TYPE_SYMBOL)
            p << message.getArgAsString( i ).data();

        else if ( message.getArgType( i ) == OFXOSC_TYPE_CHAR )
            p << message.getArgAsChar( i );

        else if ( message.getArgType( i ) == OFXOSC_TYPE_TRUE || message.getArgType( i ) == OFXOSC_TYPE_FALSE )
            p << message.getArgAsBool( i );

        else if ( message.getArgType( i ) == OFXOSC_TYPE_TRIGGER )
            p << message.getArgAsTrigger( i );

        else if ( message.getArgType( i ) == OFXOSC_TYPE_TIMETAG )
            p << (osc::int64)message.getArgAsTimetag( i );

        else if ( message.getArgType( i ) == OFXOSC_TYPE_BLOB ){
            ofBuffer buff = message.getArgAsBlob(i);
            osc::Blob b(buff.getData(), (unsigned long)buff.size());
            p << b;

        }
        else {
            ofLogError("ofxOscSender") << "append_message(): bad argument type " << message.getArgType(i);
            assert(false);
        }
    }

    p << osc::EndMessage;
}

//--------------------------------------------------------------
// TSP
//--------------------------------------------------------------
// send the message bundled inside using the osc protocol
// @args: 	m 		 	--> the points used to compute the path optimization
// 	  		device	 	--> a ofxSerial device
// 	  		buffer_size	--> the size of the serial buffer
//--------------------------------------------------------------
void ofApp::send_osc_bundle(ofxOscMessage &m, ofxIO::SLIPPacketSerialDevice &device, int buffer_size){
    // this codes come from ofxOscSender::sendMessage in ofxOscSender.cpp
    // static const int OUTPUT_BUFFER_SIZE = buffer_size;
    char buffer[buffer_size];

    // create the packet stream
    osc::OutboundPacketStream p(buffer, buffer_size);
    
    p << osc::BeginBundleImmediate; // start the bundle
    append_message(m, p);           // add the osc message to the bundle
    p << osc::EndBundle;            // end the bundle

    // send to device
    device.send(ofxIO::ByteBuffer(p.Data(), p.Size()));
}

//--------------------------------------------------------------
// SERIAL
//--------------------------------------------------------------
void ofApp::onSerialBuffer(const ofx::IO::SerialBufferEventArgs &args){
    
	std::string received_command = args.buffer().toString();
	ofLogNotice("onSerialBuffer") << "received message --> " << received_command;

	// remove \r , \n and \0 chars from received message
	received_command.erase(std::remove(received_command.begin(), received_command.end(), '\n'), received_command.end());
	received_command.erase(std::remove(received_command.begin(), received_command.end(), '\r'), received_command.end());
	received_command.erase(std::remove(received_command.begin(), received_command.end(), '\0'), received_command.end());

	if (received_command == "home"){
		
		ofLogNotice("onSerialBuffer") << "homing done, starting";

		// start the painting by sending the values over serial
		send_current_command(current_command_index);
	}
	else if (received_command.substr(0, 7) == "stepper"){
		// check if we need to send more messages
		if (current_command_index <= sorted_dots.size() - 2){

			// The arduino sends us back a string formatted like that: "stepperx:valuey:value"
			// so we recreate artificially a similar string and we check if it's equal to the arduino message
			std::string sent_message = "";
			glm::mediump_ivec2 current_pos = sorted_dots.at(current_command_index);

			// map back the position from mm to pixels
			glm::mediump_ivec2 mapped_pos(
				ofMap(current_pos.x, face_tracking_rectangle.x, face_tracking_rectangle.x + face_tracking_rectangle.width, MACHINE_X_MIN_POS, MACHINE_X_MAX_POS, true),
				ofMap(current_pos.y, face_tracking_rectangle.y, face_tracking_rectangle.y + face_tracking_rectangle.height, MACHINE_Y_MIN_POS, MACHINE_Y_MAX_POS, true)
			);

			int corrected_x = (int) mapped_pos.x * X_DISTORTION_CORRECTION;
			
			sent_message += "stepperx:" + ofToString(corrected_x) + "y:" + ofToString(mapped_pos.y);

			ofLogNotice("onSerialBuffer") << "sent:     " << sent_message;
			ofLogNotice("onSerialBuffer") << "received: " << received_command;

			// if arduino received the same message that we sent then send the next message
			if (received_command == sent_message){
				ofLogNotice("onSerialBuffer") << "all good";
				current_command_index++;
				send_current_command(current_command_index);
			}
			else {
				ofLogError("onSerialBuffer") << "we received a different message than the sent one";
			}
		}
		// END of the painting
		else {
			ofLogNotice("onSerialBuffer") << "sent all commands!";
			current_command_index = 0;

			// get elapsed time
			auto end_time = std::chrono::steady_clock::now();
			auto elapsed_micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
			
			int elapsed_seconds = (elapsed_micros / 1000000);
			int elapsed_minutes = elapsed_seconds / 60;
			real_elapsed_time = ofToString(elapsed_minutes) + ":" + ofToString(elapsed_seconds % 60);
			ofLogNotice() << "estimated time:    " << estimated_elapsed_time;
			ofLogNotice() << "real elapsed time: " << real_elapsed_time;

			std::exit(0);
		}
	}
}

//--------------------------------------------------------------
void ofApp::onSerialError(const ofx::IO::SerialBufferErrorEventArgs &args){
    // Errors and their corresponding buffer (if any) will show up here.
    ofLogError("onSerialError") << "Serial error : " << args.exception().displayText();
}

//--------------------------------------------------------------
// EXIT
//--------------------------------------------------------------
void ofApp::exit(){
	// save the current image
	ofSaveScreen("current_portrait.png");
	cnc_device.unregisterAllEvents(this);
}
