#include <TVout.h>

#include "sclera_data_22x22.h"
#include "iris_data_22x22.h"
#include "iris_mask_data_22x22.h"


TVout TV;

// storage for image buffers
#define buffer_size 22*((22+7)/8)
static char image_data[22*((22+7)/8)];
static char temp_data[22*((22+7)/8)];
static char eyelid1_data[buffer_size];

extern const unsigned char white_pixel[];

extern const unsigned char black_pixel[];
byte card_x, card_y;

extern const unsigned char cardcenter[];

extern const unsigned char eyelid20[];
extern const unsigned char eyelid40[];
extern const unsigned char eyelid60[];
extern const unsigned char eyelid80[];


// packed binary image
struct PBM
{
  char rows, cols;
  char bpl; // bytes per line
  char* data;

  PBM(char rows, char cols, char *data)
    :rows(rows), cols(cols), bpl((cols+7)/8), data(data)
  {
  }

  ~PBM()
  {
  }

  unsigned char get_pixel(char row, char col)
  {
    return (data[row*bpl + col/8] & (1 << (7 - (col & 7)))) == 0 ? 0 : 1;
  }

  void set_pixel(char row, char col, unsigned char value)
  {
    if (value){
      data[row*bpl + col/8] |=  (1 << (7 - (col & 7)));
    } else {
      data[row*bpl + col/8] &= ~(1 << (7 - (col & 7)));
    }
  }

  void clear(char value=0)
  {
    for (int i=0; i<((int)rows)*bpl; i++){
      data[i] = value;
    }
  }
};

// bit block transfer: copy source image to dest image where indicated by
// mask image, offset in x and y directions
void blit(PBM& dest,
    PBM& source,
    PBM& mask,
    char x_offset,
    char y_offset,
    char invert_mask=0)
{
  for (char r=0; r<source.rows; r++){
    char row = r + y_offset;
    if (row >=0 && row < dest.rows){
      for (char c=0; c<source.cols; c++){
        char col = c + x_offset;
        if (col >= 0 && col < dest.cols){
          if (mask.get_pixel(r, c) ^ invert_mask){
            dest.set_pixel(row, col, source.get_pixel(r, c));
          }
        }
      }
    }
  }
}

// draw a disk (or outside of a disk)
void disk(PBM& image,
    char cx,
    char cy,
    char radius,
    char value,
    int flip=1)
{
  radius *= radius;
  for (char row=0; row<image.rows; row++){
    for (char col=0; col<image.cols; col++){
      int r = ((row - cy)* (row - cy)
                 + (col - cx) * (col - cx));
      if ((r <= radius) ^ flip){
        image.set_pixel(row, col, value);
      }
    }
  }
}

// make a mask for the open percentage of the eye
void make_lid_mask(PBM& image, float percent_open)
{
  float lid_r = 11;
  float s = (100/(percent_open+1))*(100/(percent_open+1));
  for (char row=0; row<image.rows; row++){
    for (char col=0; col<image.cols; col++){
      float r = (s*(row - float(image.rows)/2+0.5) *
                 (row - float(image.rows)/2+0.5) +
                 (col - float(image.cols)/2+0.5) *
                 (col - float(image.cols)/2+0.5));
      if (r > lid_r*lid_r){
        image.set_pixel(row, col, 0);
      } else {
        image.set_pixel(row, col, 1);
      }
    }
  }
}

// This would have been an array or arrays except we ran into
// weirdness crossing the dynamic/program memory boundary.
char* eyelid_frame(int blink_frame)
{
	switch(blink_frame)
  {
  	case 0: return eyelid80;
  	case 1: return eyelid60;
  	case 2: return eyelid40;
  	case 3: return eyelid20;
  	case 4: return eyelid40;
  	case 5: return eyelid60;
  	case 6: return eyelid80;
  }
}

