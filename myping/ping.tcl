#Create a simulator object
set ns [new Simulator]

#Open a trace file
set nf [open out.nam w]
$ns namtrace-all $nf

set tf [open out.trace w]
$ns trace-all $tf

#Define a 'finish' procedure
proc finish {} {
        global ns nf tf
        $ns flush-trace
        close $tf
        close $nf
        exec nam out.nam &
        exit 0
}

#Create three nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]

Agent/APing set packetSize_ 64

#Connect the nodes with two links
$ns duplex-link $n0 $n1 1Mb 10ms DropTail
$ns duplex-link $n1 $n2 1Mb 10ms DropTail

#Define a 'recv' function for the class 'Agent/Ping'
Agent/APing instproc recv {from rtt} {
	$self instvar node_
	puts "node [$node_ id] received ping answer from \
              $from with round-trip-time $rtt ms."
}

#Create two ping agents and attach them to the nodes n0 and n2
set p0 [new Agent/APing]
$ns attach-agent $n0 $p0

set p1 [new Agent/APing]
$ns attach-agent $n2 $p1

#Connect the two agents
$ns connect $p0 $p1

#Schedule events
$ns at 0.1 "$p0 send"
$ns at 0.2 "$p1 send"
$ns at 0.3 "$p0 send"
$ns at 0.3 "$p1 send"
$ns at 0.5 "finish"

#Run the simulation
$ns run