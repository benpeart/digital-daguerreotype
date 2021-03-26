
//
// An application to capture an image from an Intel RealSense camera and convert it to TSP Art
// that can be used to generate gcode and be sent to a laser etcher or a gcode enabled Etch-A-Sketch
//

// cheap hack to determine if building for raspberry pi
#ifdef __arm__
#define RASPBERRYPI
#endif

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "rgb2tsp.h"
#include "texture.h"
#include "gcode.h"
#include <thread>
#include <vector>
#include <future>
#ifdef RASPBERRYPI
#include <unistd.h>
#endif

using namespace rs2;
using namespace cv;

// Intel RealSense D415 Camera specs:
// RGB frame resolution: 1920 × 1080
// Depth output resolution: Up to 1280 × 720

// The raspberry pi screen resolution is 800 x 480
const int screenWidth = 800;
const int screenHeight = 480;

// The input width/height should be less than the screen and camera resolution
const int inputWidthPixels = 720;
const int inputHeightPixels = 480;

// Large Etch-A-Sketch screen size is 160 mm x 110 mm (600 x 420 pixels - roughly 3:2 ratio)
// Printable area on 8.5" / 11" paper is 250 mm x 180 mm
// The output width/height ratio should be the same as the input ratio to prevent warping the image
const int outputWidthMM = 250;
const int outputHeightMM = 167;


// constants for UI control placement and state
const int window_gap = 5;
const int slider_window_width = 80;
const int button_window_width = 80;
enum class program_modes { interactive, computing, ready, printing };

// enable threads to be canceled
std::atomic_bool cancellation_token = ATOMIC_VAR_INIT(true);
std::atomic_bool thread_running = ATOMIC_VAR_INIT(false);

// local helper functions
float get_depth_scale(device dev);
rs2_stream find_stream_to_align(const std::vector<stream_profile>& streams);
bool profile_changed(const std::vector<stream_profile>& current, const std::vector<stream_profile>& prev);
void remove_background(rs2::video_frame& other_frame, const rs2::depth_frame& depth_frame, float depth_scale, float clipping_dist);
void render_slider(rect location, float& clipping_dist);
void render_buttons(rect location, rs2::pipeline& pipe, program_modes& mode);
void *print_gcode(void *tsp);

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**) try
{
	// Track the state of the program - what step are we currently in?
	program_modes program_mode = program_modes::interactive;
	bool process_image = false;
	bool process_tsp = false;
	bool output_gcode = false;

	// The TSP we generate for the captured image
	Path tsp;

	// the OpenCV image we will draw
	Mat display_image, print_image;
	GLuint display_texture;

	// create OpenGL texture to use for caching image to be displayed
	glGenTextures(1, &display_texture);

	// Create a pipeline to easily configure and start the camera
	pipeline pipe;

	// Calling pipeline's start() without any additional parameters will start the first device
	// with its default streams.
	// The start function returns the pipeline profile which the pipeline used to start the device
	pipeline_profile profile = pipe.start();

	// Each depth camera might have different units for depth pixels, so we get it here
	// Using the pipeline's profile, we can retrieve the device that the pipeline uses
	float depth_scale = get_depth_scale(profile.get_device());

	// Pipeline could choose a device that does not have a color stream
	// If there is no color stream, choose to align depth to another stream
	rs2_stream align_to = find_stream_to_align(profile.get_streams());

	// Create a align object.
	// align allows us to perform alignment of depth frames to others frames
	// The "align_to" is the stream type to which we plan to align depth frames.
	rs2::align align(align_to);

	// Define a variable for controlling the distance to clip (integer divide by 10 to get actual flot value)
	float depth_clipping_distance = 1.f;

	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;
	const auto window_name = "Digital Daguerreotype";
#ifdef RASPBERRYPI
	// create the window full screen
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, window_name, monitor, NULL);
#else
	// create the window the size it will be on the Raspberry Pi
	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, window_name, NULL, NULL);
#endif
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
#ifdef RASPBERRYPI
	io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;           // Enable touchscreen
