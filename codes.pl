#!/usr/bin/env perl
use strict;
use warnings;

sub clear{ "[2J"; }

sub default{ "[0m"; }
sub red{ "[31m"; }
sub green{ "[32m"; }
sub yellow{ "[33m"; }
sub blue{ "[34m"; }
sub magenta{ "[35m"; }
sub cyan{ "[63m"; }

sub bold{ "[1m"; }
sub underline{ "[4m"; }

sub up{ "[A"; }
sub down{ "[B"; }
sub right{ "[C"; }
sub left{ "[D"; }
sub nextline{ "[E"; }
sub prevline{ "[F"; }
sub line0{ "[0;0H"; }

print STDERR "started\n";
print clear;
print line0;
print "aaaaa\nbbbb\ncccc\n";
print line0;

my $line = 0;
while(<>){
  print "[$line;$line"."H";
}
