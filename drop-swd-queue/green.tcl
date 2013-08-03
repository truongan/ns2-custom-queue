# ns green.tcl <type> <Mbps> <nodes> <pareto> [idmaps]
# type - Green, RED, SFB, etc.
# Mbps - bottleneck link bandwidth
# nodes - number of sender/receiver pairs on either side of bottleneck link
# pareto - 1 will use pareto ftp traffic, 0 will use infinite file ftp traffic
# idmaps - 1 will use IDMaps estimator, 0 will use embedded (TCP header) RTTs

# simulates low bandwidth flows in the background
set LOWBW_FLOWS 0

# 600 is roughly the bandwidth delay product of the bottleneck link for the
# simulations we ran. Set this accordingly
set QSIZE 600

set ns [new Simulator]
set type [lindex $argv 0]
set bneck_bw_mbps [lindex $argv 1]
set bneck_bw [lindex $argv 1]Mbps
set usepareto [lindex $argv 3]

#default nodes is 30
set nodenum 30
if {$argc >= 3} {
    set nodenum [lindex $argv 2]
}

if {$type == "Green"} {
    set idmaps [lindex $argv 4]
    puts "idmaps: $idmaps"
}

puts "args: $argc"
puts "Queue type: $type"
puts "Bottleneck bw: $bneck_bw"
puts "Num nodes: $nodenum"
puts "Pareto: $usepareto"

#bottleneck link delay 0.03 = 30ms
set bdelay 0.03

#vary delay of senders/receivers from 1ms to 200ms
set MAX_DELAY 0.2

#create the random number generator
set rng [new RNG]

#set no. of non TCP flows here
set pareto_srcs $nodenum

 
# turn on ns and nam tracing
#set f [open out.tr w]
#$ns trace-all $f
#$ns namtrace-all [open out.nam w]

#initialise average throughput
set avg_throughput -1

#set tcpacked_file [open "tcp_sent$type.out" w]
#set linksent_file [open "link_sent$type.out" w]


#the queue_type you are required to compare include DropTail, DRR, FQ, RED

set queue_type $type
set file_type $type

if {$file_type == "Green" && $idmaps == 1} {
   set file_type "GreenIDMaps"
}

puts "File type: $file_type"

#set $queue_type DropTail

#set start_time 1.0
#set log_time 50.0
#set finish_time 420.0

set start_time 1.0
set log_time 5.0
set finish_time 60.0

#progress metering
set progress_count 0
set progress_interval [expr $finish_time/10]


# create the nodes

#First create TCP senders and receivers

for {set i 0} {$i < $nodenum} {incr i} {
    
    set s($i) [$ns node]
    set r($i) [$ns node]
    

    if {$LOWBW_FLOWS == 1} {
	set ls($i) [$ns node]
	set lr($i) [$ns node]
	
    }

    if {$usepareto == 1} {
	set pareto_s($i) [$ns node]
	set pareto_r($i) [$ns node]
    }
    
}


#Then create the 2 routers
set n1 [$ns node]
set n2 [$ns node]    
    
puts "Queue type is $queue_type"

#Bottleneck link
$ns simplex-link $n1 $n2 $bneck_bw $bdelay $queue_type
$ns simplex-link $n2 $n1 $bneck_bw $bdelay DropTail

if {$queue_type == "SFB"} {
    set sfbq [[$ns link $n1 $n2] queue]
    $sfbq set hold-time 100ms
#    $sfbq set pbox-time 500ms
}

if {$queue_type == "SFQ"} {
    set sfq [[$ns link $n1 $n2] queue]
    $sfq limit 1500
    $sfq buckets 1000
    
}

if {$queue_type == "Vq"} {
    set vqq [[$ns link $n1 $n2] queue]
    $vqq set ecnlim_ 0.80
    $vqq set buflim_ 1.0
    $vqq set limit_ 150
    $vqq set update_every_N_packets_ false
    $vqq set timeoutvalue_ 2.0
    $vqq set gamma_ 0.97
    
}

if {$queue_type == "FRED"} {
   set fredq [[$ns link $n1 $n2] queue]
   $fredq set many-flows_ 0
   $fredq set thresh_ 150
   $fredq set maxthresh_ 400

}

