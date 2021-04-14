/******************************************************************************

Created with GDB Online in C++ mode. https://www.onlinegdb.com/

*******************************************************************************/
#include <stdio.h>

#define buffer_size 22*((22+7)/8)
static char temp_data[buffer_size];

struct PBM
{
  char rows, cols;
  char bpl; // bytes per line
  char *data;

  PBM (char rows, char cols, char *data):rows (rows), cols (cols),
    bpl ((cols + 7) / 8), data (data)
  {
  }

  ~PBM ()
  {
  }

  unsigned char get_pixel (char row, char col)
  {
    return (data[row * bpl + col / 8] & (1 << (7 - (col & 7)))) == 0 ? 0 : 1;
  }

  void set_pixel (char row, char col, unsigned char value)
  {
    if (value)
    {
 data[row * bpl + col / 8] |= (1 << (7 - (col & 7)));
    }
    else
    {
 data[row * bpl + col / 8] &= ~(1 << (7 - (col & 7)));
    }
  }

  void clear (char value = 0)
  {
    for (int i = 0; i < ((int) rows) * bpl; i++)
    {
 data[i] = value;
    }
  }
};

// make a mask for the open percentage of the eye
void
make_lid_mask (PBM & image, float percent_open)
{
  float lid_r = 11;
  float s = (100 / (percent_open + 1)) * (100 / (percent_open + 1));
  for (char row = 0; row < image.rows; row++)
  {
    for (char col = 0; col < image.cols; col++)
{
 float r = (s * (row - float (image.rows) / 2 + 0.5) *
    (row - float (image.rows) / 2 + 0.5) +
    (col - float (image.cols) / 2 + 0.5) *
    (col - float (image.cols) / 2 + 0.5));
 if (r > lid_r * lid_r)
      {
   image.set_pixel (row, col, 0);
 }
 else
 {
   image.set_pixel (row, col, 1);
 }
}
  }
}

void
visualize_lid_mask (PBM & image)
{
  for (char row = 0; row < image.rows; row++)
  {
    for (char col = 0; col < image.cols; col++)
{
 if (image.get_pixel(row, col))
 {
   printf("00");
 }
 else
 {
   printf("  ");
 }
}
printf("\n");
  }
}

void
print_temp_data(int suffix)
{
    printf("PROGMEM const unsigned char eyelid%d[] = {\n  ",suffix);
    for (int i = 0; i < buffer_size; i++)
    {
        printf(" 0x%02X", ((unsigned char)temp_data[i]));
        if (i < buffer_size-1)
        {
            printf(",");
        }
        if (0 == (i+1)%10)
        {
            printf("\n  ");
        }
    }
    printf("};\n");
}

int
main ()
{
  PBM temp (22, 22, temp_data);

  for (int opening=10; opening <= 100; opening+=10)
  {
    make_lid_mask(temp, opening);
    visualize_lid_mask(temp);
    print_temp_data(opening);
  }

  return 0;
}
