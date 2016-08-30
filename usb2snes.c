/* usb2snes.c
   sd2snes USB-tool
   by saturnu
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     /* abs */
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include "gopt.h"
#include "usb2snes.h"


#ifdef __unix__
//unix
    #include <unistd.h>
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #include <termios.h>
#elif defined _WIN32 || defined _WIN64
//windows
    #include <Windows.h>
    #include "rs232.h"
#else
#error "unknown platform"
#endif

void *options;
const char *device;
int port=0;
char boot_enable=1;

#ifdef __unix__
static int fd;
#endif

#ifdef __unix__
void init_portio(void){

        struct termios toptions;


        tcgetattr(fd, &toptions);
        cfsetispeed(&toptions, B9600);
        cfsetospeed(&toptions, B9600);

        toptions.c_lflag = 0;
        toptions.c_iflag = 0;
        toptions.c_oflag = 0;

        toptions.c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE);
        toptions.c_iflag &= ~(BRKINT | ICRNL | IMAXBEL);
        toptions.c_oflag &= ~(OPOST | ONLCR);
        toptions.c_cc[VMIN] = 64;
        toptions.c_cc[VTIME] = 5;

        tcsetattr(fd, TCSANOW, &toptions);


}
#endif


/* Check for regular file. */
int check_reg(const char *path) {
        struct stat sb;
        return stat(path, &sb) == 0 && S_ISREG(sb.st_mode);
}

/* Check for char device file. */
int check_chr(const char *path) {
        struct stat sb;
        return stat(path, &sb) == 0 && S_ISCHR(sb.st_mode);
}


int usb_read(unsigned char *data, int size) {

        int count;
        int count_temp;
        int ret=0;

  #ifdef __unix__
        //unixint count;
        /*
           while(count<64){
           ioctl(fd, FIONREAD, &count);

           if(count_temp!=count)
           printf("new count: %d\n",count);
           count_temp=count;
           }*/

        ret=read(fd, data, size);
        //printf("usb_read: ret-%d count-\n",ret,count);
  #elif defined _WIN32 || defined _WIN64
        //windows
        ret=RS232_PollComport(port, data, size);
  #else
  #error "unknown platform"
  #endif


        if (ret==-1) {
                printf("device: read error\n");
                return -1;
        }

        return ret;
}


int open_port(void){



        if( gopt_arg( options, 'd', &device ) && strcmp( device, "-" ) ) {



  #ifdef __unix__
                //unix

                if( check_chr(device) != 1) {
                        printf("character device file does not exist!\n");
                        return -1;
                }else{
                        fd=open(device,  O_RDWR | O_NOCTTY);
                        if (fd==-1) {
                                printf("device: file open error\n");
                        }
                }

  #elif defined _WIN32 || defined _WIN64
                //windows

                if(device[0]>0x30 && device[0]<0x40) {
                        port=device[0]-0x30-0x01;
                }else{
                        printf("wrong COM-Port number\n");
                        return -1;
                }

                int i=0,
                    cport_nr=port, /* /dev/ttyS0 (COM1 on windows) */
                    bdrate=9600; /* 9600 baud */

                char mode[]={'8','N','1',0};

                if(RS232_OpenComport(cport_nr, bdrate, mode))
                {
                        printf("Can not open comport\n");

                        return(0);
                }
  #else
  #error "unknown platform"
  #endif

        }else{
                printf("no port specified\n");
                return 0;
        }

#ifdef __unix__
        return fd;
#endif
        return 0;
}



int usb_write(unsigned char *data, int size) {

        int ret=0;

  #ifdef __unix__
        //unix
        ret=write(fd, data, size);
  #elif defined _WIN32 || defined _WIN64
        //windows
        ret=RS232_SendBuf(port, data, size);
  #else
  #error "unknown platform"
  #endif


        if (ret==-1) {
                printf("device: write error\n");
                return -1;
        }

        return ret;
}




