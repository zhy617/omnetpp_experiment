//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
import inet.common.misc.ThruputMeteringChannel;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;

network TCPUDP
{
    parameters:
        int n; // n for Tcp client number.
    types:
        channel ethernetline extends ThruputMeteringChannel
        {
            delay=1us;
            datarate=10000000kbps;
            thruputDisplayFormat = "p-b-u";
        }
    submodules:
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=100,100;is=s");
        }
        router1: Router {
            @display("p=400,250");
        }
        router2: Router {
            @display("p=550,250");
        }
        router3: Router {
            @display("p=700,250");
        }
        client[n]: StandardHost {
            parameters:
                @display("i=device/laptop;p=250,100,col,100");
        }
        clientUdp: StandardHost {
            parameters:
                @display("i=device/laptop;p=240,600,col,100");
        }
        server: StandardHost {
            parameters:
                @display("i=device/server;p=850,150");
        }
        serverUdp: StandardHost {
            parameters:
                @display("i=device/server;p=850,250");
        }
    connections:
        for i=0..n-1 {
            client[i].pppg++ <--> ethernetline <--> router1.pppg++;
        }
        clientUdp.pppg++ <--> ethernetline <--> router1.pppg++;
        router1.pppg++ <--> ethernetline <--> router2.pppg++;
        router2.pppg++ <--> ethernetline <--> router3.pppg++;
        router3.pppg++ <--> ethernetline <--> server.pppg++;
        router3.pppg++ <--> ethernetline <--> serverUdp.pppg++;
}
