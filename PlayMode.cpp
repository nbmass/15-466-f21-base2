#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("forklift.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("forklift.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Chassis") chassis = &transform;
		if (transform.name == "Lift") lift = &transform;
		if (transform.name == "Blue_Cube1" ||
		    transform.name == "Blue_Cube2" ||
		    transform.name == "Red_Cube1" ||
		    transform.name == "Red_Cube2" ||
		    transform.name == "Green_Cube1" ||
		    transform.name == "Green_Cube2") {
			cubes.push_back(&transform);
			is_carried.push_back(false);
		}
	}

	chassis_base_rotation = chassis->rotation;
	lift_base_position = lift->position;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left_arrow.downs += 1;
			left_arrow.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right_arrow.downs += 1;
			right_arrow.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up_arrow.downs += 1;
			up_arrow.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down_arrow.downs += 1;
			down_arrow.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_q) {
			lift_down.downs += 1;
			lift_down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			lift_up.downs += 1;
			lift_up.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left_arrow.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right_arrow.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up_arrow.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down_arrow.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_q) {
			lift_down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			lift_up.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// wobble += elapsed / 10.0f;
	// wobble -= std::floor(wobble);

	// hip->rotation = hip_base_rotation * glm::angleAxis(
	// 	glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 1.0f, 0.0f)
	// );

	glm::vec3 direction = glm::vec3(std::sin(glm::radians(chassis_angle)), -std::cos(glm::radians(chassis_angle)), 0.0f);
	glm::vec3 lift_position = chassis->position + 3.0f * direction;

	glm::vec3 X_prime(0.0f);
	float lift_z_prime = 0.0f;
	float theta_prime = 0.0f;

	if (left_arrow.pressed && !right_arrow.pressed) theta_prime = 50.0f * elapsed;
	if (!left_arrow.pressed && right_arrow.pressed) theta_prime = -50.0f * elapsed;
	if (!down_arrow.pressed && up_arrow.pressed) X_prime = 5.0f * elapsed * direction;
	if (down_arrow.pressed && !up_arrow.pressed) X_prime = -5.0f * elapsed * direction;
	if (lift_up.pressed && !lift_down.pressed) lift_z_prime = elapsed;
	if (!lift_up.pressed && lift_down.pressed) lift_z_prime = -elapsed;

	//move cubes and stuff
	for (uint32_t i = 0; i < cubes.size(); i++) {
		is_carried[i] = (std::abs(lift_position.x - cubes[i]->position.x) < 1.0f) &&
		                (std::abs(lift_position.y - cubes[i]->position.y) < 1.0f) &&
				            (std::abs(lift->position.z - (cubes[i]->position.z - 1.0f)) < 0.1f);
		// is_carried[i] = (glm::length(lift_position.x + lift_position.y - (cubes[i]->position.x + cubes[i]->position.y)) < 1.0f) &&
		// 		(std::abs(lift->position.z - (cubes[i]->position.z - 1)) < 0.1f);

		if (is_carried[i]) {
			//lift cube
			cubes[i]->position.z += lift_z_prime;
			if (cubes[i]->position.z < 1.0f) cubes[i]->position.z = 1.0f;
			if (cubes[i]->position.z > 3.8f) cubes[i]->position.z = 3.8f;
			//move cube
			if (lift->position.z > 0.0f) {
				cubes[i]->position += X_prime;
				//rotate cube
				cubes[i]->rotation *= glm::angleAxis(
					glm::radians(theta_prime),
					glm::vec3(0.0f, 0.0f, 1.0f)
				);
				//move cube when chassis rotates
				glm::vec3 cube_offset_old = cubes[i]->position - chassis->position;
				glm::vec3 cube_offset_new = cube_offset_old * glm::angleAxis(
					glm::radians(theta_prime),
					glm::vec3(0.0f, 0.0f, 1.0f)
				);
				cubes[i]->position += cube_offset_old - cube_offset_new;
			}
		}
	}

	//turn forklift
	chassis_angle += theta_prime;
	chassis->rotation = chassis_base_rotation * glm::angleAxis(
			glm::radians(chassis_angle),
			glm::vec3(0.0f, 0.0f, 1.0f)
	);
	//drive forklift
	chassis->position += X_prime;
	//control lift
	lift->position.z += lift_z_prime;
	if (lift->position.z < 0.0f) lift->position.z = 0.0f;
	if (lift->position.z > 2.8f) lift->position.z = 2.8f;

	// std::cout << "chassis: " << chassis->position.x << ", " << chassis->position.y << ", " << chassis->position.z
	//           << "   lift: " << lift->position.x << ", " << lift->position.y << ", " << lift->position.z
	// 					<< "   cube: " << cube->position.x << ", " << cube->position.y << ", " << cube->position.z
	// 					<< "   is_carried: " << is_carried
	// 					<< std::endl;
	// std::cout << "chassis: " << chassis->position.x << ", " << chassis->position.y << ", " << chassis->position.z
	//           << "   lift: " << lift_position.x << ", " << lift_position.y << ", " << lift_position.z
	// 					<< "   cube: " << cube->position.x << ", " << cube->position.y << ", " << cube->position.z << std::endl;
	//lift boxes


	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	// { //use DrawLines to overlay some text:
	// 	glDisable(GL_DEPTH_TEST);
	// 	float aspect = float(drawable_size.x) / float(drawable_size.y);
	// 	DrawLines lines(glm::mat4(
	// 		1.0f / aspect, 0.0f, 0.0f, 0.0f,
	// 		0.0f, 1.0f, 0.0f, 0.0f,
	// 		0.0f, 0.0f, 1.0f, 0.0f,
	// 		0.0f, 0.0f, 0.0f, 1.0f
	// 	));

	// 	constexpr float H = 0.09f;
	// 	lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
	// 		glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
	// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	// 	float ofs = 2.0f / drawable_size.y;
	// 	lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
	// 		glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
	// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	// 		glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	// }
}
