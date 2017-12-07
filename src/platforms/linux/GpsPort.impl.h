#pragma once

#include "GpsPort.header.h"

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

GpsPort::GpsPort(const char* usbDev, const char* speedTxt)
{
    if (usbDev == nullptr) {
      usbDev = "/dev/ttyUSB0";
    }
    
    int fd = open(usbDev, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
      printf("Error opening %s: %s\n", usbDev, strerror(errno));
      _device = -1;
    }
    
    int speed;
    switch(atoi(speedTxt == nullptr ? "9600" : speedTxt)) {
      case 38400: 
        speed = B38400;
        break;
      case 19200: 
        speed = B19200;
        break;
      default:
      case 9600: 
        speed = B9600;
        break;
    }
    /*baudrate 9600, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(fd, speed);
    
    _device = fd;
}

bool GpsPort::available()
{
  fd_set rfds;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  
  FD_ZERO( &rfds );
  FD_SET( _device, &rfds );
  
  int retval = select( 1, &rfds, NULL, NULL, &tv );
  if (retval == -1) {
    printf("Error select: %s\n", strerror(errno));
  }
  return retval > 0;
}

char GpsPort::read()
{
  char c;
  int rdlen = ::read( _device, &c, 1 );
  if (rdlen > 0) {
    return c;
  } else if (rdlen < 0) {
    printf("Error from read: %s\n", strerror(errno));
  }
  return '\0';
}

void GpsPort::print(char c)
{
  /* simple output */
  int wlen = ::write(_device, &c, 1);
  if (wlen != 1) {
    printf("Error from write: %d, %d\n", wlen, errno);
  }
  tcdrain(_device);    /* delay for output */
}

int GpsPort::set_interface_attribs(int fd, int speed)
{
  struct termios tty;
  
  if (tcgetattr(fd, &tty) < 0) {
    printf("Error from tcgetattr: %s\n", strerror(errno));
    return -1;
  }
  
  cfsetospeed(&tty, (speed_t)speed);
  cfsetispeed(&tty, (speed_t)speed);
  
  tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;         /* 8-bit characters */
  tty.c_cflag &= ~PARENB;     /* no parity bit */
  tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
  tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */
  
  /* setup for non-canonical mode */
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_oflag &= ~OPOST;
  
  /* fetch bytes as they become available */
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 1;
  
  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    printf("Error from tcsetattr: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}
