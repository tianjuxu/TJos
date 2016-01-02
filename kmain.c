/***********includes*******************************/
#include "io.h"
#include "multiboot.h"
/***********FB & IO ports defines***********************/
#define FB_COMMAND_PORT  0x3D4
#define FB_DATA_PORT     0x3D5
#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND  15
/* All the serial IO ports are calculated relative tothe data port. 
 * This is because all serial ports (COM1,COM2,COM3,COM4) have their 
 * ports in same order, but they start from a different value.
*/
#define SERIAL_COM1_BASE                0x3F8
#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base+2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base+3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base+4)
#define SERIAL_LINE_STATUS_PORT(base)   (base+5)
/*IO port commands*/

/* SERIAL_LINE_ENABLE_DLAB:
 * tells the serial port to expect first the highest 8 bits on the data port,
 *then the lowest 8 bits will follow
*/
#define SERIAL_LINE_ENABLE_DLAB          0x80
/***********PIC ports******************************/
#define PIC1_PORT_A      0x20
#define PIC2_PORT_A      0xA0
/*PIC interrupts have been re-mapped*/
#define PIC1_START_INTERRUPT 0x20
#define PIC2_START_INTERRUPT 0x28
#define PIC2_END_INTERRUPT   PIC2_START_INTERRUPT + 7
#define PIC_ACK              0x20                 
/***********colors defined for framebuffers********/
#define FB_BLACK         0  
#define FB_BLUE          1
#define FB_GREEN         2
#define FB_CYAN          3
#define FB_RED           4
#define FB_MAGENTA       5
#define FB_BROWN         6
#define FB_LIGHT_GREY    7
#define FB_DARK_GREY     8
#define FB_LIGHT_BLUE    9
#define FB_LIGHT_GREEN   10
#define FB_LIGHT_CYAN    11
#define FB_LIGHT_RED     12
#define FB_LIGHT_MAGENTA 13
#define FB_LIGHT_BROWN   14
#define FB_WHITE         15
/***********keyboard port**************************/
#define KBD_DATA_PORT 0x60
/***********Function declarations******************/
void fb_write_cell (unsigned int i, char c, unsigned char fg, unsigned char bg);
void fb_move_cursor(unsigned short pos);
void serial_configure_baud_rate(unsigned short com, unsigned short divisor);
void serial_configure_line(unsigned short com);
void serial_configure_buffers(unsigned short com);
void serial_configure_modem(unsigned short com);
int serial_is_transmit_fifo_empty(unsigned short com);
unsigned int count_strlen(char *str);
int fb_write(char *buf, unsigned int len, unsigned char fg, unsigned char bg);                                      //a simple implementation of fb 
                                          //driver for now.
void clear_screen(void);             
void pic_acknowledge(unsigned int interrupt);
unsigned char read_scan_code (void);
/***********Global variables***********************/
char *fb = (char *) 0x000b8000; /*framebuffer pointer, memory-mapped at 0x000b8000*/
unsigned int cur_cursor_pos=0x0;       /*current cursor position*/
unsigned int cur_write_pos =0x0;       /*current write position in fb*/
unsigned int first_boot=0;
struct config_bytes {           /*configuration bytes*/
    unsigned char config;  /*bit 0 - 7*/
    unsigned short address;/*bit 8 - 23*/
    unsigned char index;   /*bit 24 - 31*/
} __attribute__((packed)); /*not C standard, only certain to work with GCC*/
int kmain (unsigned int ebx){
/*clear screen and display welcome message*/
    if(first_boot==0){
        clear_screen();
        char *welcome_msg="              Successfully booted TJos";
        unsigned int welcome_msg_len=count_strlen(welcome_msg);
        fb_write(welcome_msg, welcome_msg_len,FB_BLACK,FB_RED);    
        cur_cursor_pos=0x50;
        fb_move_cursor(cur_cursor_pos);
        cur_write_pos=cur_cursor_pos;
        first_boot++;
    }
    
    multiboot_info_t *mbinfo = (multiboot_info_t *) ebx;
    //check to see if module is loaded successfully
    if(!mbinfo->flags)
    {
       char *mod_load_err_msg = "Error when loading module";
       unsigned int err_msg_len = count_strlen(mod_load_err_msg);
       fb_write(mod_load_err_msg, err_msg_len, FB_BLACK,FB_RED);
       cur_cursor_pos=0xA0;
       fb_move_cursor(cur_cursor_pos);
       cur_write_pos=cur_cursor_pos;
    }
    if(mbinfo->mods_count!=1)
    {
        return 1;
    }
    unsigned int address_of_module = mbinfo->mods_addr;
    typedef void (*call_module_t)(void);
    call_module_t start_program;
    start_program = (call_module_t) address_of_module;
    //start_program();
    while(1);
    
    
    return 0;
}
/** read_scan_code:
  * Reads a scan code from the keyboard
  *
  * @return The scan code(NOT an ASCII character)
  *
  */
unsigned char read_scan_code(void)
{
    return inb(KBD_DATA_PORT);
}
/** pic_acknowledge:
  * Acknowledges an interrupt from either PIC1 or PIC2
  *
  * @param num The number of the interrupt
  */
