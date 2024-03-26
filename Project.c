#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#define PS2_BASE			0xFF200100
#define HEX3_HEX0_BASE		0xFF200020

int pixel_buffer_start;       // global variable
short int Buffer1[240][512];  // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];
short int colour[7];
static int ball_x = 160; // Initial x position of the ball
static int ball_y = 200; // Initial y position of the ball
static int ball_dx = 8;  // Initial x direction of the ball
static int ball_dy = 8;  // Initial y direction of the ball
static int slab_width = 50; // Width of the slab
static int slab_height = 5; // Height of the slab
static int slab_x = 135; // Initial x position of the slab
static int slab_y = 232; // Initial y position of the slab
int x_box[64];
int y_box[64];
int points;

void wait_for_vsync();
void draw_initial_tiles();
void draw_current_tiles();
void draw_box(int x, int y, short int box_color);
void populate_color();
void plot_pixel(int x, int y, short int line_color);
void draw();
void clear_screen();
void draw_slab(int x, int y, int width, int height, short int color);
void draw_ball(int x, int y, int radius, short int color);
void check_tile_collision();
void display_points();


int main(void) {
  
  volatile int *pixel_ctrl_ptr = (int *)0xFF203020;

  *(pixel_ctrl_ptr + 1) = (int)&Buffer1;  // first store the address in the  back buffer

  wait_for_vsync();

  pixel_buffer_start = *pixel_ctrl_ptr;
  clear_screen();  // pixel_buffer_start points to the pixel buffer
  draw_border();
  *(pixel_ctrl_ptr + 1) = (int)&Buffer2;
  pixel_buffer_start = *(pixel_ctrl_ptr + 1);  // we draw on the back buffer

  volatile int * PS2_ptr = (int *)PS2_BASE; 
  int PS2_data, RVALID;
  char byte1 = 0, byte2 = 0, byte3 = 0;	
  points = 0;

	// PS/2 mouse needs to be reset (must be already plugged in)
  *(PS2_ptr) = 0xFF; // reset

  draw_border();
  clear_screen();  // pixel_buffer_start points to the pixel buffer
  populate_color();
  draw_initial_tiles();
  display_points();

 
  while (1) {
	 draw();
	 PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port RVALID
	 RVALID = PS2_data & 0x8000; // extract the RVALID field
	 if (RVALID != 0) {

		/* shift the next data byte into the display */
	 byte1 = byte2;
	 byte2 = byte3;
	 byte3 = PS2_data & 0xFF;
	 	if ((byte2 == (char)0xAA) && (byte3 == (char)0x00)) { // mouse inserted; initialize sending of data 
			*(PS2_ptr) = 0xF4;
		}
		if (byte3 == (char)0x74 && byte2 == (char)0xE0) {
			if (slab_x+50 <= 315)
				slab_x += 10;
		}
		if (byte3 == (char)0x6B && byte2 == (char)0xE0) {
			if (slab_x >= 4)
				slab_x -= 10;
		}
	 }
    wait_for_vsync();  // swap front and back buffers on VGA vertical sync
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);  // new back buffer
  }
  
}

void plot_pixel(int x, int y, short int line_color) {
  short int *one_pixel_address;

  one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);

  *one_pixel_address = line_color;
}

void draw() {
    // should erase the previous content of the back buffer, two possible ways:
    // draw black pixels everywhere (slow)‣
    // faster, but harder: erase only what you drew - 2 frames ago!‣
    // for each box i
    clear_screen();
    draw_current_tiles();
    draw_slab(slab_x, slab_y, slab_width, slab_height, 0xFFFF);

    // Update ball position
    ball_x += ball_dx;
    ball_y += ball_dy;

    // Check for collisions with screen edges, considering the border
    if (ball_x - 5 <= 3 || ball_x + 5 >= 316)
        ball_dx = -ball_dx; // Reverse direction on x-axis
    if (ball_y + 5 >= 240) {
        // Ball has reached or gone past the bottom edge of the screen
        // Reset the ball position to the middle
        ball_x = 160;
        ball_y = 120;
        ball_dy = -8; // Move straight up
    } else if (ball_y - 5 <= 3) {
        ball_dy = -ball_dy; // Reverse direction on y-axis
    }

    // Check for collision with the slab
    if (ball_y + 5 >= slab_y && ball_y - 5 <= slab_y + slab_height) {
        if (ball_x + 5 >= slab_x && ball_x - 5 <= slab_x + slab_width) {
            // Ball has collided with the slab
            ball_dy = -ball_dy; // Reverse direction on y-axis
        }
    }
    check_tile_collision();
    // Draw the ball
    draw_ball(ball_x, ball_y, 3, 0xFFFF); // Adjust the radius and color as needed    
}

