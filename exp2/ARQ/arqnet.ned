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
//import receiver;

package test2;

simple sender
{
	parameters:
		@display("i=block/process");
	gates:
		input in;
		output out;
}
simple receiver
{
	parameters:
		@display("i=block/process");
	gates:
		input in;
		output out;
}
network Arqnet
{
	@display("bgb=774,582");
	submodules:
		sender:sender{
			parameters:
				@display("i=,cyan;p=545,141");
		}
		receiver: receiver {
			parameters:
				@display("i=,cyan;p=545,274");
		}
	connections:
		sender.out --> { delay = 100ms;} --> receiver.in;
		sender.in <-- { delay = 100ms;} <-- receiver.out;
}
