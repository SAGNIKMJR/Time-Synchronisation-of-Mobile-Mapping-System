
 /* This example uses the RiVLib as a DLL, so do not forget to copy
    the scanifc.dll to a place where it can be found ( usually at the
    same location as the executable when on windows). */

#include <riegl/scanifc.h>
#include "/home/user/rivlib-2_3_0-x86_64-linux-gcc44/gps_sync_header.hpp"
#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;

void print_last_error()
{
    char buffer[512];
    scanifc_uint32_t bs;
    scanifc_get_last_error(buffer,512,&bs); // actual size of error -> *bs , 512 size of buffer .. error truncated to 512 if longer than 512
    printf("%s\n",buffer);
}

#define N_TARG 1000000 // no of points per call

scanifc_xyz32       rg_xyz32[N_TARG];
scanifc_attributes  rg_attributes[N_TARG];
scanifc_time_ns     rg_time[N_TARG];

int main(int argc,char* argv[]) //argv[1] uri of the rxp stream
{
    point3dstream_handle h3ds=0; //handle for data stream
    int sync_to_pps=1; // since reading data with pps timestamp...later changed to 0
    scanifc_uint32_t count;   // type uint32_t
    scanifc_uint32_t n;
    int end_of_frame;
    unsigned long int numbr_of_points=1;
    int flag=1;
    //scanifc_xyz32       rg_xyz32[N_TARG];     //scanifc_xyz32 -> structure having x,y,z coordinates
    //scanifc_attributes  rg_attributes[N_TARG]; //scanifc_attributes-> structure having attributes like amolitude,deviation,reflectance
    //scanifc_time_ns     rg_time[N_TARG];      // timestamp havin exrernal gps since sync_to_pps=1

    while(flag==1)
    {

        flag=gps_sync(argc,argv);
        if(flag==0)
            break;
    }
    if (argc==4)
        {

            if (scanifc_point3dstream_open_with_logging(argv[1],sync_to_pps,"log.rxp",&h3ds))
                {  // !=0 -> failure...
                    fprintf(stderr,"Cannot open: %s\n",argv[1]);
                    print_last_error();
                    return 1;
                }
            else
                {

                    if (scanifc_point3dstream_add_demultiplexer(h3ds,"hk.txt",0,"status protocol")) //no package names, class names - 'status','protocol'
                        {
                            print_last_error();
                            return 1;
                        }
                    string p(argv[1]);
                    int i = p.find_last_of(".");
                    p = p.substr(0,i);
                    p = p + ".txt";
                    FILE *f=fopen(p.c_str(),"w");
                    do
                        {
                            if (scanifc_point3dstream_read(h3ds,N_TARG,rg_xyz32,rg_attributes,rg_time,&count,&end_of_frame))
                                {
                                    fprintf(stderr,"Error reading %s\n",argv[1]);      //count -> no of points actually read returned in count
                                    return 1;
                                }
                            for (n=0;n<count;++n)
                                {
                                    fprintf(f,"%f,%f,%f,%u,%lu,%d\n",rg_xyz32[n].x,rg_xyz32[n].y,rg_xyz32[n].z,(rg_attributes[n].flags & 0x40)>>6,rg_time[n],end_of_frame);
                                    //initially llu changes to lu
                                }
                                numbr_of_points+=n;
                       } while(count >0); // reads 'count' no of points

                    fprintf(f,"The number of points obtained from rxp :%lu",numbr_of_points); //writing the number of points to the file

                    if (scanifc_point3dstream_close(h3ds)) //closing the file handle
                        {
                            fprintf(stderr,"Error,closing %s\n",argv[1]);
                            return 1;
                        }
                        fclose(f);
                    return 0;
                }
        }

    fprintf(stderr,"Usage:%s <input-uri>\n",argv[0]);
    return 1;
}