// draw eye to TV buffer
void draw_eye(PBM& image,
              PBM& temp,
              //PBM& eyelid1,
              int horiz,
              int vert,
              float pupil_size,
              int blink_frame)
{
  float glint_h_offset = -2.5; // position of eye glint
  float glint_v_offset = -2.5;
  float glint_size = 1;   // radius of eye glint

  image.clear(0);

  // add the sclera image
  PBM sclera(22, 22, sclera_data);
  blit(image, sclera, sclera, 0, 0);

  // add the iris image, offset appropriately
  PBM iris(22, 22, iris_data);
  PBM iris_mask(22, 22, iris_mask_data);
  blit(image, iris, iris_mask, horiz, vert);  

  // add the pupil, note that it moves 1.5x as much as the iris
  temp.clear(0);
  disk(temp,
       float(temp.cols)/2 + horiz/2 - 0.5,
       float(temp.rows)/2 + vert/2 - 0.5,
       pupil_size, 1, 1);
  blit(image, temp, temp, horiz, vert, 1);

  // add the glint
  temp.clear(0);
  disk(temp,
       float(temp.cols)/2 + glint_h_offset,
       float(temp.rows)/2 + glint_v_offset,
       glint_size, 1, 0);
  blit(image, temp, temp, horiz, vert, 0);
 
  // close eye to specified percent  
  //make_lid_mask(temp, percent_open); //comment these out later
  memcpy_P(temp_data, eyelid_frame(blink_frame), 22*((22+7)/8));
 blit(image, temp, temp, 0, 0, 1); //comment these out later
  
// copy to TV screen
  int n = 3, gap = 1;
  for (char row=0; row<image.rows; row++){
    for (char col=0; col<image.cols; col++){
      // For each image.get_pixel(row, col)
      // Call TV.bitmap() depending on color
      if (image.get_pixel(row,col))
      {
        TV.bitmap(12+col*4, 10+row*4, white_pixel); // numbers before the plus signs change position on screen
      }
      else
      {
        TV.bitmap(12+col*4, 10+row*4, black_pixel); // numbers before the plus signs change position on screen
      }
    }
  }
}


void setup()  {
  TV.begin(NTSC, 120, 96);
  //Serial.begin(9600);
}


void loop() {

  // create structures for the working images
  PBM image(22, 22, image_data);
  PBM temp(22, 22, temp_data);
  //PBM eyelid1(22, 22, eyelid1_data);

  TV.clear_screen();
  TV.bitmap(card_x, card_y, cardcenter);
  TV.delay(5000);
  TV.clear_screen(); 
  // the parameters of the eye
  char horiz = 0;             // -4..+4
  char vert = 0;              // -4..+4
  float pupil_size = 3.f;     // 0-10(?)
  float percent_open = 100.f; // 0-100

  // Make eye lid mask before running main loop
 // make_lid_mask(eyelid1, 80.f);1

  // simple loop to draw the eye in various states as a demo
  while(1){
   int hor_value = analogRead(A0);
   int vert_value = analogRead(A1);
   int light_value = analogRead(A2);
   int hor_position = map(hor_value,0,1023,-4,4);
   int vert_position = map(vert_value,0,1023,-4,4);
   int pupil_value = map(light_value,0,800,0,7);
        horiz = hor_position;
        vert = vert_position;
        pupil_size = (pupil_value);
        percent_open = 100.f;
        draw_eye(image, temp, horiz, vert, pupil_size, 5);
       //Serial.println(light_value);
        
      }
    }


PROGMEM const unsigned char white_pixel[] = {
  4,4
,0xe0
,0xe0
,0xe0
,0x00
};

PROGMEM const unsigned char black_pixel[] = {
  4,4
,0x00
,0x00
,0x00
,0x00
};

