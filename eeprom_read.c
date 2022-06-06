#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

char *device = "/sys/bus/i2c/devices/3-0050/eeprom";
int fd = -1;

int i2c_eeprom_open(void)
{
    fd = open(device,O_RDWR);
    if(fd < 0)
    {
        printf("open eeprom err \n");
        return fd;
    }else {
        return fd;
    }
}

void i2c_eeprom_close(void)
{
    close(fd);
}


int i2c_eeprom_write(const char *data, int start, int len)
{
    int ret;
    if(data == NULL)
        return -1;

    if(i2c_eeprom_open() < 0)
        return -1;
    lseek(fd, start, SEEK_SET);
    ret = write(fd, data, len);
    if(ret > 0)
        printf("write %d data to eeprom\n", ret);

    i2c_eeprom_close();
    return ret;
}

/*eg: HMYD-YT507-xxx*/
/*
*start address
* mac0 0x50
* sn 0x30
* PN 0x10
* the first char is defined len
*/
int main(int argc, char *argv[])
{
    int ret , leng;
    char len[1] = "0";
    char pn[64] = {'\0'}, sn[64] = {'\0'};
    char mac0[16] = {'0'};
    char mac1[16] = {'0'};
    
   printf("Module EEPROM\n");
   //read PN
   if(i2c_eeprom_open() < 0)
        return -1;
   lseek(fd, 0x10, SEEK_SET);
   ret = read(fd, len, 1);
   if (ret != 0)
   {
	leng = len[0] - '0';	
   }
   else
	return -1;

   ret = read(fd, pn, leng);
   if (ret == 0)
   {
	printf("read pn err!\n");
	return -1;	
   }
   
   lseek(fd, 0x50, SEEK_SET);
   ret = read(fd, len, 1);
   if (ret != 0)
   {
	leng = len[0] - '0';	
   }else
	return -1;
   ret = read(fd, sn, leng);
   if (ret == 0)
   {
	printf("read sn err!\n");
	return -1;	
   }
    printf("PN: %s\n", pn);
    printf("SN: %s\n", sn);
   i2c_eeprom_close();
   return 0;
}