if {$queue_type == "CSFQ"} {
   set csfq [[$ns link $n1 $n2] queue]
    $csfq set qsize_ [expr $QSIZE*8000]
    $csfq set qsizeThresh_ [expr $QSIZE*8000*3/4]
    $csfq set rate_ [expr $bneck_bw_mbps*1000000]


}

if {$queue_type == "DropTail"} {
    set droptailq [[$ns link $n1 $n2] queue]
}

if {$queue_type == "Green"} {

    set greenq [[$ns link $n1 $n2] queue]
    $greenq set bw_ [expr $bneck_bw_mbps*1000000]
    $greenq set c_ 1.31
    
    $greenq set idmaps_ $idmaps
}

for {set i 0} {$i < $nodenum } {incr i} {

    if {$nodenum > 1} {
	set delay [expr 0.001 + (($MAX_DELAY - 0.001)*$i)/($nodenum-1)]
    } else {
	set delay 0.001
    }

    # ftp sources
    $ns duplex-link $s($i) $n1 10Mbps 5ms DropTail
    $ns queue-limit $s($i) $n1 500
    $ns queue-limit $n1 $s($i) 500

    $ns duplex-link $r($i) $n2 10Mbps $delay DropTail
    $ns queue-limit $r($i) $n2 500
    $ns queue-limit $n2 $r($i) 500

    # low b/w ftp sources
    if {$LOWBW_FLOWS == 1} {
	$ns duplex-link $ls($i) $n1 56Kbps 5ms DropTail
	$ns queue-limit $ls($i) $n1 500
	$ns queue-limit $n1 $ls($i) 500
	
	$ns duplex-link $lr($i) $n2 10Mbps $delay DropTail
	$ns queue-limit $lr($i) $n2 500
	$ns queue-limit $n2 $lr($i) 500
    }

    # pareto sources
    if {$usepareto == 1} {
	$ns duplex-link $pareto_s($i) $n1 10Mbps 5ms DropTail
	$ns queue-limit $pareto_s($i) $n1 500
	$ns queue-limit $n1 $pareto_s($i) 500
	
	$ns duplex-link $pareto_r($i) $n2 10Mbps $delay DropTail
	$ns queue-limit $pareto_r($i) $n2 500
	$ns queue-limit $n2 $pareto_r($i) 500
	
    }

    set rtt [expr 2*(0.005+$delay + $bdelay)]
    
#    if {$queue_type == "DropTail"} {    
#	$droptailq set-rtt $i $rtt
#	$droptailq set-rtt [expr $i + $nodenum] $rtt
#    }

    if {$queue_type == "Green"} {

	$greenq set-rtt $i $rtt
	$greenq set-rtt [expr $i + $nodenum] $rtt
	
	# set the estimation technique
	set p [$rng uniform 0 1]
	
	
	
    }
}


#set the queue-limit between n1 and n2
$ns queue-limit $n1 $n2 $QSIZE


# Uncomment for queue logging, also uncomment the "close" in finish()
set qsamples 0
set qsum 0.0
set qsum_2 0.0

#set qfile [open "/tmp/akapadia/qsize$type.out" w]
$ns at $log_time "queueLog"

set statfile [open "stats$file_type$nodenum.out" w]

set qt [open "queuetrace$type.out" w]
set qm [$ns monitor-queue $n1 $n2 $qt 0.5]
#set qm [$ns monitor-queue $n1 $n2 [$ns get-ns-traceall]]


$ns at 0.0 "progressmeter"
$ns at $log_time "logdata"
$ns at $finish_time "finish"


#Agents can be FTP or Pareto . comment out appropriate lines
# as needed.

