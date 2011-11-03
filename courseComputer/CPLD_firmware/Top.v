`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Splashelec
// Engineer: Bernt Weber
// 
// Create Date:    09:39:48 10/07/2010 
// Design Name: 
// Module Name:    Top 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////


module Top(
//    input [2:0] HbridgeMode,
    input Hbridge_pwm,
//    input [5:2] switch,
//    output [8:1] reserve,
//    input noHardwareError,
//    input noOvercurrent,
//    input noNegativeOvercurrent,
//    input heatsinkTempOk,
    output driveHighLeft,
    output driveLowLeft,
    output driveHighRight,
    output driveLowRight,
    input GCK1,
    inout GTS1 //,
//    input GSR1
    );
	 
reg pwm_reg; // register to synchronize pwm signal with clock

reg HbridgeDirection; // while Hbridge is switched on this stores the direction:
                      // High left + low right : HbridgeDirection == 0
							 // Low left + high right : HbridgeDirection == 1
							 
							 
reg [3:0] intervalCounter;
reg waiting; // active for a safety interval after switching off a transistor

// registers for Transistor states
reg HL;
reg LL;
reg HR;
reg LR;

// pullup on GTS1 which is used as active_low;
assign GTS1 = 1'bz;

// connect output drivers to correspoding state register
// using a tristate-buffer
//assign <output_wire> = <enable> ? <data> : 1'bz;
assign driveHighLeft  = ~GTS1 ? HL : 1'bz;
assign driveLowLeft   = ~GTS1 ? LL : 1'bz;
assign driveHighRight = ~GTS1 ? HR : 1'bz;
assign driveLowRight  = ~GTS1 ? LR : 1'bz;

// for testing
//assign reserve [4:1] = intervalCounter[3:0];
//assign reserve[5] = HL;
//assign reserve[6] = LL;
//assign reserve[7] = HR;
//assign reserve[8] = LR;


initial
begin
intervalCounter[3:0] = 4'b0000;
waiting = "1";
HbridgeDirection = "0";
pwm_reg = "0";
HL = "0";
LL = "0";
HR = "0";
LR = "0";
end

always @ ( posedge GCK1 ) //clk rising edge
	begin		
	// detect direction changes
	if (HbridgeDirection != pwm_reg)
		begin
		// switch off the transistors
		waiting = "1";
		HL = "0";
		LL = "0";
		HR = "0";
		LR = "0";
		intervalCounter[3:0] = 4'b0000;
		HbridgeDirection = pwm_reg;
		end
	
	if (waiting)
		begin
		if(intervalCounter < 4'b1100) 
			begin
			intervalCounter = intervalCounter + 1;	// increment
			end
		else 
			begin // stop waiting
			waiting = "0";
			end
		end
   else // no waiting
	   begin
		if (pwm_reg) // switch on bridge in first direction 
			begin
			HL = "1";
			LL = "0";
			HR = "0";
			LR = "1";
			end
		else
			begin
			HL = "0";
			LL = "1";
			HR = "1";
			LR = "0";
			end
		end
  
   // synchronization register
   pwm_reg = Hbridge_pwm;
		
	end


endmodule
