# Script parses trace file and generates another trace file , 
# It adds the event of BACK_OFF which can be later addded in
# the mac file
sub usage {
	print STDERR "usage: $0 <source trace file> <output trace file>\n";
	print STDERR "<source trace file> - trace file generated by the base tcl file\n";
	print STDERR "<output trace file> - trace file generated after processing \n";
	exit;
}

#@ARGV[0] - source trace file
#@ARGV[1] - output trace file

if($#ARGV != 1) {
	&usage;
	exit;
}

open (Source, $ARGV[0]) or die "Cannot open $ARGV[0] : $!\n";
open (Destination, ">$ARGV[1]") or die "Cannot open $ARGV[1] : $!\n";

my $enter = 0;

$line = <Source>;
while ($line) {
	if($line =~ /^M/) {
		print Destination $line;
		$enter = 1;
	}
	elsif (($line =~ /SENSING_CARRIER/) && ($enter == 0)){
		print Destination $line;
		$temp = <Source>;
		if ($temp !~ /SENSING_CARRIER/) {
			print Destination $temp;
			$line = <Source>;
			@fields = split ' ', $line;
			$t = @fields[2] + 0.001;
			print Destination $line;
			print Destination 'E -t ', $t , ' BACKING_OFF ', @fields[4] , "\n";
		}
		else {
			print Destination $temp;
		}
	}
	else {
		print Destination $line;
	}
	$line = <Source>;
}

close Source;
close Destination;