PROGMEM const unsigned char cardcenter[] = {  // 22x34 pixle 
  120, 96
,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x01,0x9c,0x3f,0x00,0x7c,0x0b,0x87,0xdb,0xe0,0x03,0xb1,0xb3,0xc0,0x00
,0x00,0x01,0xfc,0x3f,0x00,0xfe,0x0f,0x87,0xff,0xe0,0x07,0xfb,0xf7,0xe0,0x00
,0x00,0x00,0xfc,0x6f,0x01,0xff,0x0f,0x87,0xbf,0xe8,0x07,0xf9,0xf3,0xe0,0x00
,0x00,0x00,0xfe,0x6e,0x03,0xff,0x8f,0x87,0x9f,0xf8,0x07,0xfd,0xf3,0xe0,0x00
,0x00,0x00,0xfe,0x7e,0x07,0xff,0x8f,0x87,0x9e,0x7c,0x07,0x3d,0xc1,0xc0,0x00
,0x00,0x00,0x7e,0x7c,0x07,0xff,0x8f,0x87,0x9e,0x7c,0x07,0x1d,0xc1,0x80,0x00
,0x00,0x00,0x3f,0xfc,0x0f,0xe7,0x8f,0x87,0x9e,0x3c,0x07,0x1d,0xc1,0x80,0x00
,0x00,0x00,0x3f,0xf8,0x0f,0xc3,0xcf,0x87,0x9e,0x78,0x07,0x3d,0xc1,0x80,0x00
,0x00,0x00,0x1f,0xf8,0x1f,0x83,0xcf,0x87,0x9e,0x70,0x07,0x39,0xe1,0x80,0x00
,0x00,0x00,0x1f,0xf0,0x1f,0x83,0xcf,0x87,0x9e,0xf0,0x07,0xf9,0xe1,0x80,0x00
,0x00,0x00,0x1f,0xe0,0x1f,0x03,0xcd,0x07,0x9f,0xf0,0x07,0xf1,0xe1,0x80,0x00
,0x00,0x00,0x0f,0xe0,0x1f,0x03,0xcf,0x87,0x9f,0xe0,0x07,0xe1,0x81,0x80,0x00
,0x00,0x00,0x07,0xe0,0x1f,0x03,0xcf,0x87,0x9f,0xe0,0x07,0x01,0x81,0x80,0x00
,0x00,0x00,0x07,0xc0,0x3f,0x03,0xcf,0x87,0x9f,0xe0,0x07,0x01,0x81,0x80,0x00
,0x00,0x00,0x07,0xc0,0x3f,0x03,0xc7,0x87,0x1c,0xe0,0x07,0x01,0xf1,0x80,0x00
,0x00,0x00,0x07,0xc0,0x1f,0x03,0xc3,0x87,0x1c,0x60,0x06,0x00,0xe0,0x00,0x00
,0x00,0x00,0x05,0xc0,0x0f,0x07,0x83,0x8f,0x1c,0x70,0x06,0x01,0xe0,0x00,0x00
,0x00,0x00,0x05,0xc0,0x1f,0x8f,0x87,0xd0,0x00,0x38,0x03,0x00,0xc0,0x00,0x00
,0x00,0x00,0x05,0xc0,0x1f,0xcf,0x84,0x00,0x00,0x38,0x03,0x00,0x00,0x00,0x00
,0x00,0x00,0x07,0xc0,0x0f,0xdf,0x80,0x0f,0xfc,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x07,0xc0,0x07,0xff,0x00,0xff,0xfd,0xc0,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x03,0xc0,0x07,0xfe,0x07,0x3f,0xfe,0xf8,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x03,0xc0,0x03,0xfc,0x1f,0xcf,0xff,0x7e,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x03,0xc0,0x00,0xf0,0x7f,0xf7,0xff,0xff,0x80,0x00,0x00,0x00,0x00
,0x00,0x00,0x03,0x80,0x00,0x00,0xff,0xff,0xff,0xff,0xc0,0x00,0x00,0x00,0x00
,0x00,0x00,0x01,0x00,0x00,0x03,0xff,0xff,0xff,0xff,0xb0,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x07,0xff,0xff,0xff,0xff,0xf8,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x0f,0xff,0xff,0xf8,0x03,0xfc,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x1f,0xff,0xff,0xc0,0x00,0x7e,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x3b,0xff,0xff,0x01,0x50,0x1f,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x47,0xff,0xfe,0x0a,0xaa,0x0f,0x80,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x7f,0xff,0xfc,0x55,0x55,0x47,0x80,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xf8,0xaa,0xaa,0xa3,0xc0,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x01,0xff,0xff,0xf1,0x55,0x55,0x51,0xe0,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x01,0xff,0xff,0xe2,0xaa,0xaa,0xa8,0xe0,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x03,0xff,0xff,0xc5,0x57,0x55,0x54,0x70,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x03,0xff,0xff,0xca,0xaf,0x80,0xaa,0x70,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x07,0xff,0xff,0x95,0x5f,0xc0,0x54,0x38,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x07,0xff,0xff,0x8a,0xbf,0xc0,0x2a,0x38,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x07,0xdb,0xff,0x95,0x5f,0xc0,0x15,0x38,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x0f,0xa7,0xff,0x0a,0xaf,0x80,0x0a,0x1c,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x08,0x7f,0xff,0x15,0x57,0x00,0x05,0x1c,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x07,0xbf,0xff,0x2a,0xa8,0x00,0x0a,0x9c,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x0f,0xdf,0xff,0x15,0x50,0x00,0x05,0x1c,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1f,0xdf,0xff,0x2a,0xa8,0x00,0x0a,0x9e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1f,0xef,0xff,0x15,0x50,0x00,0x05,0x1e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1f,0xff,0xff,0x2a,0xa8,0x00,0x0a,0x9e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1f,0xff,0xff,0x15,0x54,0x00,0x15,0x1e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1f,0xff,0xff,0x2a,0xaa,0x00,0x2a,0x1e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1f,0xff,0xff,0x95,0x55,0x00,0x55,0x3e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1d,0xff,0xff,0x8a,0xaa,0x80,0xaa,0x3e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1d,0xff,0xff,0x95,0x55,0x55,0x54,0x3e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1d,0xff,0xff,0xca,0xaa,0xaa,0xaa,0x7e,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1d,0xff,0xff,0xc5,0x55,0x55,0x54,0x7a,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x0d,0xff,0xff,0xe2,0xaa,0xaa,0xa8,0xfc,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x0e,0xff,0xff,0xf1,0x55,0x55,0x51,0xfc,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x0e,0xff,0xff,0xf8,0xaa,0xaa,0xa3,0xfc,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x0e,0xf7,0xff,0xfc,0x55,0x55,0x47,0xfc,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x07,0x6f,0xff,0xfe,0x2a,0xaa,0x8f,0xf8,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x07,0x58,0x3f,0xff,0x05,0x54,0x1f,0xf8,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xff,0xc0,0x00,0x7f,0xf8,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xc0,0x03,0x81,0xff,0xf8,0x03,0xff,0xb0,0x00,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xff,0xf9,0xff,0xff,0xff,0xff,0xd0,0x00,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xff,0xc0,0x30,0x60,0x03,0x03,0x80,0x00,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xff,0xf0,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xff,0xe3,0xcf,0x0f,0xce,0x7c,0x7f,0xe0,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xff,0xe3,0xef,0x0f,0xfe,0x7f,0xff,0xc0,0x00,0x00
,0x00,0x00,0x00,0x01,0x7f,0xff,0xe1,0xff,0x9c,0xf8,0x7f,0xff,0xc0,0x00,0x00
,0x00,0x00,0x00,0x01,0x7f,0xf8,0x09,0xff,0x9d,0xf8,0x7f,0xff,0x00,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xe0,0x3c,0xff,0x9f,0xf0,0x3f,0xff,0xc0,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xe0,0x1e,0x7f,0xff,0xe2,0x3f,0xf0,0x00,0x00,0x00
,0x00,0x00,0x00,0x01,0xff,0xff,0x8d,0x3f,0xff,0xc6,0x7f,0xc0,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0x93,0x1d,0xff,0x8e,0x7f,0xc0,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0x1c,0x8d,0xff,0x1e,0x7f,0xfe,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0x8f,0x0f,0xfe,0x3c,0x7f,0xff,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0x0f,0xe7,0xfc,0x7c,0x7f,0xff,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xe0,0x07,0xe3,0xfc,0xf8,0x7f,0xff,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xc0,0x00,0xf3,0xf8,0xc0,0x7f,0xfe,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xc0,0x00,0x03,0xf8,0x00,0x7f,0x80,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0xc0,0x03,0xf8,0x00,0x3f,0xc0,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0xc0,0x03,0xf8,0x00,0x3f,0xe1,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0xc0,0x03,0xf8,0x00,0x2f,0xff,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xdf,0xff,0x80,0x01,0xf8,0x00,0x2f,0xff,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0xff,0xff,0x80,0x01,0xf0,0x00,0x3f,0xfe,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x1f,0xff,0xe0,0x01,0xf0,0x00,0x2f,0xff,0x80,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00

};

