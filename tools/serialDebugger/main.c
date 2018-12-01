/*
* Code from: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_interface_attribs(int fd, int speed)
{
   struct termios tty;

   if (tcgetattr(fd, &tty) < 0)
   {
      printf("Error from tcgetattr: %s\n", strerror(errno));
      return -1;
   }

   cfsetospeed(&tty, (speed_t)speed);
   cfsetispeed(&tty, (speed_t)speed);

   tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
   tty.c_cflag &= ~CSIZE;
   tty.c_cflag |= CS8;      /* 8-bit characters */
   tty.c_cflag &= ~PARENB;  /* no parity bit */
   tty.c_cflag &= ~CSTOPB;  /* only need 1 stop bit */
   tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

   /* setup for non-canonical mode */
   tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
   tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
   tty.c_oflag &= ~OPOST;

   /* fetch bytes as they become available */
   tty.c_cc[VMIN] = 1;
   tty.c_cc[VTIME] = 1;

   if (tcsetattr(fd, TCSANOW, &tty) != 0)
   {
      printf("Error from tcsetattr: %s\n", strerror(errno));
      return -1;
   }
   return 0;
}

int main()
{
   printf("#----------------------------------#\n");
   printf("CactusOS Serial debugger Application\n");
   printf("#----------------------------------#\n");

   char* portname = "/dev/ttyUSB0";
   int fd;
   int wlen;

   fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
   if (fd < 0)
   {
      printf("Error opening %s: %s\n", portname, strerror(errno));
      return -1;
   }

   set_interface_attribs(fd, B38400);

   do
   {
      unsigned char buf[80];
      int rdlen;

      rdlen = read(fd, buf, sizeof(buf) - 1);
      if (rdlen > 0)
      {
         buf[rdlen] = 0;
         printf("%s", buf);
      }
      else if (rdlen < 0)
      {
         printf("Error from read: %d: %s\n", rdlen, strerror(errno));
      }
      /* repeat read to get full message */
   } while (1);
}