#endif
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		int x, y, w, h;

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// clear the open gl frame buffer
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Take dimensions of the window for rendering purposes
		glfwGetWindowSize(window, &w, &h);

		// attempt to load an image from disk (handy when trying to make a video)
		// if the image is loaded, jump directly into the processing state
		Mat disk_image = imread("digital-daguerreotype.png");
		if (!disk_image.empty()) {
			display_image = disk_image;
			process_image = true;
			program_mode = program_modes::computing;
			rename("digital-daguerreotype.png", "digital-daguerreotype.png.bak");
		}

		switch (program_mode)
		{
		case program_modes::interactive:
		{
			// cancel any background tasks as we're going to be capuring a new image
			cancellation_token = true;

			// we block the application until a frameset is available
			frameset frameset = pipe.wait_for_frames();

			// Since align is aligning depth to some other stream, we need to make sure that the stream was not changed
			// after the call to wait_for_frames();
			if (profile_changed(pipe.get_active_profile().get_streams(), profile.get_streams()))
			{
				// If the profile was changed, update the align object, and also get the new device's depth scale
				profile = pipe.get_active_profile();
				align_to = find_stream_to_align(profile.get_streams());
				align = rs2::align(align_to);
				depth_scale = get_depth_scale(profile.get_device());
			}

			// Get processed aligned frame
			auto processed = align.process(frameset);

			// Trying to get both video and aligned depth frames
			video_frame other_frame = processed.first(align_to);
			depth_frame aligned_depth_frame = processed.get_depth_frame();

			// If one of them is unavailable, continue iteration
			if (!aligned_depth_frame || !other_frame)
			{
				continue;
			}

			// Passing both frames to remove_background so it will "strip" the background
			remove_background(other_frame, aligned_depth_frame, depth_scale, depth_clipping_distance);

			// Convert the RealSense frame to an OpenCV matrix
			display_image = frame_to_mat(other_frame);

			// we will need to process this image before printing
			process_image = true;

			// mirror the image to make it easier to center yourself
			flip(display_image, display_image, 1);

			// render the flipped foreground only image and cache it in an OpenGL texture
			x = (w - other_frame.get_width()) / 2;
			y = (h - other_frame.get_height()) / 2;
			display_texture = mat_to_gl_texture(display_image, display_texture);
			render_gl_texture(display_texture, { (float)x, (float)y, (float)other_frame.get_width(), (float)other_frame.get_height() });

			// if the screen is wide enough, display the depth map
			if (w >= 1024)
			{
				// render the depth frame, as a picture-in-picture
				x = (w - inputWidthPixels) / 2;
				y = (h - inputHeightPixels) / 2;
				rect pip_stream{ 0, 0, (float)inputWidthPixels / 2, (float)inputHeightPixels / 2 };
				pip_stream = pip_stream.adjust_ratio({ static_cast<float>(aligned_depth_frame.get_width()),static_cast<float>(aligned_depth_frame.get_height()) });
				pip_stream.x = (float)x + inputWidthPixels + window_gap;
				pip_stream.y = (float)y;

				// Render depth (as picture in picture)
				Mat depth_image;
				rs2::colorizer c;
				flip(frame_to_mat(c.process(aligned_depth_frame)), depth_image, 1);
				display_texture = mat_to_gl_texture(depth_image, display_texture);
				render_gl_texture(display_texture, pip_stream);
			}

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// draw the clipping rectangle centered on the image
			x = (w - inputWidthPixels) / 2;
			y = (h - inputHeightPixels) / 2;
			auto draw = ImGui::GetBackgroundDrawList();
			draw->AddRect(ImVec2((float)x, (float)y), ImVec2((float)x + inputWidthPixels, (float)y + inputHeightPixels), ImColor(0, 255, 0));

			// Using ImGui library to provide a slide controller to select the depth clipping distance
			render_slider({ window_gap, window_gap, slider_window_width, (float)h - window_gap * 2 }, depth_clipping_distance);

			// Using ImGui library to provide print/confirm/cancel buttons
			render_buttons({ (float)w - window_gap - button_window_width, window_gap, button_window_width, (float)h - window_gap * 2 }, pipe, program_mode);

			// Rendering
			ImGui::Render();
			break;
		}

		case program_modes::computing:
		case program_modes::ready:
		{
			// if we have a new image to process
			if (process_image)
			{
				// flip the image back so text is readable
				flip(display_image, display_image, 1);

				// Crop the image
				x = (display_image.cols - inputWidthPixels) / 2;
				y = (display_image.rows - inputHeightPixels) / 2;
				Rect box(Point(x, y), Size(inputWidthPixels, inputHeightPixels));
				Mat crop(display_image, box);

				// save the image we need to process to generate the TSP path
				print_image = crop;

				// Cache the cropped OpenGL texture so we don't have to created it every loop
				// do this _before_ we start doing the image processing on the matrix in mat_to_tsp below
				display_texture = mat_to_gl_texture(print_image, display_texture);
				process_image = false;

				// start converting cv:Mat to a vector of TSP points
				cancellation_token = false;
#ifdef _DEBUG
				imshow("print image", print_image);
#endif
				// if the tsp is empty for some reason, try capturing a new image
				tsp = mat_to_tsp(print_image, cancellation_token);
				if (tsp.empty())
					program_mode = program_modes::interactive;
				else
					process_tsp = true;
			}

			// if we have a tsp to process
			if (process_tsp)
			{
				// draw the TSP path as a series of lines to simulate what we'll be outputting
				display_image = Mat(Size(inputWidthPixels, inputHeightPixels), CV_8UC3, Scalar(255, 255, 255));
				for (Path::iterator i = tsp.begin(); i != tsp.end(); ++i)
				{
					auto j = i + 1;
					if (j != tsp.end())
						cv::line(display_image, *i, *j, cv::Scalar(0, 0, 0), 1);
				}

				// convert the output to a texture we can use to paint
				display_texture = mat_to_gl_texture(display_image, display_texture);

				// we're ready to draw the image
				program_mode = program_modes::ready;
				process_tsp = false;
				output_gcode = true;
			}

			// render the cached OpenGL texture
			x = (w - inputWidthPixels) / 2;
			y = (h - inputHeightPixels) / 2;
			render_gl_texture(display_texture, { (float)x, (float)y, (float)inputWidthPixels, (float)inputHeightPixels });

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Using ImGui library to provide print/confirm/cancel buttons
			render_buttons({ (float)w - window_gap - button_window_width, window_gap, button_window_width, (float)h - window_gap * 2 }, pipe, program_mode);

			// Rendering
			ImGui::Render();
			break;
		}

		case program_modes::printing:
		{
			// output gcode for TSP
			if (output_gcode)
			{
#ifdef RASPBERRYPI
				// create a thread to output the gcode
				pthread_t gcode_thread;
				int rc;

				// initialize thread flow control now so it's correct before the thread even starts
				cancellation_token = false;
				thread_running = true;
				rc = pthread_create(&gcode_thread, NULL, print_gcode, (void *)&tsp);
				if (rc)
				{
					fprintf(stderr, "Error %d creating gcode print thread.\n", rc);
					thread_running = false;
					program_mode = program_modes::interactive;
				}
#endif
				output_gcode = false;
			}

			// if the print thread has completed
			if (!thread_running)
			{
				// we're ready to start with a new  picture
				program_mode = program_modes::interactive;
			}

			// render the cached OpenGL texture
			x = (w - inputWidthPixels) / 2;
			y = (h - inputHeightPixels) / 2;
			render_gl_texture(display_texture, { (float)x, (float)y, (float)inputWidthPixels, (float)inputHeightPixels });

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Using ImGui library to provide print/confirm/cancel buttons
			render_buttons({ (float)w - window_gap - button_window_width, window_gap, button_window_width, (float)h - window_gap * 2 }, pipe, program_mode);

			// Rendering
			ImGui::Render();
			break;
		}
		}

		// If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
		// you may need to backup/reset/restore other state, e.g. for current shader using the commented lines below.
		//GLint last_program;
		//glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
		//glUseProgram(0);
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		//glUseProgram(last_program);

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteTextures(1, &display_texture);
	glfwDestroyWindow(window);
	glfwTerminate();

	pipe.stop();
	return 0;
}
catch (const rs2::error& e)
{
	std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	return EXIT_FAILURE;
}
catch (const std::exception& e)
{
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}

