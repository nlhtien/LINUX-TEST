#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
 
#define I2C_BUS_AVAILABLE       (          1 )              // I2C Bus 1
#define SLAVE_DEVICE_NAME       ( "ETX_OLED" )              // Device and Driver Name
#define SSD1306_SLAVE_ADDR      (       0x3C )              // SSD1306 -Slave Address
#define SSD1306_MAX_SEG         (        128 )              // Maximum segment
#define SSD1306_MAX_LINE        (          7 )              // Maximum line
#define SSD1306_DEF_FONT_SIZE   (          5 )              // Default font size

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kora");
MODULE_DESCRIPTION("SSD1306 I2C Driver");
MODULE_VERSION("1.40");
 
static int  I2C_Read( unsigned char *out_buf, unsigned int len );
static int  I2C_Write( unsigned char *buf, unsigned int len );
static void SSD1306_PrintChar( unsigned char c );
static void SSD1306_String( unsigned char *str );

static void SSD1306_StartScrollHorizontal( bool is_left_scroll,
                                           uint8_t start_line_no,
                                           uint8_t end_line_no
                                         );

static int  SSD1306_DisplayInit( void );
static void SSD1306_Fill( unsigned char data );
static void SSD1306_GoToNextLine( void );
static void SSD1306_SetCursor( uint8_t lineNo, uint8_t cursorPos );
static void SSD1306_Write(bool is_cmd, unsigned char data);


static struct i2c_adapter *etx_i2c_adapter     = NULL;  // I2C Adapter Structure
static struct i2c_client  *etx_i2c_client_oled = NULL;  // I2C Cient Structure (In our case it is OLED)

static uint8_t SSD1306_LineNum   = 0;
static uint8_t SSD1306_CursorPos = 0;
static uint8_t SSD1306_FontSize  = SSD1306_DEF_FONT_SIZE;

