#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up,
	  left_arrow, right_arrow, down_arrow, up_arrow,
		lift_down, lift_up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *chassis = nullptr;
	Scene::Transform *lift = nullptr;
	glm::quat chassis_base_rotation;
	glm::vec3 lift_base_position;
	float chassis_angle = 0.0f;
	float chassis_speed = 0.0f;
	float lift_height = 0.0f;
	
	std::vector<Scene::Transform*> cubes;
	std::vector<bool> is_carried;
	// Scene::Transform *cube = nullptr;
	// bool is_carried = false;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
