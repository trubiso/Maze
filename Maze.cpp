#include <SDL.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>
#include <tuple>
#include <vector>

#define TICK_INTERVAL 0
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define PATHFIND_WA 10
#define PATHFIND_WB 10

std::string read_file(char const *path) noexcept {
	std::string content;
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		file.open(path);
		std::stringstream str;
		str << file.rdbuf();
		file.close();
		content = str.str();
	} catch (std::ifstream::failure const &err) {
		std::cout << "[ERROR] Couldn't read file " << path << "\n" << err.what() << std::endl;
	}

	return content;
}

void add_random_wall(std::vector<std::vector<bool>> &map, size_t const &start_x,
                     size_t const &start_y, size_t const &end_x, size_t const &end_y,
                     size_t const &w, size_t const &h,
                     std::uniform_int_distribution<size_t> &distrib, std::mt19937 &gen) {
	size_t random = distrib(gen);
	if (random == start_x + start_y * w)
		return add_random_wall(map, start_x, start_y, end_x, end_y, w, h, distrib, gen);
	if (random == end_x + end_y * w)
		return add_random_wall(map, start_x, start_y, end_x, end_y, w, h, distrib, gen);
	if (map[random / w][random % w])
		return add_random_wall(map, start_x, start_y, end_x, end_y, w, h, distrib, gen);
	map[random / w][random % w] = true;
}

int main(int argc, char **argv) {
	// init maze
	// from file:
	/*
	std::string maze = read_file("maze.txt");
	std::vector<std::vector<bool>> map{};
	size_t current_h = 0;
	size_t start_x = 0, start_y = 0, end_x = 0, end_y = 0;
	bool last_was_newline = true;
	for (char const &c : maze) {
	    if (last_was_newline) {
	        map.push_back(std::vector<bool>());
	        last_was_newline = false;
	    }
	    if (c == '\n') {
	        last_was_newline = true;
	        ++current_h;
	    } else {
	        map[current_h].push_back(c == '#');
	        if (c == 'S') {
	            start_x = map[current_h].size() - 1;
	            start_y = current_h;
	        } else if (c == 'E') {
	            end_x = map[current_h].size() - 1;
	            end_y = current_h;
	        }
	    }
	}
	size_t w = map[0].size(), h = map.size();
	*/
	// random:
	size_t w = 128, h = 64;
	size_t start_x = 1, start_y = 1, end_x = w - 2, end_y = h - 2;

	// The window we'll be rendering to
	SDL_Window *window = NULL;

	// The surface contained by the window
	SDL_Surface *screen_surface = NULL;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}
	// Create window
	window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                          TILE_WIDTH * w, TILE_HEIGHT * h, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	// Get window surface
	screen_surface = SDL_GetWindowSurface(window);

	// Fill the surface white
	SDL_FillRect(screen_surface, NULL, SDL_MapRGB(screen_surface->format, 0xFF, 0xFF, 0xFF));

	// Update the surface
	SDL_UpdateWindowSurface(window);

	// init random