static const unsigned char SSD1306_font[][SSD1306_DEF_FONT_SIZE]= 
{
    {0x00, 0x00, 0x00, 0x00, 0x00},   // space
    {0x00, 0x00, 0x2f, 0x00, 0x00},   // !
    {0x00, 0x07, 0x00, 0x07, 0x00},   // "
    {0x14, 0x7f, 0x14, 0x7f, 0x14},   // #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12},   // $
    {0x23, 0x13, 0x08, 0x64, 0x62},   // %
    {0x36, 0x49, 0x55, 0x22, 0x50},   // &
    {0x00, 0x05, 0x03, 0x00, 0x00},   // '
    {0x00, 0x1c, 0x22, 0x41, 0x00},   // (
    {0x00, 0x41, 0x22, 0x1c, 0x00},   // )
    {0x14, 0x08, 0x3E, 0x08, 0x14},   // *
    {0x08, 0x08, 0x3E, 0x08, 0x08},   // +
    {0x00, 0x00, 0xA0, 0x60, 0x00},   // ,
    {0x08, 0x08, 0x08, 0x08, 0x08},   // -
    {0x00, 0x60, 0x60, 0x00, 0x00},   // .
    {0x20, 0x10, 0x08, 0x04, 0x02},   // /

    {0x3E, 0x51, 0x49, 0x45, 0x3E},   // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00},   // 1
    {0x42, 0x61, 0x51, 0x49, 0x46},   // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31},   // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10},   // 4
    {0x27, 0x45, 0x45, 0x45, 0x39},   // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30},   // 6
    {0x01, 0x71, 0x09, 0x05, 0x03},   // 7
    {0x36, 0x49, 0x49, 0x49, 0x36},   // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E},   // 9

    {0x00, 0x36, 0x36, 0x00, 0x00},   // :
    {0x00, 0x56, 0x36, 0x00, 0x00},   // ;
    {0x08, 0x14, 0x22, 0x41, 0x00},   // <
    {0x14, 0x14, 0x14, 0x14, 0x14},   // =
    {0x00, 0x41, 0x22, 0x14, 0x08},   // >
    {0x02, 0x01, 0x51, 0x09, 0x06},   // ?
    {0x32, 0x49, 0x59, 0x51, 0x3E},   // @

    {0x7C, 0x12, 0x11, 0x12, 0x7C},   // A
    {0x7F, 0x49, 0x49, 0x49, 0x36},   // B
    {0x3E, 0x41, 0x41, 0x41, 0x22},   // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C},   // D
    {0x7F, 0x49, 0x49, 0x49, 0x41},   // E
    {0x7F, 0x09, 0x09, 0x09, 0x01},   // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A},   // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F},   // H
    {0x00, 0x41, 0x7F, 0x41, 0x00},   // I
    {0x20, 0x40, 0x41, 0x3F, 0x01},   // J
    {0x7F, 0x08, 0x14, 0x22, 0x41},   // K
    {0x7F, 0x40, 0x40, 0x40, 0x40},   // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},   // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F},   // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E},   // O
    {0x7F, 0x09, 0x09, 0x09, 0x06},   // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E},   // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46},   // R
    {0x46, 0x49, 0x49, 0x49, 0x31},   // S
    {0x01, 0x01, 0x7F, 0x01, 0x01},   // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F},   // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F},   // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F},   // W
    {0x63, 0x14, 0x08, 0x14, 0x63},   // X
    {0x07, 0x08, 0x70, 0x08, 0x07},   // Y
    {0x61, 0x51, 0x49, 0x45, 0x43},   // Z

    {0x00, 0x7F, 0x41, 0x41, 0x00},   // [
    {0x55, 0xAA, 0x55, 0xAA, 0x55},   // Backslash (Checker pattern)
    {0x00, 0x41, 0x41, 0x7F, 0x00},   // ]
    {0x04, 0x02, 0x01, 0x02, 0x04},   // ^
    {0x40, 0x40, 0x40, 0x40, 0x40},   // _
    {0x00, 0x03, 0x05, 0x00, 0x00},   // `

    {0x20, 0x54, 0x54, 0x54, 0x78},   // a
    {0x7F, 0x48, 0x44, 0x44, 0x38},   // b
    {0x38, 0x44, 0x44, 0x44, 0x20},   // c
    {0x38, 0x44, 0x44, 0x48, 0x7F},   // d
    {0x38, 0x54, 0x54, 0x54, 0x18},   // e
    {0x08, 0x7E, 0x09, 0x01, 0x02},   // f
    {0x18, 0xA4, 0xA4, 0xA4, 0x7C},   // g
    {0x7F, 0x08, 0x04, 0x04, 0x78},   // h
    {0x00, 0x44, 0x7D, 0x40, 0x00},   // i
    {0x40, 0x80, 0x84, 0x7D, 0x00},   // j
    {0x7F, 0x10, 0x28, 0x44, 0x00},   // k
    {0x00, 0x41, 0x7F, 0x40, 0x00},   // l
    {0x7C, 0x04, 0x18, 0x04, 0x78},   // m
    {0x7C, 0x08, 0x04, 0x04, 0x78},   // n
    {0x38, 0x44, 0x44, 0x44, 0x38},   // o
    {0xFC, 0x24, 0x24, 0x24, 0x18},   // p
    {0x18, 0x24, 0x24, 0x18, 0xFC},   // q
    {0x7C, 0x08, 0x04, 0x04, 0x08},   // r
    {0x48, 0x54, 0x54, 0x54, 0x20},   // s
    {0x04, 0x3F, 0x44, 0x40, 0x20},   // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C},   // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C},   // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C},   // w
    {0x44, 0x28, 0x10, 0x28, 0x44},   // x
    {0x1C, 0xA0, 0xA0, 0xA0, 0x7C},   // y
    {0x44, 0x64, 0x54, 0x4C, 0x44},   // z

    {0x00, 0x10, 0x7C, 0x82, 0x00},   // {
    {0x00, 0x00, 0xFF, 0x00, 0x00},   // |
    {0x00, 0x82, 0x7C, 0x10, 0x00},   // }
    {0x00, 0x06, 0x09, 0x09, 0x06}    // ~ (Degrees)
};

static int I2C_Write(unsigned char *buf, unsigned int len)
{
  int ret = i2c_master_send(etx_i2c_client_oled, buf, len);
  
  return ret;
}

static int I2C_Read(unsigned char *out_buf, unsigned int len)
{
  int ret = i2c_master_recv(etx_i2c_client_oled, out_buf, len);
  
  return ret;
}