int main( int argc, const char **argv )
{

        //usb buffer
        unsigned char send_buff[512];
        unsigned char recv_buff[512];

        int arg_fail=1;

        const char *filename;
        const char *offset;

        //verbosity level 0-2
        int verbosity;
        int i,print_help;

        options = gopt_sort( &argc, argv, gopt_start(
                                     gopt_option( 'h', 0, gopt_shorts( 'h' ), gopt_longs( "help" )),
                                     gopt_option( 'z', 0, gopt_shorts( 'z' ), gopt_longs( "version" )),
                                     gopt_option( 'v', GOPT_REPEAT, gopt_shorts( 'v' ), gopt_longs( "verbose" )),
                                      //todo read support
                                     gopt_option( 'r', 0, gopt_shorts( 'r' ), gopt_longs( "read" )),
                                     gopt_option( 'u', 0, gopt_shorts( 'u' ), gopt_longs( "unlock" )),
                                     gopt_option( 'l', 0, gopt_shorts( 'l' ), gopt_longs( "lock" )),
                                     gopt_option( 'k', 0, gopt_shorts( 'k' ), gopt_longs( "skipheader" )),
                                     gopt_option( 'e', 0, gopt_shorts( 'e' ), gopt_longs( "speedup" )),
                                     gopt_option( 't', 0, gopt_shorts( 't' ), gopt_longs( "settime" )),

                                     gopt_option( 'c', 0, gopt_shorts( 'c' ), gopt_longs( "no-fpga-reconf" )),
                                     gopt_option( 'p', 0, gopt_shorts( 'p' ), gopt_longs( "no-dsp" )),

                                     gopt_option( 'g', 0, gopt_shorts( 'g' ), gopt_longs( "disable-d4p" )),
                                     gopt_option( 'w', 0, gopt_shorts( 'w' ), gopt_longs( "write" )),
                                     gopt_option( 'm', 0, gopt_shorts( 'm' ), gopt_longs( "rom" )),
                                     gopt_option( 's', 0, gopt_shorts( 's' ), gopt_longs( "sram" )),
                                     gopt_option( 'b', 0, gopt_shorts( 'b' ), gopt_longs( "boot" )),
                                      //todo offset support
                                      //gopt_option( 'o', GOPT_ARG, gopt_shorts( 'o' ), gopt_longs( "offset" )),
                                     gopt_option( 'd', GOPT_ARG, gopt_shorts( 'd' ), gopt_longs( "device" )),
                                     gopt_option( 'f', GOPT_ARG, gopt_shorts( 'f' ), gopt_longs( "file" ))  ));




        if( gopt( options, 'h' ) ) {

  #ifdef __unix__
                //unix
                fprintf( stdout, "Syntax: sudo ./usb2snes -d \\dev\\ttyACMx [options] ...\n\n");
  #elif defined _WIN32 || defined _WIN64
                //windows
                fprintf( stdout, "Syntax: sudo ./usb2snes -d X [options] ...\n\n");
  #else
  #error "unknown platform"
  #endif


                fprintf( stdout, "usb2snes - sd2snes USB-tool\n" );
                fprintf( stdout, "by saturnu <tt@anpa.nl>\n\n" );




  #ifdef __unix__
                //unix
                printf("Device: (required)\n");
                fprintf( stdout, " -d, --device=name\tttyACMx device file\n" );
  #elif defined _WIN32 || defined _WIN64
                //windows
                printf("Device port number: (required)\n");
                fprintf( stdout, " -d, --device=X\t\COM[X] port to use (required)\n" );
  #else
  #error "unknown platform"
  #endif

                printf("\nCommands:\n");
                fprintf( stdout, " -w, --write\t\twrite to sdram (default rom offset) - used with --file\n" );
                fprintf( stdout, " -b, --boot\t\tboot rom - used after writing sram/rom\n" );
                fprintf( stdout, " -u, --unlock\t\tunlock usb_handler (optional)\n" );
                fprintf( stdout, " -l, --lock\t\tlock usb_handler (optional)\n" );
                fprintf( stdout, " -t, --settime\t\tset localtime (optional)\n" );

                printf("\nOptions:\n");
                fprintf( stdout, " -m, --rom\t\tset rom offset (default)\n" );
                fprintf( stdout, " -s, --sram\t\tset sram offset - used with --write\n" );


                //todo offset support
                //    fprintf( stdout, " -o, --offset=0x0000\tmemory offset (optional)\n" );
                //todo read support
                fprintf( stdout, " -r, --read\t\tread from sdram (default sram offset)- used with --file\n" );
                fprintf( stdout, " -g, --disable-d4p\tdisable $213f-D4-region-patching (used with --boot)\n" );
                fprintf( stdout, " -f, --file=filename\trom or sram image\n" );
                fprintf( stdout, " -c, ---no-fpga-reconf\tdisable fpga special chip reconfiguration\n" );
                fprintf( stdout, " -p, ---no-dsp\tdisable dsp support\n" );

                fprintf( stdout, " -k, --skipheader\tskip backup unit header (512 bytes)\n" );
                fprintf( stdout, " -e, --speedup\t\tspeedup - used only in conjuction with --write (optional)\n" );

                printf("\nInformation:\n");
                fprintf( stdout, " -h, --help\t\tdisplay this help and exit\n" );
                fprintf( stdout, " -v, --verbose\t\tverbose\n" );
                fprintf( stdout, " -z, --version\t\tversion info\n" );

                return 0;
        }

        if( gopt( options, 'z' ) ) {

                fprintf( stdout, "usb2snes version v%d.%d\n", MAJOR_VERSION, MINOR_VERSION );
                return 0;
        }

        if( !gopt( options, 'd' ) ) {
                //needed every time
                printf("device option missing!\n");
                return 0;
        }


        //show info without options
        print_help=1;

        if(   gopt( options, 'u') || gopt( options, 'l') || gopt( options, 'b' ) || gopt( options, 'w' ) ||
              gopt( options, 'r') || gopt( options, 'z') || gopt( options, 'h') || gopt( options, 't') ) {

                print_help = 0;

                if( gopt( options, 't' ) || gopt( options, 'u' ) || gopt( options, 'l' ) || gopt( options, 'b' ) || gopt( options, 'w' ) || gopt( options, 'r' ))
                        arg_fail = 0;
        }



        if( gopt( options, 'w' ) && gopt( options, 'r' )) {

                fprintf( stdout, "error: could not read and write at the same time\n" );
                return 0;
        }

        if( (gopt( options, 'u' ) && gopt( options, 'b' )) || (gopt( options, 'l' ) && gopt( options, 'b' ))
            || (gopt( options, 't' ) && gopt( options, 'b' )) || (gopt( options, 'r' ) && gopt( options, 'b' ))  ) {

                fprintf( stdout, "error: use boot with write or separately\n" );
                return 0;
        }

        if( gopt( options, 'm' ) && gopt( options, 's' )) {

                fprintf( stdout, "error: use sram and rom offset separately\n" );
                return 0;
        }

        if( gopt( options, 'u' ) && gopt( options, 'l' )) {

                fprintf( stdout, "error: use lock and unlock separately\n" );
                return 0;
        }

        if( gopt( options, 'k' ) && gopt( options, 's' )) {

                fprintf( stdout, "error: use sram and skip separately\n" );
                return 0;
        }


        verbosity = gopt( options, 'v' );


        if( verbosity > 1 )
                fprintf( stderr, "being really verbose\n" );

        else if( verbosity )
                fprintf( stderr, "being verbose\n" );


        if(!arg_fail) {

                //open port
                if( open_port() == -1) {
                        //error
                        return 0;
                }

                //init port settings in linux
  #ifdef __unix__
                init_portio();
  #endif

                int ret;

                //init usb transfer buffer
                memset(send_buff, 0, 512);
                memset(recv_buff, 0, 512);

                int ret_s = 0;
                int ret_r = 0;

                memset(send_buff, 0, 512);
                memset(recv_buff, 0, 512);
                ret_s = 0;
                ret_r = 0;

                if( gopt( options, 't' ) ) {

                        memset(send_buff, 0, 512);
                        send_buff[0]='U';
                        send_buff[1]='S';
                        send_buff[2]='B';
                        send_buff[3]='T'; //time

                        time_t rawtime;
                        struct tm * timeinfo;

                        rawtime = time(NULL);
                        timeinfo = localtime ( &rawtime );

                        printf ( "Current local time and date: %s", asctime (timeinfo) );

                        rawtime = time(NULL) - 1; //1 sec correction;
                        timeinfo = localtime ( &rawtime );


                        send_buff[4]= timeinfo->tm_sec;
                        send_buff[5]= timeinfo->tm_min;
                        send_buff[6]= timeinfo->tm_hour;
                        send_buff[7]= timeinfo->tm_mday;
                        send_buff[8]= timeinfo->tm_mon+1;
                        send_buff[9] = (unsigned char) (timeinfo->tm_year+1900 >> 8);
                        send_buff[10] = (unsigned char) (timeinfo->tm_year+1900 );
                        send_buff[11]= timeinfo->tm_wday;


                        if(verbosity >= 1)
                                printf("send: %i bytes\n",ret_s);

                        ret_s = usb_write(send_buff,512);

                        printf("set time instructed...\n");

                        return 0;

                }else
                if( gopt( options, 'u' ) ) {

                        memset(send_buff, 0, 512);
                        send_buff[0]='U';
                        send_buff[1]='S';
                        send_buff[2]='B';
                        send_buff[3]='U'; //unlock

                        if(verbosity >= 1)
                                printf("send: %i bytes\n",ret_s);

                        ret_s = usb_write(send_buff,512);

                        printf("unlock instructed...\n");

                }else
                if( gopt( options, 'l' ) ) {

                        memset(send_buff, 0, 512);
                        send_buff[0]='U';
                        send_buff[1]='S';
                        send_buff[2]='B';
                        send_buff[3]='L'; //lock

                        if(verbosity >= 1)
                                printf("send: %i bytes\n",ret_s);

                        ret_s = usb_write(send_buff,512);

                        printf("lock instructed...\n");

                }else

                if( gopt( options, 'r' ) ) {

                        if( gopt_arg( options, 'f', &filename ) && strcmp( filename, "-" ) ) {

                                FILE *fp;
                                fp=fopen(filename, "w+");
                                int fsize;
                                /*
                                     int fo=open(device,  O_RDONLY );
                                      if (fo==-1) {
                                    printf("device: file fo open error\n");
                                    }
                                 */
                                memset(send_buff, 0, 512);
                                send_buff[0]='U';
                                send_buff[1]='S';
                                send_buff[2]='B';
                                send_buff[3]='R'; //read

                                //default ram mode
                                send_buff[4]=0x01;

                                //set to rom mode
                                if ( gopt( options, 'm' ) )
                                        send_buff[4]=0x00;


                                //speed up
                                if ( gopt( options, 'e' ) )
                                        send_buff[5]=0x01;

                                printf("send usb request...\n");

                                ret_s = usb_write(send_buff, 512);

                                printf("receiving mode...\n");

                                //first package
                                char buffer[FILE_CHUNK];
                                //set
                                memset(buffer, 0, FILE_CHUNK);

                                /*
                                   ret_read=128 sum=129
                                   ret_read=128 sum=257
                                   ret_read=128 sum=385
                                   ret_read=128 sum=513

                                   header ret_read 128
                                 */

                                int sumx=0;
                                int ret_read=0;
                                int data_chunk=0;

                                while(data_chunk<512) {

                                        ret_read = usb_read(buffer+data_chunk,512-data_chunk);
                                        sumx+=ret_read;
                                        printf("ret_read=%d sum=%d\n",ret_read,sumx);

                                        if(ret_read!=-1) {
                                                data_chunk+=ret_read;
                                        }else{
                                                printf("not enough usb data\n");
                                                return 0;
                                        }
                                }

                                if(verbosity >= 2)
                                        printf("fs pkg %d\n", sumx);

                                int data_blocks=0;

                                int datablocks = (int) ((buffer[3] << 8) + buffer[4]);
                                unsigned int usb_filesize = abs(datablocks*512);

                                //usb_filesize=2048*512;
                                if(verbosity >= 1)
                                        printf("usb_filesize: %d\n", usb_filesize);


                                memset(buffer, 0, FILE_CHUNK);

                                //reuse
                                data_blocks=0;

                                int sum=0;
                                char full_buffer[usb_filesize];



                                //fullbuffer file first and write at once ^^  ram on pc rocks :D
                                for(data_blocks = 0; data_blocks < usb_filesize; data_blocks += 0x200) {

                                        //read parts to memory

                                        //size_t fwrite(const void *ptr, size_t size_of_elements, size_t number_of_elements, FILE *a_file);

                                        data_chunk=0;
                                        memset(buffer, 0, 0x200);
                                        while(data_chunk<512) {

                                                memset(buffer, 0, ret_read);
                                                usleep(3500);
                                                ret_read = usb_read(buffer,512-data_chunk);
                                                //ret_read = read(fo, buffer, FILE_CHUNK);

                                                sum+=ret_read;
                                                if(verbosity >= 2)
                                                        printf("data ret_read %d,  sum=%d\n", ret_read, sum);

                                                memcpy(full_buffer+data_blocks+data_chunk, buffer, 0x200);


                                                if(ret_read!=-1) {
                                                        data_chunk+=ret_read;
                                                }else{
                                                        printf("not enough usb data\n");
                                                        return 0;
                                                }
                                        }


                                        //if(verbosity >= 2)
                                        //printf("data ret_read %d\n", ret_read);



                                }

                                int fret = fwrite(full_buffer, sizeof(full_buffer[0]), sizeof(full_buffer), fp);

                                fclose(fp);

                        }else{
                                printf("filename is missing\n");
                        }

                }else
                if( gopt( options, 'w' ) ) {

                        int data_blocks=0;

                        if( gopt_arg( options, 'f', &filename ) && strcmp( filename, "-" ) ) {

                                FILE *fp;

                                if( check_reg(filename) != 1) {
                                        printf("file does not exist!\n");
                                        return 0;
                                }


                                fp=fopen(filename, "rb");
                                int fsize;


                                struct stat st;
                                stat(filename, &st);
                                fsize = st.st_size;


                                int length = (int) fsize;

                                if(verbosity >= 1)
                                        printf("length: %d\n",length);


                                if (((length / 0x200) * 0x200) != fsize) {
                                        length = (int) (((fsize / 0x200) * 0x200) + 0x200);
                                }

                                //bleed block
                                //length+=0x200;

                                if(verbosity >= 2)
                                        printf("length_buffer: %d\n",length);

                                //FILE_CHUNK default 0x8000
                                char buffer[FILE_CHUNK];
                                memset(buffer, 0, FILE_CHUNK);

                                memset(send_buff, 0, 512);

                                send_buff[0]='U';
                                send_buff[1]='S';
                                send_buff[2]='B';
                                send_buff[3]='W'; //write

                                //offset setting
                                //if( gopt_arg( options, 'o', & offset ) && strcmp( offset, "-" ) ){

                                send_buff[4]=0x00; //offset
                                send_buff[5]=0x00; //offset


                                if ( gopt( options, 'k' ) && !gopt( options, 's' )) {
                                        printf("skipping 512bytes...\n");
                                        length -= 0x200;
                                }


                                send_buff[6] = (char) ((( length) / 0x200) >> 8); //length
                                send_buff[7] = (char) (length / 0x200); //length

                                if ( gopt( options, 'e' ) )
                                        send_buff[8]=0x01;

                                //sdram rom default
                                send_buff[9]=0x00;

                                //#define SRAM_SAVE_ADDR          (0xE00000L)
                                if ( gopt( options, 's' ) )
                                        send_buff[9]=0x01;

                                send_buff[11]=0x01; //fpga reconf enable
                                send_buff[12]=0x01; //dsp enable

                                //disable fpga reconf
                                if ( gopt( options, 'c' ) && !gopt( options, 's' )) {
                                        send_buff[11]=0x00;
                                }

                                //disable dsp
                                if ( gopt( options, 'p' ) && !gopt( options, 's' )) {
                                        send_buff[12]=0x00;
                                }


                                send_buff[10]=0x01; //filename ok
                                //filename 255-511
                                //should be more than enough
                                int flength = snprintf(send_buff+255, 256, "%s", basename((char*)filename));

                                if(verbosity >= 2) {
                                        if(flength > 255)
                                                printf("info: filename too long - disable features");
                                        send_buff[10]=0x00; //disable filename
                                }


                                if(verbosity >= 2)
                                        printf("send filename: %s\n",send_buff+255);

                                ret_s = usb_write(send_buff, 512);



                                if(verbosity >= 1)
                                        printf("send write cmd: %i bytes\n",ret_s);


                                usleep(1000*100);

                                if ( gopt( options, 'k' ) && !gopt( options, 's' )) {
                                        printf("seeking 512bytes...\n");
                                        fseek(fp, 0x200, SEEK_SET);
                                }

                                printf("sending...\n");
                                int s;
                                for(s = 0; s < length; s += 0x200) {

                                        //read parts to memory
                                        memset(buffer, 0, FILE_CHUNK);

                                        int fret = fread(buffer, sizeof(buffer[0]), FILE_CHUNK, fp);


                                        usb_write(buffer,FILE_CHUNK);
                                        data_blocks++;


                                }

                                //send bleed block
                                //memset(buffer, 0, FILE_CHUNK);
                                //usb_write(buffer,FILE_CHUNK);


                                if(verbosity >= 1)
                                        printf("data blocks written: %d\n",data_blocks);
                                printf("upload done...\n",  argv[0]);
                                if( !gopt( options, 's' ) )
                                        printf("ready to boot now...\n");
                                fclose(fp);

                        }else{
                                printf("filename is missing\n");

                                //single line load&boot disable
                                boot_enable=0;
                        }


                }

                //boot
                if( gopt( options, 'b' ) && boot_enable) {

                        memset(send_buff, 0, 512);
                        send_buff[0]='U';
                        send_buff[1]='S';
                        send_buff[2]='B';
                        send_buff[3]='B'; //boot

                        //disable d4 patching
                        if ( gopt( options, 'g' ) )
                                send_buff[4]=0x01;


                        if(verbosity >= 1)
                                printf("send: %i bytes\n",ret_s);

                        ret_s = usb_write(send_buff,512);

                        printf("boot instructed...\n");

                }


#ifdef __unix__
                //unix
                close(fd);
#elif defined _WIN32 || defined _WIN64
                //windows
                RS232_CloseComport(4);
#else
#error "unknown platform"
#endif




        }

        if(print_help) {

                printf("%s: missing operand\n",  argv[0]);
                printf("Try '%s --help' for more information.\n",  argv[0]);
        }


        gopt_free( options );

        return 0;
}
