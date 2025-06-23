#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <math.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const double map_lat_min = 40.0;
const double map_lat_max = 41.0;
const double map_lon_min = 28.0;
const double map_lon_max = 29.0;

const int window_width = 800;
const int window_height = 600;

int gps_fd = -1;

double current_lat = 40.1;
double current_lon = 28.1;

void init_gps() {
	gps_fd = open("/dev/ttyUSB0", O_RDONLY | O_NOCTTY);
	if (gps_fd < 0) {
		perror("Failed to open GPS");
		exit(1);
	}

	struct termios tty;
	tcgetattr(gps_fd, &tty);
	cfsetispeed(&tty, B9600);
	cfsetospeed(&tty, B9600);
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 1;
	tcsetattr(gps_fd, TCSANOW, &tty);
}

int read_gps_line(char* buffer, int max_len) {
	char c;
	int idx = 0;
	while (idx < max_len - 1) {
		int n = read(gps_fd, &c, 1);
		if (n <= 0) break;
		if (c == '\n') break;
		buffer[idx++] = c;
	}
	buffer[idx] = '\0';
	return idx;
}

int parse_gprmc(const char* line, double* lat, double* lon) {
	if (strncmp(line, "$GPRMC", 6) != 0) return 0;

	char copy[128];
	strncpy(copy, line, sizeof(copy)-1);
	char* fields[20];
	int i = 0;
	char* token = strtok(copy, ",");
	while (token && i < 20) {
		fields[i++] = token;
		token = strtok(NULL, ",");
	}

	if (i < 7 || strcmp(fields[2], "A") != 0) return 0;

	double lat_raw = atof(fields[3]);
	double lon_raw = atof(fields[5]);
	char lat_dir = fields[4][0];
	char lon_dir = fields[6][0];

	double lat_deg = (int)(lat_raw / 100);
	double lon_deg = (int)(lon_raw / 100);

	*lat = lat_deg + (lat_raw - lat_deg * 100) / 60.0;
	*lon = lon_deg + (lon_raw - lon_deg * 100) / 60.0;

	if (lat_dir == 'S') *lat = -*lat;
	if (lon_dir == 'W') *lon = -*lon;

	return 1;
}

void gps_to_pixels(double lat, double lon, int img_w, int img_h, int* x, int* y) {
	double x_ratio = (lon - map_lon_min) / (map_lon_max - map_lon_min);
	double y_ratio = (map_lat_max - lat) / (map_lat_max - map_lat_min);
	*x = (int)(x_ratio * img_w);
	*y = (int)(y_ratio * img_h);
}

int main(int argc, char* argv[]) {
	//init_gps();

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);

	SDL_Window* window = SDL_CreateWindow("navmap",
									   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
									   window_width, window_height, SDL_WINDOW_SHOWN);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_Surface* map_surface = IMG_Load("map.png");
	if (!map_surface) return 1;

	SDL_Texture* map_texture = SDL_CreateTextureFromSurface(renderer, map_surface);
	int map_width = map_surface->w;
	int map_height = map_surface->h;
	SDL_FreeSurface(map_surface);

	bool running = true;
	SDL_Event event;

	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				running = false;
		}

		double prev_lat = current_lat;
		double prev_lon = current_lon;

		const Uint8* keystate = SDL_GetKeyboardState(NULL);
		if (keystate[SDL_SCANCODE_CAPSLOCK]) running = false;
		if (keystate[SDL_SCANCODE_ESCAPE])   running = false;

		double move_step = 0.001;
		if (keystate[SDL_SCANCODE_UP])    current_lat += move_step;
		if (keystate[SDL_SCANCODE_DOWN])  current_lat -= move_step;
		if (keystate[SDL_SCANCODE_LEFT])  current_lon -= move_step;
		if (keystate[SDL_SCANCODE_RIGHT]) current_lon += move_step;

		char gps_line[128];
		while (read_gps_line(gps_line, sizeof(gps_line))) {
			double lat, lon;
			if (parse_gprmc(gps_line, &lat, &lon)) {
				current_lat = lat;
				current_lon = lon;
				break;
			}
		}

		int gps_px, gps_py;
		gps_to_pixels(current_lat, current_lon, map_width, map_height, &gps_px, &gps_py);

		double dx = current_lon - prev_lon;
		double dy = current_lat - prev_lat;

		double angle_rad = atan2(dx, dy);
		double angle_deg = angle_rad * (180.0 / M_PI);
		double map_rotation_deg = -angle_deg;

		int draw_x = window_width  / 2 - gps_px;
		int draw_y = window_height / 2 - gps_py;

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		SDL_Rect dst = { draw_x, draw_y, map_width, map_height };
		SDL_Point pivot = { gps_px, gps_py };

		SDL_RenderCopyEx(renderer, map_texture, NULL, &dst, map_rotation_deg, &pivot, SDL_FLIP_NONE);

		SDL_SetRenderDrawColor(renderer, 250, 50, 50, 255);
		SDL_Rect center_marker = {
			window_width / 2 - 5,
			window_height / 2 - 5,
			20, 20
		};
		SDL_RenderFillRect(renderer, &center_marker);

		SDL_Rect minimap_dst = {
			10, 10,
			map_width / 4,
			(map_height * (map_width / 4)) / map_width
		};
		SDL_RenderCopy(renderer, map_texture, NULL, &minimap_dst);

		int mini_px = minimap_dst.x + (gps_px * minimap_dst.w) / map_width;
		int mini_py = minimap_dst.y + (gps_py * minimap_dst.h) / map_height;

		SDL_Rect mini_marker = {
			mini_px - 2,
			mini_py - 2,
			10,10
		};
		SDL_RenderFillRect(renderer, &mini_marker);

		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}

	SDL_DestroyTexture(map_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
	close(gps_fd);
	return 0;
}




