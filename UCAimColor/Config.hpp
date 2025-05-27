#include <Windows.h>
namespace cfg
{
	static int trigger_fov;
	inline int color_mode = 0;
	inline bool advancednigger = false;
	inline bool useIstrigFilter = true;

	// Movement prediction settings
	inline bool use_prediction = true;           // Enable/disable movement prediction
	inline float prediction_factor = 1.2f;       // How far ahead to predict (higher = more aggressive prediction)
	inline float prediction_weight = 0.7f;       // How much to weigh prediction vs. actual position (0.0-1.0)
	inline int prediction_timeout = 500;         // How long to continue predicting after target loss (milliseconds)
	inline float prediction_acceleration = 1.1f; // Acceleration factor for prediction (1.0 = no acceleration)

	// Advanced movement pattern detection
	inline bool use_movement_patterns = true;    // Enable pattern-based prediction improvements
	inline float strafe_compensation = 1.5f;     // Compensation factor for strafing (side-to-side movement)

	// Continue tracking even if target temporarily lost
	inline bool continue_tracking = true;        // Keep tracking based on prediction when target lost

	// Adaptive smoothing based on target movement speed
	inline bool adaptive_smooth = true;          // Enable adaptive smoothing based on target speed
	inline float speed_threshold_low = 0.05f;    // Speed threshold for slow movement (pixels/ms)
	inline float speed_threshold_high = 0.15f;   // Speed threshold for fast movement (pixels/ms)
	inline float smooth_factor_slow = 1.2f;      // Smoothing factor for slow targets (higher = more smooth)
	inline float smooth_factor_medium = 1.0f;    // Smoothing factor for medium-speed targets
	inline float smooth_factor_fast = 0.7f;      // Smoothing factor for fast targets (lower = more responsive)

	inline bool aimbot_ativo{ true };
	inline int aimkey = { VK_MENU };
	inline int head_offset_x{ 0 };               // Horizontal offset for aiming at head (pixels)
	inline int head_offset_y{ -15 };             // Vertical offset for aiming at head (pixels)
	inline int aimbot_fov{ 35 };
	inline float aimbot_smooth{ 1.5 };
	inline int speed{ 4 };
	inline int sleep{ 0 };
	const int fov_limit_x = 100; // Adjust as needed
	const int fov_limit_y = 100; // Adjust as needed
	inline bool recoil_ativo{ false };
	inline int recoil_length{ 0 };
	inline int recoil_sleep{ 1 };
	inline int time_to_start{ 155 };
	inline int recoil_key = { VK_MENU };
	inline float recoil_speed{ 0.055 };
	inline double recoil_offset{ 0 };
	inline bool aimassist_ativo{ false };
	inline int assist_aimkey = { VK_MENU };
	inline int assist_head_offset_x{ 2 };
	inline int assist_head_offset_y{ 3 };
	inline int aimassist_fov{ 1 };
	inline float aimassist_smooth{ 1.5 };
	inline int assist_speed{ 1 };
	inline int assist_sleep{ 0 };
	inline bool flicker_ativo{ false };
	inline int flicker_key{ VK_XBUTTON2 };
	inline int flicker_fov{ 100 };
	inline int flicker_delay_between_shots{ 1 };
	inline float flicker_distance = 2.5f;
	inline bool silent_ativo{ false };
	inline int silent_key{ VK_XBUTTON2 };
	inline int silent_head_offset_x{ 2 };
	inline int silent_head_offset_y{ 3 };
	inline int silent_fov{ 100 };
	inline int silent_delay_between_shots{ 1 };
	inline float distance = 2.5f;
	inline bool triggerbot_ativo{ false };
	inline int triggerbot_key{ VK_XBUTTON2 };
	inline int triggerbot_delay{ 1 };
	inline int triggerbot_fovX{ 3 };
	inline int triggerbot_fovY{ 3 };
	//cores
	inline int menorRGB[3] = { 70, 0, 120 };
	inline int maiorRGB[3] = { 255, 190, 255 };
	inline int menorHSV[3] = { 270, 38, 40 };
	inline int maiorHSV[3] = { 310, 100, 100 };
}