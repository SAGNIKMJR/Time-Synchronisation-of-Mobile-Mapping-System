
#include <riegl/ctrllib.hpp>
#include <riegl/scanlib.hpp>

#include <iostream>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <memory>

using namespace ctrllib;
using namespace scanlib;
using namespace std;

int main(int argc,char* argv[]) //3rd argument , argv[2] -> output filename ;argv[0] -> tls s no / ip addrs ; argv[1] -> port no of external gps
{

    if ((argc!=3)||(string(argv[1])=="--help"))
    {
        cerr<<"Usage:"<<argv[0]<<" address[:service] outputfile"<<endl;
        return 1;
    }

    ctrl_client_session session;

    try
    {
        // now connect to the instrument specified as first argument
        // the connection string is expected in format
        //   "<host_name_or_ip_address>[:<service_name_or_port_number>]"
        // Examples:
        //   "192.168.0.234", "192.168.0.234:20002",
        //   "s9991234" or "s9991234:20002"
        // if port number is not specified, 20002 is assumed as default

        session.open(argv[1]);
        cout<<"Connected to "<<argv[1]<<endl;

        string result;
        vector<string> comment;
        string status;

        session.execute_command("MEAS_ABORT","",&result,&status,&comment);// telling the TLS to abort the current session
        cout<<"MEAS_ABORT() returned "<<result<<endl;
        for(int i=0;i<comment.size();i++) //for(int i=0 ; i<comment.size();i++)
            cout<<comment[i]<<endl;
        cout<<"Status is "<<status<<endl;

        session.execute_command("MEAS_BUSY","1",&result,&status,&comment);
        cout<<"MEAS_BUSY(1) returned "<<result<<endl;//waiting for the TLS to terminate the previous session
        for(int i=0;i<comment.size();i++)//for(size_t i=0; i<comment.size(); ++i)
            cout<<comment[i]<<endl;
        cout<<"Status is "<<status<<endl;

        string value;
        session.get_property("INST_IDENT",value);
        cout<<"Instrument is "<<value<<endl;

        //
        if (std::string(value).compare(1,6,"VZ-400")!=0)
            {
                    throw runtime_error("Error:VZ-400 instrument required");
            }

        if (std::string(value).compare(1,6,"VZ-400")==0)//if (std::string(value).compare(1, 6, "VQ-250") == 0)
        {
            session.set_property("GPS_MODE","2");
            cout<<"GPS_MODE set to 2 (external GPS via RS232)"<<endl;

            session.set_property("GPS_EXT_COM_BAUDRATE","6");
            cout<<"GPS_EXT_COM_BAUDRATE set to 115200"<<endl;

            session.set_property("GPS_EXT_EDGE","0");
            cout<<"GPS_EXT_EDGE set to (0=positive,1=negative)"<<endl;

            session.set_property("GPS_EXT_FORMAT", "\"GPZDA\"");
            cout<<"GPS_EXT_FORMAT set to GPZDA"<<endl;

            session.set_property("GPS_EXT_SEQUENCE", "\"PPS_FIRST\"");
            cout<<"GPS_EXT_SEQUENCE set to PPS_FIRST"<<endl;
        }

        session.get_property("MEAS_PROG",value); //current measurement program
        cout<<"MEAS_PROG is "<<value<<endl;

        session.execute_command("MEAS_SET_PROG","1",&result,&status,&comment); // set measurement program
        cout<<"MEAS_SET_PROG(1) returned "<<result<<endl;
        for(int i=0;i<comment.size();i++)
            cout<<comment[i]<<endl;
        cout<<"Status is "<<status<<endl;

        session.execute_command("SCN_SET_SCAN","30, 130, 0.9875, 180",&result,&status,&comment);
        cout<<"SCN_SET_SCAN(30, 130, 10) returned "<<result<<endl;
        for(int i=0;i<comment.size();i++)
            cout<<comment[i]<<endl;
        cout<<"Status is "<<status<<endl;

        session.execute_command("MEAS_START","",&result,&status,&comment); //line scan details : starting theta, theta_end,increment=0.09875 , phi(alignment)=180
        cout<<"MEAS_START() returned "<<result<<endl;
        for(int i=0;i<comment.size();i++)
            cout<<comment[i]<<endl;
        cout<<"Status is "<<status<<endl;

                                 /*acquirirng data*/

        cout<<"Output:"<<argv[2]<<endl;
        ofstream outfile(argv[2],ios::binary|ios::out);//binary file created in output mode

            /* opening input connection to scanner: */

        bool shutdown_requested=false;
        string connstr(argv[1]);//string connstr(argv[1]);
        connstr="rdtp://"+connstr+"/current";
        cout<<"Input: "<<connstr<<endl;

        std::shared_ptr<scanlib::basic_rconnection> rc;
        rc = basic_rconnection::create(connstr);
        rc->open();

        basic_rconnection::size_type limit=50*1024000;  //basic_rconnection::size_type limit = 50*1000000;
        basic_rconnection::size_type count;
        basic_rconnection::size_type total=0;
        char buffer[8192];  //1 kB of char
        int last_percent=0;
        int data_percent;

        cout<<"Data limit:"<<limit<<" bytes"<<endl;
        cout<<"Percent:"<<endl;
        cout<<"1..10...20...30...40...50...60...70...80...90..100"<<endl;

        /* loop runs as long as the instrument delivers data */

        while(0!=(count=rc->readsome(buffer,8192)))
        {
            outfile.write(buffer,count);
            total+=count;
            if((total>limit)&&(!shutdown_requested))
            {
                /*requesting shutdown of data delivery*/
                rc->request_shutdown();
                shutdown_requested=true;
            }
            data_percent= min(100,int(ceil(total*100.0/limit)));

            /*updating progress bar*/
            while(last_percent<data_percent)
                {
                    cout<<"*";
                    last_percent += 2; // 1 '*' = 2 percent
                }
        }
        cout<<endl;

        outfile.flush(); // flushing the '.rxp' output file
        outfile.close(); // closing the '.rxp' output file

        cout<<total<<" bytes received"<<endl;

                   /*stopping measurement:*/

        session.execute_command("MEAS_STOP","",&result,&status,&comment);
        cout<<"MEAS_STOP() returned "<<result<<endl;
        for(int i=0;i<comment.size();i++)
            cout<<comment[i]<<endl;
        cout<<"Status is "<<status<<endl;

        /* the connection is closed if the instrument is no longer needed */

        session.close();  // closing the connection
        cout << "Connection closed" << endl;

        cout<<"Demo program successfully finished"<<endl;
    }
    catch(exception& e)
    {
        cerr<<e.what()<<endl;
        return 1;
    }
    catch(...)
    {
        cerr<<"unknown exception"<<endl;
        return 1;
    }

    return 0;
}