static void SSD1306_Write(bool is_cmd, unsigned char data)
{
  unsigned char buf[2] = {0};
  int ret;
   
  if( is_cmd == true )
  {
      buf[0] = 0x00;
  }
  else
  {
      buf[0] = 0x40;
  }
  
  buf[1] = data;
  
  ret = I2C_Write(buf, 2);
}

static void SSD1306_SetCursor( uint8_t lineNo, uint8_t cursorPos )
{
  /* Move the Cursor to specified position only if it is in range */
  if((lineNo <= SSD1306_MAX_LINE) && (cursorPos < SSD1306_MAX_SEG))
  {
    SSD1306_LineNum   = lineNo;             // Save the specified line number
    SSD1306_CursorPos = cursorPos;          // Save the specified cursor position

    SSD1306_Write(true, 0x21);              // cmd for the column start and end address
    SSD1306_Write(true, cursorPos);         // column start addr
    SSD1306_Write(true, SSD1306_MAX_SEG-1); // column end addr

    SSD1306_Write(true, 0x22);              // cmd for the page start and end address
    SSD1306_Write(true, lineNo);            // page start addr
    SSD1306_Write(true, SSD1306_MAX_LINE);  // page end addr
  }
}

static void  SSD1306_GoToNextLine( void )
{
  SSD1306_LineNum++;
  SSD1306_LineNum = (SSD1306_LineNum & SSD1306_MAX_LINE);

  SSD1306_SetCursor(SSD1306_LineNum,0); /* Finally move it to next line */
}

static void SSD1306_PrintChar(unsigned char c)
{
  uint8_t data_byte;
  uint8_t temp = 0;
 
  if( (( SSD1306_CursorPos + SSD1306_FontSize ) >= SSD1306_MAX_SEG ) ||
      ( c == '\n' )
  )
  {
    SSD1306_GoToNextLine();
  }

  // print charcters other than new line
  if( c != '\n' )
  {
  
    c -= 0x20;  //or c -= ' ';

    do
    {
      data_byte= SSD1306_font[c][temp]; // Get the data to be displayed from LookUptable

      SSD1306_Write(false, data_byte);  // write data to the OLED
      SSD1306_CursorPos++;
      
      temp++;
      
    } while ( temp < SSD1306_FontSize);
    SSD1306_Write(false, 0x00);         //Display the data
    SSD1306_CursorPos++;
  }
}


static void SSD1306_String(unsigned char *str)
{
  while(*str)
  {
    SSD1306_PrintChar(*str++);
  }
}

static void SSD1306_StartScrollHorizontal( bool is_left_scroll,
                                           uint8_t start_line_no,
                                           uint8_t end_line_no
                                         )
{
  if(is_left_scroll)
  {
    // left horizontal scroll
    SSD1306_Write(true, 0x27);
  }
  else
  {
    // right horizontal scroll 
    SSD1306_Write(true, 0x26);
  }
  
  SSD1306_Write(true, 0x00);            // Dummy byte (dont change)
  SSD1306_Write(true, start_line_no);   // Start page address
  SSD1306_Write(true, 0x00);            // 5 frames interval
  SSD1306_Write(true, end_line_no);     // End page address
  SSD1306_Write(true, 0x00);            // Dummy byte (dont change)
  SSD1306_Write(true, 0xFF);            // Dummy byte (dont change)
  SSD1306_Write(true, 0x2F);            // activate scroll
}