# create TCP agents
for {set i 0} {$i < $nodenum} {incr i} {
    
    #regular TCP agents
    set tcp($i) [new Agent/TCP]
    $tcp($i) set fid_ [expr ($i)]
    $tcp($i) set window_ 3000
    set sink($i) [new Agent/TCPSink]

    $ns attach-agent $s($i) $tcp($i)
    $ns attach-agent $r($i) $sink($i)
    $ns connect $tcp($i) $sink($i)
    set ftp($i) [new Application/FTP]
    $ftp($i) attach-agent $tcp($i)

    #background low b/w  TCP agents
    if {$LOWBW_FLOWS == 1} {
	set ltcp($i) [new Agent/TCP]
	$ltcp($i) set fid_ [expr ($i+$nodenum)]
	$ltcp($i) set window_ 3000
	set lsink($i) [new Agent/TCPSink]
	
	$ns attach-agent $ls($i) $ltcp($i)
	$ns attach-agent $lr($i) $lsink($i)
	$ns connect $ltcp($i) $lsink($i)
	set lftp($i) [new Application/FTP]
	$lftp($i) attach-agent $ltcp($i)
	$ns at 0 "$lftp($i) start"
	$ns at $finish_time "$lftp($i) stop"
    }

    #pareto agents
    if {$usepareto == 1} { 
	set ptcp($i) [new Agent/TCP]
	$ptcp($i) set fid_ [expr ($i+$nodenum)]
	$ptcp($i) set window_ 3000
	set psink($i) [new Agent/TCPSink]
	
	$ns attach-agent $pareto_s($i) $ptcp($i)
	$ns attach-agent $pareto_r($i) $psink($i)
	$ns connect $ptcp($i) $psink($i)
	
	set pareto_sender($i) [new Application/Traffic/Pareto]
	$pareto_sender($i) set packetSize_ 1000
	$pareto_sender($i) set burst_time_ 2s
	$pareto_sender($i) set idle_time_ 5s
	$pareto_sender($i) set shape_ 1.5
	$pareto_sender($i) set rate_ 160K
	$pareto_sender($i) attach-agent $ptcp($i)

	$ns at 0 "$pareto_sender($i) start"
	$ns at $finish_time "$pareto_sender($i) stop"
    }

    set start_time [$rng uniform 0 1]
    $ns at $start_time "$ftp($i) start"
    $ns at $finish_time "$ftp($i) stop"

    #$ns at $start_time "$p($i) start"
}


# create a new Timer class
Class eTimer -superclass Timer
Class eTimer2 -superclass Timer

# initialize the timer with a unique id
eTimer instproc init { ns tid} {
    $self set tid_ $tid
    $self set prev_seq 0
    $self set recv_seq 0
    #seqno of last recvd+acked packet
    eval $self next $ns
}

eTimer2 instproc init {ns tid} {
    $self set tid_ $tid
    $self set prev_seq 0
    $self set recv_seq 0
    eval $self next $ns
}

#this is done to remove the intial transient
proc logdata {} {
    global ns nodenum tcp tcpacked linksent1 qm sink forcedDrops greenq queue_type earlyDrops totalsent
    #puts "logging data"
    
    if {$queue_type == "Green"} {

	set forcedDrops [$greenq set greenForcedDrops_] 
	set earlyDrops  [$greenq set greenEarlyDrops_] 
    }

    for {set i 0} {$i < $nodenum} {incr i} {
	set rt [$tcp($i) set nrexmitpack_]
	set total [$tcp($i) set ndatapack_]
#	set tcpacked($i) [expr $total - $rt] 
#	set tcpacked($i) [$sink($i) getseq] 
#	set tcpacked($i) [expr [$tcp($i) set ndatapack_] - [$tcp($i) set nrexmitpack_]]
	set tcpacked($i) [expr [$tcp($i) set ndatapack_]]
	puts "$i sent $tcpacked($i)"
    }
    set linksent1 [$qm set pdepartures_]

}

proc progressmeter {} {
    global progress_count progress_interval ns
    set now [$ns now]
    puts "[expr $progress_count * 10]% done ($now s)"
    set progress_count [expr $progress_count+1]
    $ns at [expr $now+$progress_interval] progressmeter
    
}

proc queueLog {} {
    global ns qfile qm qsamples qsum qsum_2
    set qsamples [expr $qsamples + 1]

    set qs [$qm set pkts_]
    set qsum [expr $qsum + $qs]
    set qsum_2 [expr $qsum_2 + $qs*$qs]

    set now [$ns now]
#    puts $qfile "$now $qs"
    $ns at [expr $now+0.02] queueLog
}