void pic_acknowledge(unsigned int interrupt)
{
    if(interrupt < PIC1_START_INTERRUPT || interrupt > PIC2_END_INTERRUPT)
        return;
    if(interrupt < PIC2_START_INTERRUPT) {
        outb(PIC1_PORT_A, PIC_ACK);
    } 
    else {
        outb(PIC2_PORT_A, PIC_ACK); 
    }
}
/** clear_screen:
  * clears every cell in fb with space
*/
void clear_screen(void)
{
    int i;
    for(i=0;i<2000;i++)
        fb_write(" ",1,FB_BLACK,FB_BLACK);
    cur_write_pos=0;
    return;
}
/** count_strlen:
  * a function that counts the length of a string
  * @param *str The target string that to be counted
*/
unsigned int count_strlen(char *str)
{  
    unsigned int i=0;
    if(*str=='\0')
        return 0;
    while(*(str++))
        i++;
    return i;
}
/** fb_write:
  * the function that acts like a driver that provides interface for
  * the rest of the OS to interact with framebuffer. It writes a string
  * into framebuffer with the supplied length. It uses the fb_write_cell
  * function. It also moves the cursor to the end of the string.
  * @param *buf the string that to be written
  * @param len  the length of the string
  * @param fg   the foreground color desired for that string
  * @param bg   the background color desired for that string
  */
int fb_write(char *buf, unsigned int len, unsigned char fg, unsigned char bg)
{
    if(*buf=='\0'||len==0||len>=2000)
        return -1;
    unsigned int i=0;
    while(i<len)
    {
        fb_write_cell(cur_write_pos<<1,*(buf+i),fg,bg);
        i++;
        cur_write_pos++;
    }
    return 0;
}
/** fb_write_cell:
 *  writes a char to the given position i in the framebuffer.
 *  remember framebuffer is a 25x80 buffer.
 *  colors are pre-defined at the top.
 *  @param i  the postiion in the framebuffer
 *  @param c  the ASCII encoding of that char
 *  @param fg the foreground color
 *  @param bg the background color
*/
void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg)
{
    fb[i] = c;
    fb[i+1] = ((fg & 0x0f) << 4) | (bg & 0x0f);
} 
/** fb_move_cursor:
  * moves the cursor in framebuffer to a given position
  * @param pos the new position of the cursor
*/
void fb_move_cursor(unsigned short pos)
{
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT,    ((pos>>8) & 0x00ff));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT,    pos & 0x00ff);
}
/** serial_configure_baud_rate:
 * sets the speed of the data being sent. The default speed of the serial 
 * port is 115200 bits/s. The argument is a divisor of that number, 
 * thus the resulting rate is (115200/param) bits/s.
 * @param com      The COM port to configure
 * @param divisor  The divisor
*/
void serial_configure_baud_rate(unsigned short com, unsigned short divisor)
{
    outb(SERIAL_LINE_COMMAND_PORT(com),
         SERIAL_LINE_ENABLE_DLAB);
    outb(SERIAL_DATA_PORT(com),
         (divisor >> 8) & 0x00FF);
    outb(SERIAL_DATA_PORT(com),
         divisor & 0x00FF);
}
/** serial_configure_line:
  * Configures the line of the given serial port. 
  * The port is set to have a length of 8 bits, no parity bits,
  * one stop bit and break control disabled.
  * @param com The serial port to configure
  */
void serial_configure_line(unsigned short com)
{
    /*Bit:    | 7 | 6 | 5 4 3 | 2 | 1 0 |
     *content | d | b | prty  | s | dl  |
     *value   | 0 | 0 | 0 0 0 | 0 | 1 1 | = 0x03
     */
     outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}
/** serial_configure_buffers:
  * Configures the receiving and transmitting buffers of the given 
  * serial port. 
  * The buffer is set to be enabled, clear both receiver and transmitter 
  * FIFO queue, use 14 bytes as size of queue
  * @param com The serial port to configure
  */
void serial_configure_buffers(unsigned short com)
{
    /*Bit:    | 7 6 | 5 | 4 | 3 | 2 | 1 | 0 |
     *content | lvl | bs| r |dma|clt|clr| e |
     *value   | 1 1 | 0 | 0 | 0 | 1 | 1 | 1 | = 0xc7
     */
     outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}
/** serial_configure_modem:
  * Configures the modem control register whose flow control is via 
  * Read to Transmit (RTS) and Data Terminal Ready (DTR) pins
  * We need those two to be 1 when configuring the port 
  * All the others can be set to 0
  * @param com The serial port to configure
  */
void serial_configure_modem(unsigned short com)
{
     outb(SERIAL_MODEM_COMMAND_PORT(com), 0x03);
}
/** serial_is_transmit_fifo_empty:
  * checks whether the transmit FIFO queue is empty or not for the 
  * given COM port. It's empty if bit 5 of status is 1.
  * @param com The COM port
  * @return 0 if the transmit FIFO queue is not empty
  *         1 if the transmit FIFO queue is empty
  */
int serial_is_transmit_fifo_empty(unsigned short com)
{
     /* 0x20 = 0010 0000 */
     return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}