void draw_border() {
    int x, y;
    // Drawing the wider border
    for (x = 0; x < 320; x++) {
        for (y = 0; y < 4; y++) {
            plot_pixel(x, y, 0xFFFF);           // Top border
        }
    }
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 4; x++) {
            plot_pixel(x, y, 0xFFFF);           // Left border
            plot_pixel(319 - x, y, 0xFFFF);     // Right border
        }
    }
}

void clear_screen() {
    // Clearing the inner area
    for (int x = 4; x < 316; x++) {
        for (int y = 4; y < 236; y++) {
            plot_pixel(x, y, 0);
        }
    }
}

void populate_color() {
  colour[0] = 0xA840;
  colour[1] = 0x07e0;
  colour[2] = 0xf800;
  colour[3] = 0x001f;
  colour[4] = 0xf81f;
  colour[5] = 0x07ff;
  colour[6] = 0xffe0;
  //colour[7] = 0x4B82;
}

void draw_initial_tiles() {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 7; j++) {
			draw_box(39*i + 4, j*12 + 4, colour[j]);
			x_box[7*i + j] = 39*i + 4;
			y_box[7*i + j] = 12*j + 4;
		}
	}
}

void draw_current_tiles() {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 7; j++) {
			if (x_box[7*i + j] == -100) {
				continue;
			}
			draw_box(39*i + 4, j*12 + 4, colour[j]);
		}
	}
}

void draw_box(int x, int y, short int box_color) {
  bool draw_black = false;
  for (int i = x; i < x+39; i++) {
	  if (i == x+38) draw_black = true; 
	  for (int j = y; j < y+12; j++) {
		  if (draw_black) plot_pixel(i, j, 0);
		  else plot_pixel(i, j, box_color);
	 }
  }
}

void draw_ball(int x, int y, int radius, short int color) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                plot_pixel(x + dx, y + dy, color);
            }
        }
    }
}

void draw_slab(int x, int y, int width, int height, short int color) {
    // Draw the rectangle representing the slab
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            plot_pixel(x + i, y + j, color);
        }
    }
}

void check_tile_collision() {
	if (ball_y-5 > 88) {
		return;
	}
	for (int i = 0; i < 56; i++) {
		if ((ball_y-5 <= y_box[i]+12 || ball_y-5 <= y_box[i]) && ball_x-5 >= x_box[i] 
			&& ball_x+5 <= x_box[i] + 39) {
			ball_dy *= -1;
			x_box[i] = -100;
			points += 5;
			display_points();
		}
	}
}

void wait_for_vsync() {
  volatile int *pixel_ctrl_ptr = (int *)0xff203020;  // base address
  int status;
  *pixel_ctrl_ptr = 1;  // start the synchronization process
  // - write 1 into front buffer address register
  status = *(pixel_ctrl_ptr + 3);  // read the status register
  while ((status & 0x01) != 0)     // polling loop waiting for S bit to go to 0
  {
    status = *(pixel_ctrl_ptr + 3);
  }
}

void display_points() {
	volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;

    // Define the seven-segment codes for displaying numbers
    unsigned char seven_seg_decode_table[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
                                              0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};

    // Display number 1 (index 1 in the seven_seg_decode_table)
		//*HEX3_HEX0_ptr = 
		*HEX3_HEX0_ptr = (seven_seg_decode_table[(points/100)%10] << 16) + 
						 (seven_seg_decode_table[(points/10)%10] << 8) + 
			 			  seven_seg_decode_table[points%10];
}