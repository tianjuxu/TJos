#ifndef INCLUDE_IO_H
#define INCLUDE_IO_H

/** outb:
  * Sends the given data to the given IO port. Defined in io.s 
  * @param port - the target IO port
  * @param data - data to be sent
  */
    void outb(unsigned short port, unsigned char data);
/** inb:
  * read a byte from the IO port
  * @param port The addr of IO port
  * @return     the read byte
*/
    unsigned char inb(unsigned short port);

#endif 