float get_depth_scale(device dev)
{
	// Go over the device's sensors
	for (sensor& sensor : dev.query_sensors())
	{
		// Check if the sensor if a depth sensor
		if (depth_sensor dpt = sensor.as<depth_sensor>())
		{
			return dpt.get_depth_scale();
		}
	}
	throw std::runtime_error("Device does not have a depth sensor");
}

rs2_stream find_stream_to_align(const std::vector<stream_profile>& streams)
{
	// Given a vector of streams, we try to find a depth stream and another stream to align depth with.
	// We prioritize color streams to make the view look better.
	// If color is not available, we take another stream that (other than depth)
	rs2_stream align_to = RS2_STREAM_ANY;
	bool depth_stream_found = false;
	bool color_stream_found = false;
	for (stream_profile sp : streams)
	{
		rs2_stream profile_stream = sp.stream_type();
		if (profile_stream != RS2_STREAM_DEPTH)
		{
			if (!color_stream_found)         //Prefer color
				align_to = profile_stream;

			if (profile_stream == RS2_STREAM_COLOR)
			{
				color_stream_found = true;
			}
		}
		else
		{
			depth_stream_found = true;
		}
	}

	if (!depth_stream_found)
		throw std::runtime_error("No Depth stream available");

	if (align_to == RS2_STREAM_ANY)
		throw std::runtime_error("No stream found to align with Depth");

	return align_to;
}