static int SSD1306_DisplayInit(void)
{
  msleep(100);               // delay

  /*
  ** Commands to initialize the SSD_1306 OLED Display
  */
  SSD1306_Write(true, 0xAE); // Entire Display OFF
  SSD1306_Write(true, 0xD5); // Set Display Clock Divide Ratio and Oscillator Frequency
  SSD1306_Write(true, 0x80); // Default Setting for Display Clock Divide Ratio and Oscillator Frequency that is recommended
  SSD1306_Write(true, 0xA8); // Set Multiplex Ratio
  SSD1306_Write(true, 0x3F); // 64 COM lines
  SSD1306_Write(true, 0xD3); // Set display offset
  SSD1306_Write(true, 0x00); // 0 offset
  SSD1306_Write(true, 0x40); // Set first line as the start line of the display
  SSD1306_Write(true, 0x8D); // Charge pump
  SSD1306_Write(true, 0x14); // Enable charge dump during display on
  SSD1306_Write(true, 0x20); // Set memory addressing mode
  SSD1306_Write(true, 0x00); // Horizontal addressing mode
  SSD1306_Write(true, 0xA1); // Set segment remap with column address 127 mapped to segment 0
  SSD1306_Write(true, 0xC8); // Set com output scan direction, scan from com63 to com 0
  SSD1306_Write(true, 0xDA); // Set com pins hardware configuration
  SSD1306_Write(true, 0x12); // Alternative com pin configuration, disable com left/right remap
  SSD1306_Write(true, 0x81); // Set contrast control
  SSD1306_Write(true, 0x80); // Set Contrast to 128
  SSD1306_Write(true, 0xD9); // Set pre-charge period
  SSD1306_Write(true, 0xF1); // Phase 1 period of 15 DCLK, Phase 2 period of 1 DCLK
  SSD1306_Write(true, 0xDB); // Set Vcomh deselect level
  SSD1306_Write(true, 0x20); // Vcomh deselect level ~ 0.77 Vcc
  SSD1306_Write(true, 0xA4); // Entire display ON, resume to RAM content display
  SSD1306_Write(true, 0xA6); // Set Display in Normal Mode, 1 = ON, 0 = OFF
  SSD1306_Write(true, 0x2E); // Deactivate scroll
  SSD1306_Write(true, 0xAF); // Display ON in normal mode
  
  //Clear the display
  SSD1306_Fill(0x00);
  return 0;
}
 

static void SSD1306_Fill(unsigned char data)
{
  unsigned int total  = 128 * 8;  // 8 pages x 128 segments x 8 bits of data
  unsigned int i      = 0;
  
  //Fill the Display
  for(i = 0; i < total; i++)
  {
      SSD1306_Write(false, data);
  }
}
 

static int etx_oled_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
  SSD1306_DisplayInit();
  SSD1306_Fill(0xFF);
  
  //Set cursor
  //SSD1306_SetCursor(0,0);  
  //SSD1306_StartScrollHorizontal( true, 0, 2);

  //Write String to OLED
  //SSD1306_String("HELLO KORA");
  
  pr_info("OLED Probed!!!\n");
  
  return 0;
}
 

static int etx_oled_remove(struct i2c_client *client)
{

  SSD1306_String("ThanK YoU!!!");
  
  msleep(1000);
  
  //Set cursor
  SSD1306_SetCursor(0,0);  
  //clear the display
  SSD1306_Fill(0x00);
  
  SSD1306_Write(true, 0xAE); // Entire Display OFF
  
  pr_info("OLED Removed!!!\n");
  return 0;
}
 
/*
** Structure that has slave device id
*/
static const struct i2c_device_id etx_oled_id[] = {
  { SLAVE_DEVICE_NAME, 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, etx_oled_id);
 
/*
** I2C driver Structure that has to be added to linux
*/
static struct i2c_driver etx_oled_driver = {
  .driver = {
    .name   = SLAVE_DEVICE_NAME,
    .owner  = THIS_MODULE,
  },
  .probe          = etx_oled_probe,
  .remove         = etx_oled_remove,
  .id_table       = etx_oled_id,
};
 
/*
** I2C Board Info strucutre
*/
static struct i2c_board_info oled_i2c_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SSD1306_SLAVE_ADDR)
};
 
/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
  int ret = -1;
  etx_i2c_adapter     = i2c_get_adapter(I2C_BUS_AVAILABLE);
  
  if( etx_i2c_adapter != NULL )
  {
    etx_i2c_client_oled = i2c_new_device(etx_i2c_adapter, &oled_i2c_board_info);
    
    if( etx_i2c_client_oled != NULL )
    {
      i2c_add_driver(&etx_oled_driver);
      ret = 0;
    }
      
      i2c_put_adapter(etx_i2c_adapter);
  }
  
  pr_info("Driver Added!!!\n");
  return ret;
}
 
/*
** Module Exit function
*/
static void __exit etx_driver_exit(void)
{
  i2c_unregister_device(etx_i2c_client_oled);
  i2c_del_driver(&etx_oled_driver);
  pr_info("Driver Removed!!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