PROGMEM const unsigned char eyelid20[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xE0,
   0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFC, 0x1F, 0xFF, 0xE0, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   
PROGMEM const unsigned char eyelid40[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30,
   0x00, 0x0F, 0xFF, 0xC0, 0x3F, 0xFF, 0xF0, 0x7F, 0xFF, 0xF8,
   0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFC, 0x7F, 0xFF, 0xF8, 0x3F,
   0xFF, 0xF0, 0x0F, 0xFF, 0xC0, 0x00, 0x30, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   
PROGMEM const unsigned char eyelid60[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xFC, 0x00, 0x07, 0xFF, 0x80, 0x1F, 0xFF,
   0xE0, 0x3F, 0xFF, 0xF0, 0x7F, 0xFF, 0xF8, 0xFF, 0xFF, 0xFC,
   0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFC, 0x7F,
   0xFF, 0xF8, 0x3F, 0xFF, 0xF0, 0x1F, 0xFF, 0xE0, 0x07, 0xFF,
   0x80, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   
PROGMEM const unsigned char eyelid80[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x07,
   0xFF, 0x80, 0x1F, 0xFF, 0xE0, 0x3F, 0xFF, 0xF0, 0x3F, 0xFF,
   0xF0, 0x7F, 0xFF, 0xF8, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFC,
   0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFC, 0xFF,
   0xFF, 0xFC, 0x7F, 0xFF, 0xF8, 0x3F, 0xFF, 0xF0, 0x3F, 0xFF,
   0xF0, 0x1F, 0xFF, 0xE0, 0x07, 0xFF, 0x80, 0x00, 0xFC, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