init_random:
	std::random_device rd; // a seed source for the random number engine
	auto seed = rd();
	std::cout << "seed: " << seed << "\n";
	std::mt19937 gen(seed); // mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<size_t> distrib(0, w * h - 1);

	std::vector<std::vector<bool>> map(h, std::vector<bool>(w, false));
	map[0] = (map[h - 1] = std::vector<bool>(w, true));
	for (auto &col : map) {
		col[0] = true;
		col[w - 1] = true;
	}

	for (size_t i = 0; i < w * h / 4; ++i) {
		add_random_wall(map, start_x, start_y, end_x, end_y, w, h, distrib, gen);
	}

	std::vector<std::tuple<size_t, size_t>> directions{};
	// clang-format off
	directions.push_back({ 1,  0});
	directions.push_back({-1,  0});
	directions.push_back({ 0,  1});
	directions.push_back({ 0, -1});
	// clang-format on

	std::vector<size_t> visited(w * h, false);
	std::vector<size_t> gdistance(w * h, SIZE_MAX);
	std::vector<double> fdistance(w * h, DBL_MAX);
	std::vector<size_t> parent(w * h, SIZE_MAX);
	std::vector<size_t> queue{};
	bool path_found = false;
	gdistance[start_x + start_y * w] = 0;
	fdistance[start_x + start_y * w] = 0;
	queue.push_back(start_x + start_y * w);

	// Hack to get window to stay up
	SDL_Event e;
	bool quit = false;
	uint64_t next_time = SDL_GetTicks64() + TICK_INTERVAL;
	uint64_t begin = SDL_GetTicks64();
	while (quit == false) {
		SDL_FillRect(screen_surface, NULL, SDL_MapRGB(screen_surface->format, 0x00, 0x00, 0x00));
		SDL_Rect rect;
		if (!queue.empty()) {
			for (auto const &elem : queue) {
				rect.x = (int)(TILE_WIDTH * (elem % w));
				rect.y = (int)(TILE_HEIGHT * (elem / w));
				rect.w = TILE_WIDTH;
				rect.h = TILE_HEIGHT;
				SDL_FillRect(screen_surface, &rect,
				             SDL_MapRGB(screen_surface->format, 0x00, 0x00, 0xFF));
			}
		}
		for (size_t i = 0; i < h; ++i) {
			for (size_t j = 0; j < w; ++j) {
				rect.x = (int)(TILE_WIDTH * j);
				rect.y = (int)(TILE_HEIGHT * i);
				rect.w = TILE_WIDTH;
				rect.h = TILE_HEIGHT;
				if (map[i][j]) {
					SDL_FillRect(screen_surface, &rect,
					             SDL_MapRGB(screen_surface->format, 0xAA, 0xAA, 0xAA));
				}
				if (visited[i * w + j]) {
					SDL_FillRect(screen_surface, &rect,
					             SDL_MapRGB(screen_surface->format, 0x00, 0xAA, 0xEE));
				}
				if (start_x == j && start_y == i) {
					SDL_FillRect(screen_surface, &rect,
					             SDL_MapRGB(screen_surface->format, 0x00, 0xFF, 0x00));
				}
				if (end_x == j && end_y == i) {
					SDL_FillRect(screen_surface, &rect,
					             SDL_MapRGB(screen_surface->format, 0xFF, 0x00, 0x00));
				}
			}
		}
		if (path_found) {
			size_t end = parent[end_x + end_y * w];
			while (end != start_x + start_y * w) {
				rect.x = (int)(TILE_WIDTH * (end % w));
				rect.y = (int)(TILE_HEIGHT * (end / w));
				rect.w = TILE_WIDTH;
				rect.h = TILE_HEIGHT;
				SDL_FillRect(screen_surface, &rect,
				             SDL_MapRGB(screen_surface->format, 0xFF, 0xFF, 0xFF));
				end = parent[end];
			}
			SDL_UpdateWindowSurface(window);
			SDL_Delay(1000);
			goto init_random;
		}
		if (!path_found && queue.empty()) {
			std::cout << "No path from start to end\n";
			SDL_Delay(1000);
			goto init_random;
		}
		if (SDL_GetTicks64() >= begin + 5000 * (TICK_INTERVAL == 0 ? 1 : TICK_INTERVAL)) {
			std::cout << "Timed out!\n";
			SDL_Delay(1000);
			goto init_random;
		}
		SDL_UpdateWindowSurface(window);

		// a-star
		auto const heuristic = [](size_t ax, size_t bx, size_t ay, size_t by) {
			auto dx = bx > ax ? bx - ax : ax - bx;
			auto dy = by > ay ? by - ay : ay - by;
			if (dx > dy)
				return PATHFIND_WA * dy + PATHFIND_WB * (dx - dy);
			return PATHFIND_WA * dx + PATHFIND_WB * (dy - dx);
		};
		if (!queue.empty() && !path_found) {
			double min_dist = DBL_MAX;
			size_t min_dist_elem = SIZE_MAX;
			for (size_t elem : queue) {
				if (fdistance[elem] < min_dist) {
					min_dist = fdistance[elem];
					min_dist_elem = elem;
				}
			}
			queue.erase(std::remove(queue.begin(), queue.end(), min_dist_elem), queue.end());
			size_t next = min_dist_elem;
			visited[next] = true;

			size_t next_x = next % w;
			size_t next_y = next / w;

			for (auto const &direction : directions) {
				size_t dx = std::get<0>(direction);
				size_t dy = std::get<1>(direction);
				size_t new_x = next_x + dx;
				size_t new_y = next_y + dy;
				if (new_y >= h)
					continue;
				if (new_x >= w)
					continue;
				if (map[new_y][new_x])
					continue;
				size_t new_pos = new_x + new_y * w;
				size_t tentative_gscore = gdistance[next];
				if (gdistance[new_pos] > tentative_gscore) {
					double dist_to_new = heuristic(new_x, end_x, new_y, end_y);
					parent[new_pos] = next;
					gdistance[new_pos] = tentative_gscore;
					fdistance[new_pos] = tentative_gscore + dist_to_new;
					queue.push_back(new_pos);
					if (new_pos == end_x + end_y * w) {
						path_found = true;
						size_t pixs = 1;
						size_t end = parent[end_x + end_y * w];
						while (end != start_x + start_y * w) {
							end = parent[end];
							++pixs;
						}
						std::cout << "Path length: " << pixs << "\n";
					}
				}
			}
		}

		// events
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				quit = true;
		}

		// frame stuff
		uint64_t now = SDL_GetTicks64();
		if (next_time > now)
			SDL_Delay(next_time - now);
		next_time += TICK_INTERVAL;
	}

	// Destroy window
	SDL_DestroyWindow(window);

	// Quit SDL subsystems
	SDL_Quit();

	return 0;
}