bool profile_changed(const std::vector<stream_profile>& current, const std::vector<stream_profile>& prev)
{
	for (auto&& sp : prev)
	{
		// If previous profile is in current (maybe just added another)
		auto itr = std::find_if(std::begin(current), std::end(current), [&sp](const stream_profile& current_sp) { return sp.unique_id() == current_sp.unique_id(); });

		// If it previous stream wasn't found in current
		if (itr == std::end(current))
		{
			return true;
		}
	}
	return false;
}

void remove_background(rs2::video_frame& other_frame, const rs2::depth_frame& depth_frame, float depth_scale, float clipping_dist)
{
	const uint16_t* p_depth_frame = reinterpret_cast<const uint16_t*>(depth_frame.get_data());
	uint8_t* p_other_frame = reinterpret_cast<uint8_t*>(const_cast<void*>(other_frame.get_data()));

	int width = other_frame.get_width();
	int height = other_frame.get_height();
	int other_bpp = other_frame.get_bytes_per_pixel();

	// Using OpenMP to try to parallelise the loop
#pragma omp parallel for schedule(dynamic) 
	for (int y = 0; y < height; y++)
	{
		auto depth_pixel_index = y * width;
		for (int x = 0; x < width; x++, ++depth_pixel_index)
		{
			// Get the depth value of the current pixel
			auto pixels_distance = depth_scale * p_depth_frame[depth_pixel_index];

			// Check if the depth value is invalid (<=0) or greater than the threashold
			if (pixels_distance <= 0.f || pixels_distance > clipping_dist)
			{
				// Calculate the offset in other frame's buffer to current pixel
				auto offset = depth_pixel_index * other_bpp;

				// Set "background" pixel color to white
				std::memset(&p_other_frame[offset], 255, other_bpp);
			}
		}
	}
}

void render_slider(rect location, float& clipping_dist)
{
	// Some trickery to display the control nicely
	static const int flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove;
	const int pixels_to_buttom_of_stream_text = 5;

	ImGui::SetNextWindowPos({ location.x, location.y });
	ImGui::SetNextWindowSize({ location.w, location.h });

	// Render the vertical slider
	ImGui::Begin("slider", nullptr, flags);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImU32)ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImU32)ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, (ImU32)ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
	auto slider_size = ImVec2(location.w / 4, location.h - (pixels_to_buttom_of_stream_text * 2) - 20);
	ImGui::VSliderFloat("", slider_size, &clipping_dist, 0.0f, 6.0f, "", ImGuiSliderFlags_None);

	// remove the tool tips for now as they just obscure the image underneath
#ifdef TOOLTIP
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Depth Clipping Distance: %.3f", clipping_dist);
#endif
	ImGui::PopStyleColor(3);

	// Display bars next to slider
	float bars_dist = (slider_size.y / 6.0f);
	for (int i = 0; i <= 6; i++)
	{
		ImGui::SetCursorPos({ slider_size.x, i * bars_dist });
		std::string bar_text = "- " + std::to_string(6 - i) + "m";
		ImGui::Text("%s", bar_text.c_str());
	}
	ImGui::End();
}