proc finish {} {
    global ns sink nodenum file sfile qfile plossfile linkutilfile tcp tcpacked_file tcpacked qm linksent1 bneck_bw_mbps statfile finish_time log_time greenq forcedDrops queue_type qt earlyDrops totalsent qsamples qsum type file_type qsum_2 ptcp
    #pm_file pm_eff_file
    $ns flush-trace
    #puts "TCP tputs"

    set total_tcpacked 0
    set total_rtx 0
    set total_tcpacked_trans 0

    if {$queue_type == "Green"} {
	
	set tfd [$greenq set greenForcedDrops_] 
	set forcedDrops [expr $tfd - $forcedDrops]

	set ted [$greenq set greenEarlyDrops_] 
	set earlyDrops [expr $ted - $earlyDrops]
	puts $statfile "green forced drops $forcedDrops"
	puts $statfile "green early drops $earlyDrops"

    }

    for {set i 0} {$i < $nodenum} {incr i} {
	set rt [$tcp($i) set nrexmitpack_]
#	set acked [expr [$tcp($i) set ndatapack_] - [$tcp($i) set nrexmitpack_]]
	set acked [expr [$tcp($i) set ndatapack_]]
	set total_tcpacked [expr $total_tcpacked + $acked]
	set total_tcpacked_trans [expr $total_tcpacked_trans + $tcpacked($i)]
	set total_rtx [expr $total_rtx + $rt]
	
	set tcp_log($i) [expr $acked - $tcpacked($i)]
    }

#    close $tcpacked_file
    close $qt

    set totalsent [$qm set pdepartures_]
    set dt [expr $finish_time - $log_time]

#    puts $statfile "total_link_arrivals [$qm set parrivals_]"
#    puts $statfile "total_link_drops [$qm set pdrops_]\n"
#    puts $statfile "total_tcp_rtx $total_rtx"

#    puts $statfile "link_sent_trans $linksent1"
#    puts $statfile "tcp_acked_trans $total_tcpacked_trans\n"
 
#   puts $statfile "link_sent_total $totalsent"
#   puts $statfile "tcp_acked_total $total_tcpacked\n"

    puts $statfile "link_packet_loss [expr ([$qm set pdrops_]+0.0)/[$qm set parrivals_]]"
    set qavg [expr $qsum/$qsamples]
    puts $statfile "avg_queue_size $qavg"
    puts $statfile "queue_size_std [expr sqrt($qsum_2/$qsamples - $qavg*$qavg)]"
    puts $statfile "util [expr ($totalsent - $linksent1)*1.04*8/($bneck_bw_mbps * 10 * $dt)]"
#    puts $statfile "goodutil [expr ($total_tcpacked - $total_tcpacked_trans)*8/($bneck_bw_mbps * 10 * $dt)]"

#    puts $statfile "\nFairness Data:"
# Calculate fairness here
    set tcpfile [open "tcp_sent$file_type$nodenum.out" w]
#   set ptcpfile [open "ptcp_sent$file_type$nodenum.out" w]

    set sum 0.0
    set sum_square 0.0
    set min $tcp_log(0)

    for {set i 1} {$i < $nodenum} {incr i} {
	if {$tcp_log($i) < $min} {
	    if {$tcp_log($i) != 0} {
		set min $tcp_log($i)
	    }
	}
    }

    for {set i 0} {$i < $nodenum} {incr i} {
	
#	puts $ptcpfile "$i [$ptcp($i) set ndatapack_]"
	puts $tcpfile "$i $tcp_log($i)"
	set x [expr $tcp_log($i)/$min]
	set sum [expr $sum + $x]
	set sum_square [expr $sum_square + $x*$x]
    }

    close $tcpfile
#    close $ptcpfile
    
    set fairness [expr ($sum*$sum)/($nodenum*$sum_square)]

    puts $statfile "fairness $fairness"
    close $statfile

    
#    close $qfile
#    close $plossfile
#    close $linkutilfile
    
    
    #close pm_file
    #close pm_eff_file

    #puts "running nam..."
    #exec nam out.nam &
    exec rm -f $qt
    #    exec xgraph *.tr -geometry 800x400 &
    exit 0
}


$ns run


