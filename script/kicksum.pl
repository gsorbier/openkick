#!/usr/bin/env perl
use warnings;
use strict;
use File::Slurp;

# Given an Amiga ROM executable on STDIN, this will pad the image to 512kB and
# set the last 32 bit word such that the truncated 32 bit sum-with-carry of all
# of the words when treated as unsigned m68k long integers is 0xffffffff. Words
# are in m68k endian order, even when this script is running on another arch.
# This satisfies the AmigaOS ROM checksum test.

my $rombytes = 512 * 1024;
my $romwords = $rombytes / 4;
my $format = "N$romwords";
my $wantsum = 0xffffffff;

sub checksum {
    my($sum, $prev) = (0, 0);
    foreach my $data (@_) {
        $sum = ($sum + $data) & 0xffffffff;
        if($sum < $prev) {
            $sum += 1;
        }
        $prev = $sum;
    }
    return $sum;
}

my $rom = read_file(\*STDIN);
my @words = unpack $format, $rom;
$#words = $romwords - 1;
@words = map $_//0, @words;

$words[-1] = 0;
$words[-1] = $wantsum - checksum(@words);
my $newsum = checksum(@words);
warn sprintf "BUG: miscalculated checksum, wanted %x, got %x", $wantsum, $newsum
    unless $wantsum == $newsum;
print pack $format, @words;