void render_buttons(rect location, rs2::pipeline& pipe, program_modes& program_mode)
{
	const float button_width = location.w - 2 * window_gap;
	const float button_height = location.h / 2 - 2 * window_gap;

	// Some trickery to display the control nicely
	static const int flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove;

	ImGui::SetNextWindowPos({ location.x, location.y });
	ImGui::SetNextWindowSize({ location.w, location.h });

	// Set options for the ImGui buttons
	ImGui::Begin("buttons", nullptr, flags);
	ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1, 1, 1, 1 });
	ImGui::PushStyleColor(ImGuiCol_Button, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);

	// display the UI controls based on the current program state
	switch (program_mode)
	{
	case program_modes::interactive:
		ImGui::SetCursorPos({ window_gap, window_gap });
		if (ImGui::Button("start", { button_width, button_height }))
			program_mode = program_modes::computing;

		// remove the tool tips for now as they just obscure the image underneath
#ifdef TOOLTIP
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Click 'start' to capture the current image");
#endif
		break;

	case program_modes::computing:
		ImGui::SetCursorPos({ window_gap, window_gap});
		if (ImGui::Button("cancel", { button_width, button_height }))
			program_mode = program_modes::interactive;
#ifdef TOOLTIP
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Click 'cancel' to choose a different image");
#endif
		break;

	case program_modes::ready:
		ImGui::SetCursorPos({ window_gap, window_gap });
		if (ImGui::Button("cancel", { button_width, button_height }))
			program_mode = program_modes::interactive;
#ifdef TOOLTIP
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Click 'cancel' to choose a different image");
#endif
		ImGui::SetCursorPos({ window_gap, location.h / 2 + window_gap });
		if (ImGui::Button("draw", { button_width, button_height }))
			program_mode = program_modes::printing;
#ifdef TOOLTIP
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Click 'draw' to begin drawing your picture");
#endif
		break;

	case program_modes::printing:
		ImGui::SetCursorPos({ window_gap, window_gap });
		if (ImGui::Button("cancel", { button_width, button_height })) 
			program_mode = program_modes::interactive;
#ifdef TOOLTIP
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Click 'cancel' to stop the current drawing and start over");
#endif
		break;
	}

	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();
	ImGui::End();
}

#ifdef RASPBERRYPI
void *print_gcode(void *arg)
{
	Path *tsp = (Path *)arg;
	const char *portname = "/dev/ttyUSB0";
	int fd;
	char buf[256], *p;
	int len;
	float x, y;

	// open the serial port to the CNC machine
	fd = gcode_open(portname);
	if (-1 == fd)
		goto ErrorExit;

	// initilizae grbl state
	if (gcode_write(fd, "$H\n"))	// run homing cycle
		goto ErrorExit;
	if (gcode_write(fd, "G00 G91 G21 Z-5 F3000\n"))	// lift the pen
		goto ErrorExit;
	if (gcode_write(fd, "G00 G91 G21 X10 Y-10 Z0 F3000\n"))	// move to 10mm x 10mm to avoid the limit switches while drawing
		goto ErrorExit;
	if (gcode_write(fd, "G92 X0 Y0 Z0\n"))	// Change the current coordinates without moving
		goto ErrorExit;
	if (gcode_write(fd, "G90\n"))	// use absolute coordinates from the program's origin
		goto ErrorExit;
	if (gcode_write(fd, "G21\n"))	// programming in mm
		goto ErrorExit;
	if (gcode_write(fd, "G1 F3000\n"))	// set a feed rate (determines move speed)
		goto ErrorExit;
//	if (gcode_write(fd, "$1=255\n"))	// tell motors to prevent moving when stationary (step idle delay)
//		goto ErrorExit;

	// move to the first point in the TSP with the pen up then lower the pen
	// I'm flipping the x and y axis to match my CNC machine orientation
	x = (float)(*tsp)[0].x * outputWidthMM / inputWidthPixels;
	y = (float)(*tsp)[0].y * outputHeightMM / inputHeightPixels;
	sprintf(buf, "G1 X%f Y%f Z0\n", y, x-outputWidthMM);
	if (gcode_write(fd, buf))
		goto ErrorExit;
	if (gcode_write(fd, "G1 Z5\n"))
		goto ErrorExit;

	// move from point to point in the TSP
	for (Path::iterator i = (*tsp).begin(); i != (*tsp).end(); ++i)
	{
		// output each point as the next position to move to (invert the Y coordinate)
		x = (float)(*i).x * outputWidthMM / inputWidthPixels;
		y = (float)(*i).y * outputHeightMM / inputHeightPixels;
		sprintf(buf, "G1 X%f Y%f Z5\n", y, x-outputWidthMM);
		if (gcode_write(fd, buf))
			goto ErrorExit;

		// if we've been asked to cancel, bail out early
		if (cancellation_token)
			goto ErrorExit;
	}

ErrorExit:
	// reset the CNC to a safe location
//	if (gcode_write(fd, "$1=254\n"))	// step idle delay, milliseconds
//		goto ErrorExit;
	gcode_write(fd, "G00 G90 G21 Z0 F3000\n");	// lift the pen
	gcode_write(fd, "G00 G90 G21 X10 Y-10 F3000\n");	// move to 10mm x 10mm to avoid the limit switches

	// give grbl enough time to complete these last commands before we
	// close the port as it will abort any command in progress
	sleep(2);
	gcode_close(fd);

	// exit the thread cleanly
	thread_running = false;
	pthread_exit(NULL);
	return NULL;
}
#endif
