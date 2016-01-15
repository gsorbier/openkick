#!/usr/bin/env perl
use warnings;
use strict;
use 5.010;

my $rom = do { local $/; scalar <> };
utf8::downgrade($rom);

my $top = 16<<20;
my $bottom = $top - length $rom;

# It's easier to pad NULs onto the end of the ROM than try and deal with cases
# where our unpacks may fall off the end.
$rom .= '\0' x 256;

sub kunpack($$) {
    my($template, $address) = @_;
    return unpack $template, substr($rom, ($address-$bottom));
}

my %TYPES = qw/
                  0 ?
                  1 task
                  3 dev
                  8 rsrc
                  9 lib
              /;

sub type($) { return $TYPES{$_[0]} // $_[0] };

my @FLAGS = qw/ C S D 3 4 5 6 A /;

sub flags($) {
    my($flags) = @_;
    my @bits = grep { $flags & (1<<$_) } (0..$#FLAGS);
    return '-' unless @bits;
    return join '', @FLAGS[@bits];
};

my $walk = $bottom;
my $skipstart = $walk;

while($rom =~ /\x{4a}\x{fc}/sg) {
    $walk = $bottom - 2 + pos $rom;
#    warn sprintf "%x", $walk;
    # if we matched on an odd address, it's definitely not a matchword
    next if $walk % 2;

    my($magic, $start, $end, $flags, $version, $type, $pri, $name, $id, $init)
        = kunpack 'nNNCCCcNNN', $walk;
    next unless $magic == 0x4afc && $start == $walk;

    if($skipstart < $walk) {
        printf "%6x%7d Data\n", $skipstart, $walk - $skipstart;
    }

    for($name, $id) {
        $_ = kunpack 'Z*', $_;
        s/[\r\n]//g;
    }

    my($first) = split /\./, $name;
    my $idname = $id;
    $idname = "$name: $idname"
        unless $idname =~ s/^$first /[$name] /;
    $idname = "[$name]" if $name eq $id;

    printf "%6x%7d %-43.43s v%d %-4s%4d %s@%x\n",
        $start, $end-$start, $idname,
            $version, type $type, $pri, flags($flags), $init;
    #printf "%X",  kunpack 'I>', 0xfc0008;
    $walk = $end;
    $skipstart = $walk;
}
$walk = $top;

if($skipstart < $walk) {
    printf "%6x%7d Data\n", $skipstart, $walk - $skipstart;
}
