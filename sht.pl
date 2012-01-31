#!/usr/bin/env perl
use warnings;
use strict;

# current colours:
# types (int, struct, char, etc) Green
# literals (strings, numbers) red
# 'dangerous' keywords (return, include, define) magenta
# keywords (if, for, class, while, sub) yellow
# special punc (brackets) blue
# comments blue or cyan?? FIXME could change special punc to be cyan

sub token{
  my $str = shift;
  my @parts = split //, $str;
  my $token = "";
  my $p;
  while( defined ($p = shift @parts) ){
    if( $p eq '(' || $p eq ')' || $p eq '[' || $p eq ']' || $p eq '{' || $p eq '}' ){
      if( $token ){
        unshift @parts, $p;
        return ( $token, (join "", @parts) );
      } else {
        return ( $p, (join "", @parts) );
      }
    } elsif( $p eq ' ' ){
      return ( $token, (join "", @parts) ) if $token ne '';
    } else {
      $token .= $p;
    }
  }
  print "GOT HERE\n";
  print @parts;
  return ( $token, undef );
}

#mockup of syntax highlighitng tool
while( <> ){
  my $str = $_;
  my $token;
  while(){
    ($token, $str) = token($str);
    exit 1 unless defined $token;
    print $token, "\n";
    exit 1 unless defined $str;
    print "==== $str\n";
  }
}
