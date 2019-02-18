# Taking inputs and evaluting the syntax
if { $argc != 2 } {
puts "Invalid use"
puts "Try : ns $argv0 <TCP_flavor> <case_no>"
}
set case [lindex $argv 1]
if {$case > 3 || $case < 1} {
puts "Invalid case_no: Try 1,2,3"
exit
}

#Checking for the flavor of TCP to be simulated.
global flav
set flavor [lindex $argv 0]
if {$flavor == "SACK"} {
set flav "Sack1"
} elseif {$flavor == "VEGAS"} {
set flav "Vegas"
} else {
puts "Invalid TCP flavor $flavor"
exit
} 

#Assigning the value to delay depending on case number.
global delay
set delay 0
switch $case {
	global delay
	1 {set delay "12.5ms" }
	2 {set delay "20ms" }
	3 {set delay "27.5ms" }
}

#Creating event scheduler
set ns [new Simulator]

#Tracepackets on all links
set flow1 [open output1.tr w]
set flow2 [open output2.tr w]
set file "output_$flavor$case"
set tracefile [open output.tr w]
$ns trace-all $tracefile
set namfile [open output.nam w]
$ns namtrace-all $namfile
 
set throughput1 0
set throughput2 0
set counter 0

#Defining nodes
set src1 [$ns node]
set src2 [$ns node]
set R1 [$ns node]
set R2 [$ns node]
set rcv1 [$ns node]
set rcv2 [$ns node]


#Differentiating flows
$ns color 1 Red
$ns color 2 Green

#Links and queuing
$ns duplex-link $R1 $R2 1Mb 5ms DropTail
$ns duplex-link $src1 $R1 10Mb 5ms DropTail
$ns duplex-link $src2 $R1 10Mb $delay DropTail
$ns duplex-link $rcv1 $R2 10Mb 5ms DropTail
$ns duplex-link $rcv2 $R2 10Mb $delay DropTail

#Network dynamics
$ns duplex-link-op $R1 $R2 orient center
$ns duplex-link-op $src1 $R1 orient right-down
$ns duplex-link-op $src2 $R1 orient right-up
$ns duplex-link-op $R2 $rcv1 orient right-up
$ns duplex-link-op $R2 $rcv2 orient right-down

#Creating TCP connection
set TCP1 [new Agent/TCP/$flav]
set TCP2 [new Agent/TCP/$flav]
$ns attach-agent $src1 $TCP1
$ns attach-agent $src2 $TCP2
$TCP1 set class_ 1
$TCP2 set class_ 2

#creating traffic on top of TCP
set FTP1 [new Application/FTP]
set FTP2 [new Application/FTP]
$FTP1 attach-agent $TCP1
$FTP2 attach-agent $TCP2
set TCPsink1 [new Agent/TCPSink]
set TCPsink2 [new Agent/TCPSink]
$ns attach-agent $rcv1 $TCPsink1
$ns attach-agent $rcv2 $TCPsink2
$ns connect $TCP1 $TCPsink1
$ns connect $TCP2 $TCPsink2

#Termination
proc finish {} {
	global ns nf tracefile namfile file throughput1 throughput2 counter
	$ns flush-trace
	puts "average throughput for source_1=[expr $throughput1/$counter] Mbps\n"
	puts "average throughput for source_2=[expr $throughput2/$counter] Mbps\n"
	close $tracefile
	close $namfile
	exec nam output.nam &
 	exit 0
}

#Calculating the required parameters
proc record {} {
	global TCPsink1 TCPsink2 flow1 flow2 throughput1 throughput2 counter
	set ns [Simulator instance] 
	set time 0.5
	set bandwidth1 [$TCPsink1 set bytes_]
	set bandwidth2 [$TCPsink2 set bytes_]
	set now [$ns now]
	puts $flow1 "$now [expr $bandwidth1/$time*8/1000000]"
	puts $flow2 "$now [expr $bandwidth2/$time*8/1000000]"
	set throughput1 [expr $throughput1 + $bandwidth1/$time*8/1000000]
	set throughput2 [expr $throughput2 + $bandwidth2/$time*8/1000000]
	set counter [expr $counter+1]
	
	$TCPsink1 set bytes_ 0
	$TCPsink2 set bytes_ 0
	$ns at [expr $now + $time] "record"
}

#Running the script
$ns at 0 "record"
$ns at 0 "$FTP1 start"
$ns at 0 "$FTP2 start"
$ns at 400 "$FTP1 stop"
$ns at 400 "$FTP2 stop"
$ns at 400 "finish"

set tf1 [open "$file-s1.tr" w]
$ns trace-queue $src1 $R1 $tf1
set tf2 [open "$file-s2.tr" w]
$ns trace-queue $src2 $R1 $tf2

$ns run








